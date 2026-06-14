/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aliro_listener.h"

#include "framing/apdu_types.h"
#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

static aliro_data_t s_model;
static aliro_state_t s_state;
static atomic_t s_crypto_inflight;
static struct k_work s_crypto_work;
static uint8_t s_resp[NFC_TRANSPORT_MAX_RESPONSE_LEN];

static void aliro_listener_send_sw(uint16_t sw)
{
	s_resp[0] = (uint8_t)(sw >> 8U);
	s_resp[1] = (uint8_t)(sw & 0xFFU);
	(void)nfc_transport_send_response(s_resp, 2U);
}

static void aliro_listener_reset(void)
{
	s_state = ALIRO_STATE_IDLE;
	(void)atomic_set(&s_crypto_inflight, 0);
}

static void aliro_listener_crypto_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (s_state != ALIRO_STATE_AWAIT_AUTH0) {
		s_state = ALIRO_STATE_ERROR;
		aliro_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		(void)atomic_set(&s_crypto_inflight, 0);
		return;
	}

	s_resp[0] = 0x00U;
	s_resp[1] = 0x00U;
	(void)memset(&s_resp[2], 0xAAU, 16U);
	s_resp[18] = 0x04U;
	(void)memset(&s_resp[19], 0xBBU, ALIRO_P256_PUBLIC_KEY_SIZE);
	(void)nfc_transport_send_response(s_resp, 19U + ALIRO_P256_PUBLIC_KEY_SIZE);
	s_state = ALIRO_STATE_AWAIT_AUTH1;
	(void)atomic_set(&s_crypto_inflight, 0);
}

static void aliro_listener_on_select(const uint8_t *aid, size_t aid_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	aliro_listener_reset();

	if ((aid_len == ALIRO_AID_LEN) && (memcmp(aid, aliro_expedited_aid, ALIRO_AID_LEN) == 0)) {
		s_state = ALIRO_STATE_AWAIT_AUTH0;
		s_resp[0] = 0x00U;
		s_resp[1] = 0x00U;
		(void)memset(&s_resp[2], 0x11U, ALIRO_NONCE_SIZE);
		(void)nfc_transport_send_response(s_resp, 2U + ALIRO_NONCE_SIZE);
		return;
	}

	if ((aid_len == ALIRO_AID_LEN) && (memcmp(aid, aliro_stepup_aid, ALIRO_AID_LEN) == 0)) {
		aliro_listener_send_sw(0x6A81U);
		return;
	}

	aliro_listener_send_sw(NFC_SW_FILE_NOT_FOUND);
}

static void aliro_listener_on_apdu(const nfc_apdu_t *apdu, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if ((apdu == NULL) || (s_state == ALIRO_STATE_ERROR) || (s_state == ALIRO_STATE_DONE)) {
		aliro_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	if (apdu->cla != ALIRO_CLA_PROP) {
		aliro_listener_send_sw(NFC_SW_CLA_NOT_SUPPORTED);
		return;
	}

	if (atomic_get(&s_crypto_inflight) != 0) {
		s_state = ALIRO_STATE_ERROR;
		aliro_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	if ((apdu->ins == ALIRO_INS_AUTH0) && (s_state == ALIRO_STATE_AWAIT_AUTH0)) {
		(void)atomic_set(&s_crypto_inflight, 1);
		(void)nfc_transport_submit_work(&s_crypto_work);
		return;
	}

	if ((apdu->ins == ALIRO_INS_AUTH1) && (s_state != ALIRO_STATE_AWAIT_AUTH1)) {
		aliro_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	if ((apdu->ins == ALIRO_INS_EXCHANGE) && (s_state != ALIRO_STATE_AWAIT_EXCHANGE)) {
		aliro_listener_send_sw(NFC_SW_CONDITIONS_NOT_SAT);
		return;
	}

	aliro_listener_send_sw(NFC_SW_INS_NOT_SUPPORTED);
}

static void aliro_listener_on_deselect(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	aliro_listener_reset();
}

static void aliro_listener_on_field_off(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	aliro_listener_reset();
}

static int aliro_listener_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return aliro_serialize(&s_model, out, out_max, out_len);
}

static int aliro_listener_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	aliro_listener_reset();
	return aliro_deserialize(&s_model, in, in_len);
}

static nfc_service_t s_service = {
	.on_select = aliro_listener_on_select,
	.on_apdu = aliro_listener_on_apdu,
	.on_deselect = aliro_listener_on_deselect,
	.on_field_off = aliro_listener_on_field_off,
	.serialize = aliro_listener_serialize,
	.deserialize = aliro_listener_deserialize,
	.persist_id = NFC_PERSIST_ID_ALIRO,
};

int aliro_listener_init(const aliro_data_t *model)
{
	k_work_init(&s_crypto_work, aliro_listener_crypto_handler);

	if (model != NULL) {
		s_model = *model;
	} else {
		aliro_data_reset(&s_model);
		s_model.protocol_major = 0x00U;
		s_model.protocol_minor = 0x09U;
		s_model.credential_pubkey[0] = 0x04U;
	}

	aliro_listener_reset();
	return 0;
}

int aliro_listener_shutdown(void)
{
	aliro_listener_reset();
	return 0;
}

const nfc_service_t *aliro_listener_get(void)
{
	return &s_service;
}

aliro_state_t aliro_listener_state(void)
{
	return s_state;
}

bool aliro_listener_crypto_inflight(void)
{
	return atomic_get(&s_crypto_inflight) != 0;
}
