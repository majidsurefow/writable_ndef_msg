/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/iso15693_3/iso15693_3_listener.c
 */

#include "protocols/iso15693_3/iso15693_3_listener.h"

#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "protocols/iso15693_3/iso15693_3_poller_i.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#define ISO15693_LISTENER_BUF_SIZE 256U

static iso15693_3_data_t s_model;
static bool s_initialized;
static uint8_t s_resp[ISO15693_LISTENER_BUF_SIZE];

static void iso15693_3_listener_send(const uint8_t *payload, size_t payload_len)
{
	uint8_t frame[ISO15693_LISTENER_BUF_SIZE];
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

static uint8_t iso15693_3_listener_sysinfo_flags(void)
{
	uint8_t flags = 0U;

	flags |= ISO15693_SYSINFO_FLAG_DSFID;
	flags |= ISO15693_SYSINFO_FLAG_AFI;
	flags |= ISO15693_SYSINFO_FLAG_MEMORY;
	flags |= ISO15693_SYSINFO_FLAG_IC_REF;
	return flags;
}

static void iso15693_3_listener_handle_inventory(void)
{
	size_t offset = 0U;
	size_t uid_len = 0U;

	s_resp[offset++] = 0U;
	s_resp[offset++] = s_model.dsfid;
	iso15693_3_append_uid_reversed(s_model.uid, &s_resp[offset], &uid_len);
	offset += uid_len;
	iso15693_3_listener_send(s_resp, offset);
}

static void iso15693_3_listener_handle_get_sys_info(void)
{
	size_t offset = 0U;
	uint8_t info_flags = iso15693_3_listener_sysinfo_flags();

	s_resp[offset++] = 0U;
	s_resp[offset++] = info_flags;
	s_resp[offset++] = s_model.dsfid;
	s_resp[offset++] = s_model.afi;
	s_resp[offset++] = (uint8_t)(s_model.block_count - 1U);
	s_resp[offset++] = (uint8_t)((s_model.block_size - 1U) & 0x1FU);
	s_resp[offset++] = s_model.ic_ref;
	iso15693_3_listener_send(s_resp, offset);
}

static void iso15693_3_listener_handle_read_block(uint8_t block_num)
{
	size_t offset = 0U;
	size_t block_off;

	if (block_num >= s_model.block_count) {
		s_resp[offset++] = ISO15693_RESP_FLAG_ERROR;
		s_resp[offset++] = 0x10U;
		iso15693_3_listener_send(s_resp, offset);
		return;
	}

	block_off = (size_t)block_num * (size_t)s_model.block_size;
	s_resp[offset++] = 0U;
	(void)memcpy(&s_resp[offset], &s_model.block_data[block_off], s_model.block_size);
	offset += s_model.block_size;
	iso15693_3_listener_send(s_resp, offset);
}

static void iso15693_3_listener_handle_get_blocks_security(uint8_t first, uint8_t last)
{
	size_t offset = 0U;
	uint8_t block;

	if ((first >= s_model.block_count) || (last >= s_model.block_count) || (first > last)) {
		s_resp[offset++] = ISO15693_RESP_FLAG_ERROR;
		s_resp[offset++] = 0x10U;
		iso15693_3_listener_send(s_resp, offset);
		return;
	}

	s_resp[offset++] = 0U;
	for (block = first; block <= last; block++) {
		s_resp[offset++] = s_model.block_security[block];
	}
	iso15693_3_listener_send(s_resp, offset);
}

static bool iso15693_3_listener_uid_matches(const uint8_t *rx_uid)
{
	uint8_t expected[ISO15693_UID_SIZE];
	size_t uid_len = 0U;

	iso15693_3_append_uid_reversed(s_model.uid, expected, &uid_len);
	return memcmp(rx_uid, expected, ISO15693_UID_SIZE) == 0;
}

static void iso15693_3_listener_on_tag_cmd(const uint8_t *tx, size_t tx_len, void *user_ctx)
{
	uint8_t frame[ISO15693_LISTENER_BUF_SIZE];
	size_t frame_len;
	const uint8_t *data;
	size_t data_len;
	uint8_t flags;
	uint8_t command;

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

	flags = frame[0];
	command = frame[1];
	data = &frame[2];
	data_len = frame_len - 2U;

	if (((flags & ISO15693_INV_FLAG_INVENTORY) == 0U) &&
	    ((flags & ISO15693_ADDR_FLAG_ADDRESSED) != 0U)) {
		if (data_len < ISO15693_UID_SIZE) {
			return;
		}
		if (!iso15693_3_listener_uid_matches(data)) {
			return;
		}
		data = &data[ISO15693_UID_SIZE];
		data_len -= ISO15693_UID_SIZE;
	}

	switch (command) {
	case ISO15693_CMD_INVENTORY:
		iso15693_3_listener_handle_inventory();
		break;
	case ISO15693_CMD_GET_SYS_INFO:
		if (data_len == 0U) {
			iso15693_3_listener_handle_get_sys_info();
		}
		break;
	case ISO15693_CMD_READ:
		if (data_len >= 1U) {
			iso15693_3_listener_handle_read_block(data[0]);
		}
		break;
	case ISO15693_CMD_GET_BLOCKS_SEC:
		if (data_len >= 2U) {
			iso15693_3_listener_handle_get_blocks_security(data[0], data[1]);
		}
		break;
	default:
		break;
	}
}

static int iso15693_3_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return iso15693_3_serialize(&s_model, out, out_max, out_len);
}

static int iso15693_3_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return iso15693_3_deserialize(&s_model, in, in_len);
}

static const nfc_service_t s_iso15693_listener_service = {
	.on_tag_cmd = iso15693_3_listener_on_tag_cmd,
	.serialize = iso15693_3_listener_serialize,
	.deserialize = iso15693_3_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_ISO15693,
	.user_ctx = NULL,
};

int iso15693_3_listener_init(const iso15693_3_data_t *model)
{
	if (model != NULL) {
		s_model = *model;
	} else {
		iso15693_3_data_reset(&s_model);
	}

	s_initialized = true;
	return 0;
}

int iso15693_3_listener_shutdown(void)
{
	iso15693_3_data_reset(&s_model);
	s_initialized = false;
	return 0;
}

const nfc_service_t *iso15693_3_listener_get(void)
{
	return &s_iso15693_listener_service;
}

const iso15693_3_data_t *iso15693_3_listener_model(void)
{
	return s_initialized ? &s_model : NULL;
}
