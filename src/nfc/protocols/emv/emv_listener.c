/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emv_listener.h"

#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#define EMV_INS_GPO              0xA8U
#define EMV_INS_READ_RECORD      0xB2U
#define EMV_CLA_GP               0x80U

static emv_card_image_t s_image;
static emv_session_state_t s_session;
static bool s_initialized;
static uint8_t s_ppse_fci[128];
static size_t s_ppse_fci_len;
static uint8_t s_app_fci[128];
static size_t s_app_fci_len;
static uint8_t s_gpo[16];
static size_t s_gpo_len;
static uint8_t s_resp[NFC_TRANSPORT_MAX_RESPONSE_LEN];

static void emv_listener_rebuild_wire(const emv_card_image_t *image)
{
	s_ppse_fci_len = 0U;
	s_ppse_fci[s_ppse_fci_len++] = 0x6FU;
	s_ppse_fci[s_ppse_fci_len++] = 0x1EU;
	s_ppse_fci[s_ppse_fci_len++] = 0x84U;
	s_ppse_fci[s_ppse_fci_len++] = EMV_PPSE_AID_LEN;
	(void)memcpy(&s_ppse_fci[s_ppse_fci_len], emv_ppse_aid, EMV_PPSE_AID_LEN);
	s_ppse_fci_len += EMV_PPSE_AID_LEN;
	s_ppse_fci[s_ppse_fci_len++] = 0xA5U;
	s_ppse_fci[s_ppse_fci_len++] = 0x10U;
	s_ppse_fci[s_ppse_fci_len++] = 0xBFU;
	s_ppse_fci[s_ppse_fci_len++] = 0x0CU;
	s_ppse_fci[s_ppse_fci_len++] = 0x0EU;
	s_ppse_fci[s_ppse_fci_len++] = 0x61U;
	s_ppse_fci[s_ppse_fci_len++] = 0x0CU;
	s_ppse_fci[s_ppse_fci_len++] = 0x4FU;
	s_ppse_fci[s_ppse_fci_len++] = image->app_aid_len;
	(void)memcpy(&s_ppse_fci[s_ppse_fci_len], image->app_aid, image->app_aid_len);
	s_ppse_fci_len += image->app_aid_len;
	s_ppse_fci[s_ppse_fci_len++] = 0x87U;
	s_ppse_fci[s_ppse_fci_len++] = 0x01U;
	s_ppse_fci[s_ppse_fci_len++] = 0x01U;

	s_app_fci_len = 0U;
	s_app_fci[s_app_fci_len++] = 0x6FU;
	s_app_fci[s_app_fci_len++] = 0x12U;
	s_app_fci[s_app_fci_len++] = 0x84U;
	s_app_fci[s_app_fci_len++] = image->app_aid_len;
	(void)memcpy(&s_app_fci[s_app_fci_len], image->app_aid, image->app_aid_len);
	s_app_fci_len += image->app_aid_len;
	s_app_fci[s_app_fci_len++] = 0xA5U;
	s_app_fci[s_app_fci_len++] = 0x07U;
	s_app_fci[s_app_fci_len++] = 0x50U;
	s_app_fci[s_app_fci_len++] = 0x04U;
	(void)memcpy(&s_app_fci[s_app_fci_len], "VISA", 4U);
	s_app_fci_len += 4U;
	s_app_fci[s_app_fci_len++] = 0x87U;
	s_app_fci[s_app_fci_len++] = 0x01U;
	s_app_fci[s_app_fci_len++] = 0x01U;

	s_gpo_len = 0U;
	s_gpo[s_gpo_len++] = EMV_TAG_RESP_TMPL_FMT1;
	s_gpo[s_gpo_len++] = 0x06U;
	s_gpo[s_gpo_len++] = image->aip[0];
	s_gpo[s_gpo_len++] = image->aip[1];
	(void)memcpy(&s_gpo[s_gpo_len], image->afl, EMV_AFL_BYTES);
	s_gpo_len += EMV_AFL_BYTES;
}

static void emv_listener_send(const uint8_t *data, size_t len, uint16_t sw)
{
	if ((len + 2U) > sizeof(s_resp)) {
		s_resp[0] = (uint8_t)(NFC_SW_CONDITIONS_NOT_SAT >> 8U);
		s_resp[1] = (uint8_t)(NFC_SW_CONDITIONS_NOT_SAT & 0xFFU);
		(void)nfc_transport_send_response(s_resp, 2U);
		return;
	}

	if (len > 0U) {
		(void)memcpy(s_resp, data, len);
	}
	s_resp[len] = (uint8_t)(sw >> 8U);
	s_resp[len + 1U] = (uint8_t)(sw & 0xFFU);
	(void)nfc_transport_send_response(s_resp, len + 2U);
}

static void emv_listener_on_select(const uint8_t *aid, size_t aid_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if ((aid_len == EMV_PPSE_AID_LEN) && (memcmp(aid, emv_ppse_aid, EMV_PPSE_AID_LEN) == 0)) {
		s_session = EMV_SESSION_PPSE_SELECTED;
		emv_listener_send(s_ppse_fci, s_ppse_fci_len, NFC_SW_OK);
		return;
	}

	if ((aid_len == s_image.app_aid_len) &&
	    (memcmp(aid, s_image.app_aid, aid_len) == 0)) {
		s_session = EMV_SESSION_APP_SELECTED;
		emv_listener_send(s_app_fci, s_app_fci_len, NFC_SW_OK);
		return;
	}

	s_session = EMV_SESSION_IDLE;
	emv_listener_send(NULL, 0U, NFC_SW_FILE_NOT_FOUND);
}

static void emv_listener_on_apdu(const nfc_apdu_t *apdu, void *user_ctx)
{
	uint8_t sfi;
	uint8_t rec_no;

	ARG_UNUSED(user_ctx);

	if (apdu == NULL) {
		emv_listener_send(NULL, 0U, NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	if ((apdu->cla == EMV_CLA_GP) && (apdu->ins == EMV_INS_GPO)) {
		if (s_session != EMV_SESSION_APP_SELECTED) {
			emv_listener_send(NULL, 0U, NFC_SW_CONDITIONS_NOT_SAT);
			return;
		}
		s_session = EMV_SESSION_GPO_DONE;
		emv_listener_send(s_gpo, s_gpo_len, NFC_SW_OK);
		return;
	}

	if ((apdu->cla == NFC_CLA_ISO7816) && (apdu->ins == EMV_INS_READ_RECORD)) {
		if (s_session != EMV_SESSION_GPO_DONE) {
			emv_listener_send(NULL, 0U, NFC_SW_CONDITIONS_NOT_SAT);
			return;
		}
		if ((apdu->p2 & 0x07U) != 0x04U) {
			emv_listener_send(NULL, 0U, NFC_SW_WRONG_P1P2);
			return;
		}
		sfi = (uint8_t)((apdu->p2 >> 3U) & 0x1FU);
		rec_no = apdu->p1;
		if ((sfi != (s_image.afl[0] >> 3U)) || (rec_no < 1U) || (rec_no > s_image.record_count)) {
			emv_listener_send(NULL, 0U, 0x6A83U);
			return;
		}
		emv_listener_send(s_image.record_data[rec_no - 1U], s_image.record_len[rec_no - 1U],
				  NFC_SW_OK);
		return;
	}

	emv_listener_send(NULL, 0U, NFC_SW_INS_NOT_SUPPORTED);
}

static void emv_listener_on_deselect(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	s_session = EMV_SESSION_IDLE;
}

static void emv_listener_on_field_off(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	s_session = EMV_SESSION_IDLE;
}

static int emv_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return emv_serialize(&s_image, out, out_max, out_len);
}

static int emv_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	int ret;

	ARG_UNUSED(user_ctx);

	ret = emv_deserialize(&s_image, in, in_len);
	if (ret != 0) {
		return ret;
	}

	emv_listener_rebuild_wire(&s_image);
	s_session = EMV_SESSION_IDLE;
	return 0;
}

static nfc_service_t s_service = {
	.on_select = emv_listener_on_select,
	.on_apdu = emv_listener_on_apdu,
	.on_deselect = emv_listener_on_deselect,
	.on_field_off = emv_listener_on_field_off,
	.serialize = emv_listener_serialize,
	.deserialize = emv_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_EMV,
};

int emv_listener_init(const emv_card_image_t *image)
{
	if (image != NULL) {
		s_image = *image;
	} else {
		emv_card_image_load_default(&s_image);
	}

	emv_listener_rebuild_wire(&s_image);
	s_session = EMV_SESSION_IDLE;
	s_initialized = true;
	return 0;
}

int emv_listener_shutdown(void)
{
	s_session = EMV_SESSION_IDLE;
	s_initialized = false;
	return 0;
}

const nfc_service_t *emv_listener_get(void)
{
	return &s_service;
}

emv_session_state_t emv_listener_session_state(void)
{
	return s_session;
}
