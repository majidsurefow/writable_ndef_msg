/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Type-4 listener — T4T file FSM (cookbook §5.1 / wave5-ndef §3.6).
 */

#include "ndef_listener.h"

#include <errno.h>
#include <string.h>

#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
#include "nfc_stack/nfc_stack.h"
#endif

#define NDEF_FILE_ID_CC           0xE103U
#define NDEF_FILE_ID_NDEF         0xE104U

#define NDEF_FILE_BUF_SIZE        (NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE)

#define NDEF_SELECT_P2_FIRST_FCI  0x00U
#define NDEF_SELECT_P2_NO_FCI     0x0CU

#define NDEF_CC_WR_ACCESS_OPEN    0x00U
#define NDEF_CC_WR_ACCESS_NONE    0xFFU

typedef enum {
	NDEF_FILE_NONE = 0,
	NDEF_FILE_CC,
	NDEF_FILE_NDEF,
} ndef_file_sel_t;

static ndef_data_t s_model;
static bool s_writable;
static bool s_initialized;
static ndef_file_sel_t s_file_sel;
static uint8_t s_response_buf[NFC_TRANSPORT_MAX_RESPONSE_LEN];

static void ndef_listener_send_sw(uint16_t sw)
{
	s_response_buf[0] = (uint8_t)(sw >> 8U);
	s_response_buf[1] = (uint8_t)(sw & 0xFFU);
	(void)nfc_transport_send_response(s_response_buf, 2U);
}

static void ndef_listener_build_cc(uint8_t *cc, bool writable)
{
	uint16_t mle = (uint16_t)(CONFIG_NFC_NDEF_MAX_SIZE + NFC_NDEF_NLEN_FIELD_LEN);
	uint16_t mlc = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE;
	uint16_t max_ndef = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE;

	cc[0] = 0x00U;
	cc[1] = NFC_NDEF_CC_LEN;
	cc[2] = 0x20U;
	cc[3] = (uint8_t)(mle >> 8U);
	cc[4] = (uint8_t)(mle & 0xFFU);
	cc[5] = (uint8_t)(mlc >> 8U);
	cc[6] = (uint8_t)(mlc & 0xFFU);
	cc[7] = 0x04U;
	cc[8] = 0x06U;
	cc[9] = 0xE1U;
	cc[10] = 0x04U;
	cc[11] = (uint8_t)(max_ndef >> 8U);
	cc[12] = (uint8_t)(max_ndef & 0xFFU);
	cc[13] = 0x00U;
	cc[14] = writable ? NDEF_CC_WR_ACCESS_OPEN : NDEF_CC_WR_ACCESS_NONE;
}

static void ndef_listener_sync_file_len(void)
{
	uint16_t nlen;

	nlen = (uint16_t)(((uint16_t)s_model.ndef_file[0] << 8U) |
			  (uint16_t)s_model.ndef_file[1]);

	if (nlen <= CONFIG_NFC_NDEF_MAX_SIZE) {
		s_model.ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + nlen);
	}
}

static void ndef_listener_pad_ndef_file(void)
{
	if (s_model.ndef_file_len < NDEF_FILE_BUF_SIZE) {
		(void)memset(&s_model.ndef_file[s_model.ndef_file_len], 0,
			     (size_t)NDEF_FILE_BUF_SIZE - (size_t)s_model.ndef_file_len);
	}

	s_model.ndef_file_len = (uint16_t)NDEF_FILE_BUF_SIZE;
}

static void ndef_listener_on_select(const uint8_t *aid, size_t aid_len, void *user_ctx)
{
	ARG_UNUSED(aid);
	ARG_UNUSED(aid_len);
	ARG_UNUSED(user_ctx);

	s_file_sel = NDEF_FILE_NONE;
	ndef_listener_send_sw(NFC_SW_OK);
}

static bool ndef_listener_select_p2_valid(uint8_t p2)
{
	return (p2 == NDEF_SELECT_P2_FIRST_FCI) || (p2 == NDEF_SELECT_P2_NO_FCI);
}

static void ndef_listener_handle_select_file(const nfc_apdu_t *apdu)
{
	uint16_t file_id;

	if (apdu->p1 != NFC_SELECT_P1_BY_FILE_ID) {
		ndef_listener_send_sw(NFC_SW_WRONG_P1P2);
		return;
	}

	if (!ndef_listener_select_p2_valid(apdu->p2)) {
		ndef_listener_send_sw(NFC_SW_WRONG_P1P2);
		return;
	}

	if ((apdu->lc != 2U) || (apdu->data == NULL)) {
		ndef_listener_send_sw(NFC_SW_WRONG_LENGTH);
		return;
	}

	file_id = (uint16_t)(((uint16_t)apdu->data[0] << 8U) | (uint16_t)apdu->data[1]);

	if (file_id == NDEF_FILE_ID_CC) {
		s_file_sel = NDEF_FILE_CC;
		ndef_listener_send_sw(NFC_SW_OK);
		return;
	}

	if (file_id == NDEF_FILE_ID_NDEF) {
		s_file_sel = NDEF_FILE_NDEF;
		ndef_listener_send_sw(NFC_SW_OK);
		return;
	}

	ndef_listener_send_sw(NFC_SW_FILE_NOT_FOUND);
}

static uint32_t ndef_listener_read_ne(const nfc_apdu_t *apdu, uint32_t available)
{
	if (!apdu->has_le) {
		return available;
	}

	if (apdu->extended && (apdu->le == 0U)) {
		return (available < 65536U) ? available : 65536U;
	}

	if (!apdu->extended && (apdu->le == 0U)) {
		return (available < 256U) ? available : 256U;
	}

	return (uint32_t)apdu->le;
}

static void ndef_listener_handle_read_binary(const nfc_apdu_t *apdu)
{
	const uint8_t *file_ptr;
	uint32_t file_length;
	uint32_t offset;
	uint32_t available;
	uint32_t ne;
	uint32_t actual;
	size_t resp_len;

	if (s_file_sel == NDEF_FILE_NONE) {
		ndef_listener_send_sw(NFC_SW_NO_EF);
		return;
	}

	if (s_file_sel == NDEF_FILE_CC) {
		file_ptr = s_model.cc;
		file_length = NFC_NDEF_CC_LEN;
	} else {
		file_ptr = s_model.ndef_file;
		file_length = NDEF_FILE_BUF_SIZE;
	}

	offset = (uint32_t)(((uint32_t)apdu->p1 << 8U) | (uint32_t)apdu->p2);

	if (offset >= file_length) {
		ndef_listener_send_sw(NFC_SW_WRONG_OFFSET);
		return;
	}

	available = file_length - offset;
	ne = ndef_listener_read_ne(apdu, available);

	if (ne == 0U) {
		ndef_listener_send_sw(NFC_SW_WRONG_OFFSET);
		return;
	}

	actual = (ne < available) ? ne : available;

	if (actual > (uint32_t)(NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
		actual = (uint32_t)(NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U);
	}

	(void)memcpy(s_response_buf, &file_ptr[offset], actual);
	s_response_buf[actual] = NFC_SW1_OK;
	s_response_buf[actual + 1U] = NFC_SW2_OK;
	resp_len = (size_t)actual + 2U;
	(void)nfc_transport_send_response(s_response_buf, resp_len);
}

static void ndef_listener_handle_update_binary(const nfc_apdu_t *apdu)
{
	uint32_t offset;

	if (s_file_sel == NDEF_FILE_NONE) {
		ndef_listener_send_sw(NFC_SW_NO_EF);
		return;
	}

	if (s_file_sel == NDEF_FILE_CC) {
		ndef_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	if (!s_writable) {
		ndef_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	if ((apdu->lc == 0U) || (apdu->data == NULL)) {
		ndef_listener_send_sw(NFC_SW_WRONG_LENGTH);
		return;
	}

	offset = (uint32_t)(((uint32_t)apdu->p1 << 8U) | (uint32_t)apdu->p2);

	if ((offset + (uint32_t)apdu->lc) > NDEF_FILE_BUF_SIZE) {
		ndef_listener_send_sw(NFC_SW_WRONG_LENGTH);
		return;
	}

	(void)memcpy(&s_model.ndef_file[offset], apdu->data, apdu->lc);
#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
	nfc_stack_on_service_dirty(ndef_listener_get());
#endif
	ndef_listener_send_sw(NFC_SW_OK);
}

static void ndef_listener_on_apdu(const nfc_apdu_t *apdu, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (apdu == NULL) {
		return;
	}

	if (apdu->cla != 0x00U) {
		ndef_listener_send_sw(NFC_SW_CLA_NOT_SUPPORTED);
		return;
	}

	switch (apdu->ins) {
	case NFC_INS_SELECT:
		ndef_listener_handle_select_file(apdu);
		break;
	case NFC_INS_READ_BINARY:
		ndef_listener_handle_read_binary(apdu);
		break;
	case NFC_INS_UPDATE_BINARY:
		ndef_listener_handle_update_binary(apdu);
		break;
	default:
		ndef_listener_send_sw(NFC_SW_INS_NOT_SUPPORTED);
		break;
	}
}

static void ndef_listener_on_deselect(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	s_file_sel = NDEF_FILE_NONE;
}

static void ndef_listener_on_field_off(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	s_file_sel = NDEF_FILE_NONE;
}

static int ndef_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	ndef_listener_sync_file_len();
	return ndef_serialize(&s_model, out, out_max, out_len);
}

static int ndef_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	int ret;

	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	ret = ndef_deserialize(&s_model, in, in_len);
	if (ret != 0) {
		return ret;
	}

	ndef_listener_pad_ndef_file();
	return 0;
}

static const nfc_service_t s_ndef_listener_service = {
	.on_select = ndef_listener_on_select,
	.on_apdu = ndef_listener_on_apdu,
	.on_deselect = ndef_listener_on_deselect,
	.on_field_off = ndef_listener_on_field_off,
	.serialize = ndef_listener_serialize,
	.deserialize = ndef_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_NDEF,
	.user_ctx = NULL,
};

int ndef_listener_init(const ndef_data_t *model, const ndef_listener_config_t *cfg)
{
	ndef_listener_config_t defaults = NDEF_LISTENER_CONFIG_DEFAULT;

	if (s_initialized) {
		return -EALREADY;
	}

	if (cfg != NULL) {
		defaults = *cfg;
	}

	s_writable = defaults.writable;
	ndef_data_reset(&s_model);

	if (model != NULL) {
		s_model = *model;
	}

	if (s_model.cc_len == NFC_NDEF_CC_LEN) {
		s_model.cc[14] = s_writable ? NDEF_CC_WR_ACCESS_OPEN : NDEF_CC_WR_ACCESS_NONE;
	} else {
		ndef_listener_build_cc(s_model.cc, s_writable);
		s_model.cc_len = NFC_NDEF_CC_LEN;
	}

	ndef_listener_pad_ndef_file();
	s_file_sel = NDEF_FILE_NONE;
	s_initialized = true;
	return 0;
}

int ndef_listener_shutdown(void)
{
	if (!s_initialized) {
		return 0;
	}

	s_initialized = false;
	s_file_sel = NDEF_FILE_NONE;
	return 0;
}

const nfc_service_t *ndef_listener_get(void)
{
	return &s_ndef_listener_service;
}
