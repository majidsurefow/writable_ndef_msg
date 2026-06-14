/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/slix/slix_listener.c
 */

#include "protocols/slix/slix_listener.h"

#include "protocols/iso15693_3/iso15693_3_listener.h"
#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "protocols/iso15693_3/iso15693_3_poller_i.h"
#include "protocols/slix/slix_poller_i.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#define SLIX_LISTENER_BUF_SIZE 256U
#define SLIX_CMD_GET_RANDOM      0xB2U
#define SLIX_CMD_SET_PASSWORD    0xB3U
#define SLIX_CMD_CUSTOM_START    0xA0U

static slix_data_t s_model;
static bool s_initialized;
static uint8_t s_resp[SLIX_LISTENER_BUF_SIZE];
static bool s_password_match[SLIX_PASSWORD_DESTROY + 1U];
static uint16_t s_random;

static void slix_listener_send(const uint8_t *payload, size_t payload_len)
{
	uint8_t frame[SLIX_LISTENER_BUF_SIZE];
	size_t frame_len;

	if (payload_len > (sizeof(frame) - ISO15693_CRC_SIZE)) {
		return;
	}

	(void)memcpy(frame, payload, payload_len);
	frame_len = iso15693_3_crc_append(frame, sizeof(frame), payload_len);
	if (frame_len == 0U) {
		return;
	}

	(void)nfc_transport_send_response(frame, frame_len);
}

static uint32_t slix_listener_read_be32(const uint8_t *bytes)
{
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) |
	       (uint32_t)bytes[3];
}

static slix_password_type_t slix_listener_password_type_from_id(uint8_t id)
{
	for (uint8_t i = 0U; i < SLIX_PASSWORD_COUNT; i++) {
		if (id == (uint8_t)(1U << i)) {
			return (slix_password_type_t)i;
		}
	}

	return SLIX_PASSWORD_COUNT;
}

static uint32_t slix_listener_unxor_password(uint32_t password_xored, uint16_t random)
{
	uint8_t rn_l = (uint8_t)(random >> 8);
	uint8_t rn_h = (uint8_t)(random & 0xFFU);
	uint32_t double_rand = ((uint32_t)rn_h << 24) | ((uint32_t)rn_l << 16) |
			       ((uint32_t)rn_h << 8) | (uint32_t)rn_l;

	return password_xored ^ double_rand;
}

static bool slix_listener_check_password(slix_password_type_t type, uint32_t password)
{
	if (type >= SLIX_PASSWORD_COUNT) {
		return false;
	}

	if (s_model.capabilities == SLIX_CAP_ACCEPT_ALL) {
		s_password_match[type] = true;
		return true;
	}

	uint8_t pw[SLIX_PASSWORD_SIZE];

	pw[0] = (uint8_t)((password >> 24) & 0xFFU);
	pw[1] = (uint8_t)((password >> 16) & 0xFFU);
	pw[2] = (uint8_t)((password >> 8) & 0xFFU);
	pw[3] = (uint8_t)(password & 0xFFU);
	if (memcmp(pw, s_model.passwords[type], SLIX_PASSWORD_SIZE) == 0) {
		s_password_match[type] = true;
		return true;
	}

	return false;
}

static bool slix_listener_is_slix_custom(uint8_t command)
{
	return (command >= SLIX_CMD_CUSTOM_START) && (command < 0xC0U);
}

static void slix_listener_handle_get_nxp_sysinfo(void)
{
	size_t offset = 0U;
	uint8_t lock_bits = 0U;
	uint32_t feature_flags = 0x00000002U;

	s_resp[offset++] = 0U;
	s_resp[offset++] = s_model.protection_pointer;
	s_resp[offset++] = s_model.protection_condition;
	if (s_model.parent.lock_dsfid != 0U) {
		lock_bits |= 0x01U;
	}
	if (s_model.parent.lock_afi != 0U) {
		lock_bits |= 0x02U;
	}
	if (s_model.lock_eas != 0U) {
		lock_bits |= 0x04U;
	}
	if (s_model.lock_ppl != 0U) {
		lock_bits |= 0x08U;
	}
	s_resp[offset++] = lock_bits;
	s_resp[offset++] = (uint8_t)(feature_flags & 0xFFU);
	s_resp[offset++] = (uint8_t)((feature_flags >> 8U) & 0xFFU);
	s_resp[offset++] = (uint8_t)((feature_flags >> 16U) & 0xFFU);
	s_resp[offset++] = (uint8_t)((feature_flags >> 24U) & 0xFFU);
	slix_listener_send(s_resp, offset);
}

static void slix_listener_handle_read_signature(void)
{
	size_t offset = 0U;

	s_resp[offset++] = 0U;
	(void)memcpy(&s_resp[offset], s_model.signature, SLIX_SIGNATURE_SIZE);
	offset += SLIX_SIGNATURE_SIZE;
	slix_listener_send(s_resp, offset);
}

static void slix_listener_handle_get_random(void)
{
	size_t offset = 0U;

	s_random = (uint16_t)(0x1234U + (uint16_t)s_model.parent.uid[7]);
	s_resp[offset++] = 0U;
	s_resp[offset++] = (uint8_t)(s_random & 0xFFU);
	s_resp[offset++] = (uint8_t)((s_random >> 8U) & 0xFFU);
	slix_listener_send(s_resp, offset);
}

static void slix_listener_handle_set_password(const uint8_t *data, size_t data_len, bool addressed)
{
	slix_password_type_t type;
	uint32_t password;

	if (data_len < (1U + SLIX_PASSWORD_SIZE)) {
		return;
	}

	ARG_UNUSED(addressed);

	type = slix_listener_password_type_from_id(data[0]);
	password = slix_listener_unxor_password(slix_listener_read_be32(&data[1]), s_random);

	if (!slix_listener_check_password(type, password)) {
		return;
	}

	if (type == SLIX_PASSWORD_PRIVACY) {
		s_model.privacy_mode = 0U;
	}

	s_resp[0] = 0U;
	slix_listener_send(s_resp, 1U);
}

static void slix_listener_handle_custom(const uint8_t *frame, size_t frame_len, bool addressed)
{
	const uint8_t *data;
	size_t data_len;
	uint8_t command;

	if (frame_len < 3U) {
		return;
	}

	command = frame[1];
	if (frame[2] != SLIX_NXP_MFG_CODE) {
		return;
	}

	data = &frame[3];
	data_len = frame_len - 3U;
	if (addressed) {
		if (data_len < ISO15693_UID_SIZE) {
			return;
		}
		data = &data[ISO15693_UID_SIZE];
		data_len -= ISO15693_UID_SIZE;
	}

	switch (command) {
	case SLIX_CMD_GET_NXP_SYSINFO:
		slix_listener_handle_get_nxp_sysinfo();
		break;
	case SLIX_CMD_READ_SIGNATURE:
		slix_listener_handle_read_signature();
		break;
	case SLIX_CMD_GET_RANDOM:
		slix_listener_handle_get_random();
		break;
	case SLIX_CMD_SET_PASSWORD:
		slix_listener_handle_set_password(data, data_len, addressed);
		break;
	default:
		break;
	}
}

static void slix_listener_on_tag_cmd(const uint8_t *tx, size_t tx_len, void *user_ctx)
{
	uint8_t frame[SLIX_LISTENER_BUF_SIZE];
	size_t frame_len;
	uint8_t command;
	bool addressed;

	ARG_UNUSED(user_ctx);

	if (!s_initialized || (tx == NULL) || (tx_len < (1U + ISO15693_CRC_SIZE))) {
		return;
	}

	if (!iso15693_3_crc_check(tx, tx_len)) {
		return;
	}

	if (tx_len > sizeof(frame)) {
		return;
	}

	(void)memcpy(frame, tx, tx_len);
	frame_len = iso15693_3_crc_trim(frame, tx_len);
	if (frame_len < 2U) {
		return;
	}

	addressed = ((frame[0] & ISO15693_ADDR_FLAG_ADDRESSED) != 0U) &&
		    ((frame[0] & ISO15693_INV_FLAG_INVENTORY) == 0U);
	command = frame[1];

	if (slix_listener_is_slix_custom(command)) {
		slix_listener_handle_custom(frame, frame_len, addressed);
		return;
	}

	if (s_model.privacy_mode != 0U) {
		if (command == ISO15693_CMD_INVENTORY) {
			return;
		}
	}

	iso15693_3_listener_init(&s_model.parent);
	iso15693_3_listener_get()->on_tag_cmd(tx, tx_len, NULL);
}

static int slix_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return slix_serialize(&s_model, out, out_max, out_len);
}

static int slix_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return slix_deserialize(&s_model, in, in_len);
}

static const nfc_service_t s_slix_listener_service = {
	.on_tag_cmd = slix_listener_on_tag_cmd,
	.serialize = slix_listener_serialize,
	.deserialize = slix_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_SLIX,
	.user_ctx = NULL,
};

int slix_listener_init(const slix_data_t *model)
{
	if (model != NULL) {
		s_model = *model;
	} else {
		slix_data_reset(&s_model);
	}

	(void)memset(s_password_match, 0, sizeof(s_password_match));
	s_random = 0U;
	s_initialized = true;
	return 0;
}

int slix_listener_shutdown(void)
{
	slix_data_reset(&s_model);
	(void)memset(s_password_match, 0, sizeof(s_password_match));
	s_random = 0U;
	s_initialized = false;
	return 0;
}

const nfc_service_t *slix_listener_get(void)
{
	return &s_slix_listener_service;
}
