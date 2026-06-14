/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier B — poller detect/read (cookbook §14.3 / §5.3).
 */

#include "protocols/classic/classic_poller.h"
#include "protocols/classic/classic_i.h"
#include "protocols/classic/crypto1.h"
#include "protocols/classic/iso14443_crc.h"
#include "nfc_session_mock.h"

#include "MfClassic_1K_4b_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static classic_data_t s_data;
static nfc_reader_session_t s_session;

static const uint8_t TX_DETECT_AUTH[] = {0x60U, 0x00U};
static const uint8_t TX_DETECT_4K_PROBE[] = {0x60U, 0xFEU};
static const uint8_t TX_DETECT_1K_PROBE[] = {0x60U, 0x3EU};
static const uint8_t TX_AUTH_NT_S0[] = {0x60U, 0x00U};
static const uint8_t TX_AUTH_NR_AR_S0[] = {
	0x91U, 0x07U, 0xBDU, 0x3AU, 0xFFU, 0xF9U, 0xF4U, 0x82U,
};
static const uint8_t TX_READ_BLOCK0[] = {0x2CU, 0x7AU, 0xFBU, 0xB1U};

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	s_session.tag.valid = true;
	s_session.tag.uid.len = 4U;
	s_session.tag.uid.bytes[0] = 0x04U;
	s_session.tag.uid.bytes[1] = 0xDEU;
	s_session.tag.uid.bytes[2] = 0xADU;
	s_session.tag.uid.bytes[3] = 0xBEU;
	classic_data_reset(&s_data);
}

static void assert_tx_equals(size_t index, const uint8_t *exp, size_t exp_len)
{
	const uint8_t *tx = NULL;
	size_t tx_len = 0U;

	zassert_ok(nfc_session_mock_get_tx(index, &tx, &tx_len));
	zassert_equal(tx_len, exp_len);
	zassert_mem_equal(tx, exp, exp_len);
}

ZTEST(classic_poller, test_detect_auth_probe)
{
	static const uint8_t nt[] = {0x10U, 0x00U, 0x00U, 0x00U};
	static const nfc_session_mock_step_t script[] = {
		{.rx = nt, .rx_len = sizeof(nt), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_ok(classic_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
	assert_tx_equals(0U, TX_DETECT_AUTH, sizeof(TX_DETECT_AUTH));
}

ZTEST(classic_poller, test_detect_enotsup_short_rx)
{
	static const uint8_t short_rx[] = {0x04U};
	static const nfc_session_mock_step_t script[] = {
		{.rx = short_rx, .rx_len = sizeof(short_rx), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(classic_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(classic_poller, test_crypto_decrypt_block0_golden)
{
	struct crypto1 crypto;
	uint8_t rx_plain[32];
	uint8_t nt[] = {0x10U, 0x00U, 0x00U, 0x00U};
	uint8_t nr[] = {0x01U, 0x02U, 0x03U, 0x04U};
	uint8_t tx_plain[4];
	size_t tx_len = 2U;
	uint32_t nt_num;
	uint32_t cuid = 0x04DEADBEU;

	nt_num = ((uint32_t)nt[0] << 24) | ((uint32_t)nt[1] << 16) |
		 ((uint32_t)nt[2] << 8) | (uint32_t)nt[3];
	crypto1_init(&crypto, CLASSIC_DEFAULT_KEY);
	crypto1_word(&crypto, nt_num ^ cuid, 0);
	for (size_t i = 0U; i < sizeof(nr); i++) {
		(void)crypto1_byte(&crypto, nr[i], 0);
	}
	nt_num = crypto1_prng_successor(nt_num, 32U);
	for (size_t i = 0U; i < 4U; i++) {
		nt_num = crypto1_prng_successor(nt_num, 8U);
		(void)crypto1_byte(&crypto, 0U, 0);
	}
	crypto1_word(&crypto, 0U, 0);

	tx_plain[0] = CLASSIC_CMD_READ_BLOCK;
	tx_plain[1] = 0U;
	iso14443_crc_append_a(tx_plain, &tx_len);
	crypto1_encrypt(&crypto, NULL, tx_plain, tx_len, rx_plain);

	crypto1_decrypt(&crypto, classic_MfClassic_1K_4b_step4_rx,
			  sizeof(classic_MfClassic_1K_4b_step4_rx), rx_plain);
	zassert_true(iso14443_crc_check_a(rx_plain, sizeof(classic_MfClassic_1K_4b_step4_rx)));
}

ZTEST(classic_poller, test_crypto_decrypt_block4_golden)
{
	struct crypto1 crypto;
	uint8_t rx_plain[32];
	uint8_t nt[] = {0x10U, 0x00U, 0x00U, 0x01U};
	uint8_t nr[] = {0x01U, 0x02U, 0x03U, 0x04U};
	uint8_t tx_plain[4];
	size_t tx_len = 2U;
	uint32_t nt_num;
	uint32_t cuid = 0x04DEADBEU;

	nt_num = ((uint32_t)nt[0] << 24) | ((uint32_t)nt[1] << 16) |
		 ((uint32_t)nt[2] << 8) | (uint32_t)nt[3];
	crypto1_init(&crypto, CLASSIC_DEFAULT_KEY);
	crypto1_word(&crypto, nt_num ^ cuid, 0);
	for (size_t i = 0U; i < sizeof(nr); i++) {
		(void)crypto1_byte(&crypto, nr[i], 0);
	}
	nt_num = crypto1_prng_successor(nt_num, 32U);
	for (size_t i = 0U; i < 4U; i++) {
		nt_num = crypto1_prng_successor(nt_num, 8U);
		(void)crypto1_byte(&crypto, 0U, 0);
	}
	crypto1_word(&crypto, 0U, 0);

	tx_plain[0] = CLASSIC_CMD_READ_BLOCK;
	tx_plain[1] = 4U;
	iso14443_crc_append_a(tx_plain, &tx_len);
	crypto1_encrypt(&crypto, NULL, tx_plain, tx_len, rx_plain);

	crypto1_decrypt(&crypto, classic_MfClassic_1K_4b_step10_rx,
			  sizeof(classic_MfClassic_1K_4b_step10_rx), rx_plain);
	zassert_true(iso14443_crc_check_a(rx_plain, sizeof(classic_MfClassic_1K_4b_step10_rx)));
}

ZTEST(classic_poller, test_read_1k_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(classic_MfClassic_1K_4b_read_steps,
			      CLASSIC_MFCLASSIC_1K_4B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(s_data.type, CLASSIC_TYPE_1K);
	zassert_equal(s_data.uid_len, 4U);
	zassert_mem_equal(s_data.uid, s_session.tag.uid.bytes, 4U);
	zassert_true(classic_is_block_read(&s_data, 0U));
	zassert_mem_equal(s_data.blocks[0], classic_MfClassic_1K_4b_model + 58U,
			  CLASSIC_BLOCK_SIZE);
}

ZTEST(classic_poller, test_read_tx_sequence)
{
	poller_before(NULL);
	nfc_session_mock_load(classic_MfClassic_1K_4b_read_steps,
			      CLASSIC_MFCLASSIC_1K_4B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_tx_count(), NFC_SESSION_MOCK_MAX_TX);
	assert_tx_equals(0U, TX_DETECT_4K_PROBE, sizeof(TX_DETECT_4K_PROBE));
	assert_tx_equals(1U, TX_DETECT_1K_PROBE, sizeof(TX_DETECT_1K_PROBE));
	assert_tx_equals(2U, TX_AUTH_NT_S0, sizeof(TX_AUTH_NT_S0));
	assert_tx_equals(3U, TX_AUTH_NR_AR_S0, sizeof(TX_AUTH_NR_AR_S0));
	assert_tx_equals(4U, TX_READ_BLOCK0, sizeof(TX_READ_BLOCK0));
}

ZTEST_SUITE(classic_poller, NULL, NULL, poller_before, NULL, NULL);
