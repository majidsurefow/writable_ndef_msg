/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight_poller_i.c
 */

#include "ultralight_poller_i.h"

#include "ultralight_3des.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#ifndef CONFIG_ZTEST
#include <zephyr/random/random.h>
#endif

#define UL_POLLER_I_TIMEOUT K_MSEC(5000)

#if defined(CONFIG_ZTEST)
static bool s_use_fixed_rnda;
static uint8_t s_fixed_rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];

void ultralight_poller_i_test_set_fixed_rnda(const uint8_t rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE])
{
	s_use_fixed_rnda = true;
	(void)memcpy(s_fixed_rnda, rnda, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
}
#endif

int ultralight_poller_i_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				   uint8_t *rx, size_t *rx_len)
{
	return nfc_reader_session_transceive(session, tx, tx_len, rx,
					     NFC_TRANSPORT_MAX_RESPONSE_LEN, rx_len,
					     UL_POLLER_I_TIMEOUT);
}

int ultralight_poller_i_read_page(nfc_reader_session_t *session, uint8_t page,
				  uint8_t out[ULTRALIGHT_READ_RESP_LEN])
{
	uint8_t tx[2] = {ULTRALIGHT_CMD_READ, page};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ultralight_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < ULTRALIGHT_READ_RESP_LEN) {
		return -EIO;
	}

	(void)memcpy(out, rx, ULTRALIGHT_READ_RESP_LEN);
	return 0;
}

int ultralight_poller_i_get_version(nfc_reader_session_t *session, uint8_t ver[8])
{
	uint8_t tx[1] = {ULTRALIGHT_CMD_GET_VERSION};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ultralight_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < ULTRALIGHT_VERSION_SIZE) {
		return -EIO;
	}

	(void)memcpy(ver, rx, ULTRALIGHT_VERSION_SIZE);
	return 0;
}

int ultralight_poller_i_read_signature(nfc_reader_session_t *session, uint8_t sig[32])
{
	uint8_t tx[2] = {ULTRALIGHT_CMD_READ_SIG, 0x00U};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ultralight_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < ULTRALIGHT_SIGNATURE_SIZE) {
		return -EIO;
	}

	(void)memcpy(sig, rx, ULTRALIGHT_SIGNATURE_SIZE);
	return 0;
}

int ultralight_poller_i_read_counter(nfc_reader_session_t *session, uint8_t index, uint8_t out[3])
{
	uint8_t tx[2] = {ULTRALIGHT_CMD_READ_CNT, index};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ultralight_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < 3U) {
		return -EIO;
	}

	(void)memcpy(out, rx, 3U);
	return 0;
}

int ultralight_poller_i_read_tearing(nfc_reader_session_t *session, uint8_t index, uint8_t *flag)
{
	uint8_t tx[2] = {ULTRALIGHT_CMD_CHECK_TEARING, index};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ultralight_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < 1U) {
		return -EIO;
	}

	*flag = rx[0];
	return 0;
}

int ultralight_poller_i_pwd_auth(nfc_reader_session_t *session, const uint8_t pwd[4])
{
	uint8_t tx[5] = {ULTRALIGHT_CMD_PWD_AUTH, pwd[0], pwd[1], pwd[2], pwd[3]};
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	int ret;

	ret = ultralight_poller_i_transceive(session, tx, sizeof(tx), rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len < ULTRALIGHT_PWD_AUTH_RESP_LEN) {
		return -EIO;
	}

	return 0;
}

static int ultralight_poller_i_send_auth_cmd(nfc_reader_session_t *session, const uint8_t *cmd,
					     size_t cmd_len, bool initial_cmd, uint8_t *response,
					     uint8_t full_rx[ULTRALIGHT_C_AUTH_RESPONSE_SIZE])
{
	uint8_t rx[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t rx_len = 0U;
	uint8_t expect_code;
	int ret;

	ret = ultralight_poller_i_transceive(session, cmd, cmd_len, rx, &rx_len);
	if (ret != 0) {
		return ret;
	}

	expect_code = initial_cmd ? ULTRALIGHT_CMD_AUTH_CONT : 0x00U;
	if ((rx_len != ULTRALIGHT_C_AUTH_RESPONSE_SIZE) || (rx[0] != expect_code)) {
		return -EACCES;
	}

	if (full_rx != NULL) {
		(void)memcpy(full_rx, rx, ULTRALIGHT_C_AUTH_RESPONSE_SIZE);
	}

	if (response != NULL) {
		(void)memcpy(response, &rx[1], ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
	}

	return 0;
}

int ultralight_poller_i_authentication_test(nfc_reader_session_t *session)
{
	uint8_t auth_cmd[2] = {ULTRALIGHT_CMD_AUTH, 0x00U};

	return ultralight_poller_i_send_auth_cmd(session, auth_cmd, sizeof(auth_cmd), true, NULL,
						 NULL);
}

static void ultralight_poller_i_fill_rnda(uint8_t rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE])
{
#if defined(CONFIG_ZTEST)
	if (s_use_fixed_rnda) {
		(void)memcpy(rnda, s_fixed_rnda, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
		return;
	}

	(void)memset(rnda, 0, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
#else
	(void)sys_rand_get(rnda, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
#endif
}

int ultralight_poller_i_authenticate(nfc_reader_session_t *session,
				     const uint8_t key[ULTRALIGHT_C_AUTH_DES_KEY_SIZE],
				     bool *authenticated)
{
	uint8_t auth_cmd[2] = {ULTRALIGHT_CMD_AUTH, 0x00U};
	uint8_t enc_rnd_b[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
	uint8_t output[ULTRALIGHT_C_AUTH_DATA_SIZE];
	uint8_t rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
	uint8_t decoded_shifted_rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE];
	uint8_t auth_rx[ULTRALIGHT_C_AUTH_RESPONSE_SIZE];
	uint8_t iv[ULTRALIGHT_C_AUTH_IV_BLOCK_SIZE] = {0};
	uint8_t af_cmd[ULTRALIGHT_C_ENCRYPTED_PACK_SIZE];
	uint8_t *rnd_b;
	int ret;

	if ((session == NULL) || (key == NULL) || (authenticated == NULL)) {
		return -EINVAL;
	}

	*authenticated = false;
	ultralight_poller_i_fill_rnda(rnda);

	ret = ultralight_poller_i_send_auth_cmd(session, auth_cmd, sizeof(auth_cmd), true, enc_rnd_b,
						NULL);
	if (ret != 0) {
		return ret;
	}

	rnd_b = &output[ULTRALIGHT_C_AUTH_RND_B_BLOCK_OFFSET];
	ultralight_3des_decrypt(key, iv, enc_rnd_b, sizeof(enc_rnd_b), rnd_b);
	ultralight_3des_shift_data(rnd_b);
	(void)memcpy(output, rnda, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE);
	ultralight_3des_encrypt(key, enc_rnd_b, output, ULTRALIGHT_C_AUTH_DATA_SIZE, output);

	af_cmd[0] = ULTRALIGHT_CMD_AUTH_CONT;
	(void)memcpy(&af_cmd[1], output, ULTRALIGHT_C_AUTH_DATA_SIZE);

	ret = ultralight_poller_i_send_auth_cmd(session, af_cmd, sizeof(af_cmd), false, NULL,
						auth_rx);
	if (ret != 0) {
		return ret;
	}

	ultralight_3des_decrypt(key, rnd_b, &auth_rx[1], ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE,
				decoded_shifted_rnda);
	ultralight_3des_shift_data(rnda);
	*authenticated =
		(memcmp(rnda, decoded_shifted_rnda, ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE) == 0);
	return 0;
}
