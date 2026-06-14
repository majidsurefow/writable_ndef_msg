/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_listener.c
 */

#include "ultralight_listener_t2.h"

#include "ultralight_3des.h"

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#ifndef CONFIG_ZTEST
#include <zephyr/random/random.h>
#endif

typedef enum {
	UL_T2_AUTH_IDLE = 0,
	UL_T2_AUTH_WAIT_CONT,
	UL_T2_AUTH_SUCCESS,
} ultralight_t2_auth_state_t;

static ultralight_data_t s_model;
static ultralight_config_t s_cfg;
static bool s_initialized;
static ultralight_t2_auth_state_t s_auth_state;
static uint8_t s_rnd_b[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
static uint8_t s_enc_b[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
static uint8_t s_resp[NFC_TRANSPORT_MAX_RESPONSE_LEN];

static bool ultralight_t2_type_supports_auth(ultralight_type_t type)
{
	return type == UL_TYPE_MFUL_C;
}

static bool ultralight_t2_type_supports_pwd(ultralight_type_t type)
{
	return ultralight_type_has_password_config(type);
}

static bool ultralight_t2_page_readable(uint16_t page)
{
	if (page >= s_model.pages_total) {
		return false;
	}

	if (ultralight_t2_type_supports_pwd(s_model.type)) {
		if ((s_auth_state != UL_T2_AUTH_SUCCESS) && s_cfg.valid &&
		    ultralight_page_needs_auth(&s_cfg, page)) {
			return false;
		}
	}

	return true;
}

static void ultralight_t2_send(const uint8_t *data, size_t len)
{
	(void)nfc_transport_send_response(data, len);
}

static void ultralight_t2_handle_read(const uint8_t *tx, size_t tx_len)
{
	uint8_t start_page;
	uint8_t out[ULTRALIGHT_READ_RESP_LEN];

	if (tx_len < 2U) {
		return;
	}

	start_page = tx[1];

	(void)memset(out, 0, sizeof(out));

	for (uint8_t i = 0U; i < 4U; i++) {
		uint16_t page = (uint16_t)(start_page + i);

		if (page >= s_model.pages_total) {
			continue;
		}

		if (!ultralight_t2_page_readable(page)) {
			s_resp[0] = 0x00U;
			ultralight_t2_send(s_resp, 1U);
			return;
		}

		(void)memcpy(&out[i * ULTRALIGHT_PAGE_SIZE], s_model.pages[page],
			     ULTRALIGHT_PAGE_SIZE);
	}

	ultralight_t2_send(out, sizeof(out));
}

static void ultralight_t2_handle_get_version(void)
{
	if (s_model.has_version) {
		ultralight_t2_send(s_model.version, ULTRALIGHT_VERSION_SIZE);
	}
}

static void ultralight_t2_handle_read_sig(void)
{
	if (s_model.has_signature) {
		ultralight_t2_send(s_model.signature, ULTRALIGHT_SIGNATURE_SIZE);
	}
}

static void ultralight_t2_handle_read_cnt(const uint8_t *tx, size_t tx_len)
{
	if ((tx_len < 2U) || (tx[1] >= s_model.counter_count)) {
		return;
	}

	ultralight_t2_send(s_model.counters[tx[1]], 3U);
}

static void ultralight_t2_handle_tearing(const uint8_t *tx, size_t tx_len)
{
	if ((tx_len < 2U) || (tx[1] >= s_model.tearing_flag_count)) {
		return;
	}

	s_resp[0] = s_model.tearing_flags[tx[1]];
	ultralight_t2_send(s_resp, 1U);
}

static void ultralight_t2_handle_pwd_auth(const uint8_t *tx, size_t tx_len)
{
	if (!ultralight_t2_type_supports_pwd(s_model.type) || (tx_len < 5U)) {
		return;
	}

	if (!s_cfg.valid) {
		return;
	}

	if (memcmp(&tx[1], s_cfg.pwd, ULTRALIGHT_PAGE_SIZE) != 0) {
		return;
	}

	s_auth_state = UL_T2_AUTH_SUCCESS;
	ultralight_t2_send(s_cfg.pack, ULTRALIGHT_PWD_AUTH_RESP_LEN);
}

#if defined(CONFIG_ZTEST)
static const uint8_t s_test_rnd_b[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE] = {
	0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U,
};
#endif

static void ultralight_t2_fill_rnd_b(void)
{
#if defined(CONFIG_ZTEST)
	(void)memcpy(s_rnd_b, s_test_rnd_b, sizeof(s_rnd_b));
#else
	(void)sys_rand_get(s_rnd_b, sizeof(s_rnd_b));
#endif
}

static void ultralight_t2_handle_auth_start(void)
{
	const uint8_t *key;
	uint8_t iv[ULTRALIGHT_C_AUTH_IV_BLOCK_SIZE] = {0};

	if (!ultralight_t2_type_supports_auth(s_model.type)) {
		return;
	}

	key = ultralight_3des_get_key(&s_model);
	ultralight_t2_fill_rnd_b();
	ultralight_3des_encrypt(key, iv, s_rnd_b, sizeof(s_rnd_b), s_enc_b);

	s_resp[0] = ULTRALIGHT_CMD_AUTH_CONT;
	(void)memcpy(&s_resp[1], s_enc_b, sizeof(s_enc_b));
	ultralight_t2_send(s_resp, ULTRALIGHT_C_AUTH_RESPONSE_SIZE);
	s_auth_state = UL_T2_AUTH_WAIT_CONT;
}

static void ultralight_t2_handle_auth_cont(const uint8_t *tx, size_t tx_len)
{
	const uint8_t *key;
	uint8_t plain[ULTRALIGHT_C_AUTH_DATA_SIZE];
	uint8_t shifted_a[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
	uint8_t shifted_b[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
	const uint8_t *rnd_a;
	const uint8_t *decoded_shifted_b;

	if (!ultralight_t2_type_supports_auth(s_model.type) ||
	    (tx_len != ULTRALIGHT_C_ENCRYPTED_PACK_SIZE)) {
		return;
	}

	key = ultralight_3des_get_key(&s_model);
	ultralight_3des_decrypt(key, s_enc_b, &tx[1], sizeof(plain), plain);

	rnd_a = plain;
	decoded_shifted_b = &plain[ULTRALIGHT_C_AUTH_RND_B_BLOCK_OFFSET];
	(void)memcpy(shifted_a, rnd_a, sizeof(shifted_a));
	(void)memcpy(shifted_b, s_rnd_b, sizeof(shifted_b));
	ultralight_3des_shift_data(shifted_a);
	ultralight_3des_shift_data(shifted_b);

	if (memcmp(decoded_shifted_b, shifted_b, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE) != 0) {
		s_auth_state = UL_T2_AUTH_IDLE;
		return;
	}

	s_auth_state = UL_T2_AUTH_SUCCESS;
	ultralight_3des_encrypt(key, &tx[1 + ULTRALIGHT_C_AUTH_RND_B_BLOCK_OFFSET], shifted_a,
				ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE, shifted_a);
	s_resp[0] = 0x00U;
	(void)memcpy(&s_resp[1], shifted_a, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
	ultralight_t2_send(s_resp, ULTRALIGHT_C_AUTH_RESPONSE_SIZE);
}

static void ultralight_listener_t2_on_tag_cmd(const uint8_t *tx, size_t tx_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if ((tx == NULL) || (tx_len == 0U) || !s_initialized) {
		return;
	}

	switch (tx[0]) {
	case ULTRALIGHT_CMD_READ:
		ultralight_t2_handle_read(tx, tx_len);
		break;
	case ULTRALIGHT_CMD_GET_VERSION:
		ultralight_t2_handle_get_version();
		break;
	case ULTRALIGHT_CMD_READ_SIG:
		ultralight_t2_handle_read_sig();
		break;
	case ULTRALIGHT_CMD_READ_CNT:
		ultralight_t2_handle_read_cnt(tx, tx_len);
		break;
	case ULTRALIGHT_CMD_CHECK_TEARING:
		ultralight_t2_handle_tearing(tx, tx_len);
		break;
	case ULTRALIGHT_CMD_PWD_AUTH:
		ultralight_t2_handle_pwd_auth(tx, tx_len);
		break;
	case ULTRALIGHT_CMD_AUTH:
		ultralight_t2_handle_auth_start();
		break;
	case ULTRALIGHT_CMD_AUTH_CONT:
		if (s_auth_state == UL_T2_AUTH_WAIT_CONT) {
			ultralight_t2_handle_auth_cont(tx, tx_len);
		}
		break;
	default:
		break;
	}
}

static int ultralight_listener_t2_serialize(uint8_t *out, size_t out_max, size_t *out_len,
					    void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	return ultralight_serialize(&s_model, out, out_max, out_len);
}

static int ultralight_listener_t2_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	int ret;

	ARG_UNUSED(user_ctx);

	if (!s_initialized) {
		return -ENODEV;
	}

	ret = ultralight_deserialize(&s_model, in, in_len);
	if (ret != 0) {
		return ret;
	}

	(void)memset(&s_cfg, 0, sizeof(s_cfg));
	(void)ultralight_parse_config(&s_model, &s_cfg);
	s_auth_state = UL_T2_AUTH_IDLE;
	if (ultralight_t2_type_supports_pwd(s_model.type) &&
	    (s_model.pages_read >= s_model.pages_total) && s_cfg.valid &&
	    (!ultralight_read_protection_enabled(&s_cfg) || !s_cfg.prot)) {
		s_auth_state = UL_T2_AUTH_SUCCESS;
	}
	return 0;
}

static const nfc_service_t s_ultralight_listener_t2_service = {
	.on_tag_cmd = ultralight_listener_t2_on_tag_cmd,
	.serialize = ultralight_listener_t2_serialize,
	.deserialize = ultralight_listener_t2_deserialize,
	.persist_id = NFC_PERSIST_ID_ULTRALIGHT,
	.user_ctx = NULL,
};

int ultralight_listener_t2_init(void)
{
	if (s_initialized) {
		return -EALREADY;
	}

	ultralight_data_reset(&s_model);
	s_auth_state = UL_T2_AUTH_IDLE;
	s_initialized = true;
	return 0;
}

int ultralight_listener_t2_shutdown(void)
{
	if (!s_initialized) {
		return 0;
	}

	ultralight_data_reset(&s_model);
	s_auth_state = UL_T2_AUTH_IDLE;
	s_initialized = false;
	return 0;
}

const nfc_service_t *ultralight_listener_t2_get(void)
{
	return &s_ultralight_listener_t2_service;
}

