/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier B — 1K 7b / Mini variant poller goldens (split for QEMU RAM).
 */

#include "protocols/classic/classic_poller.h"
#include "protocols/classic/classic_i.h"
#include "nfc_session_mock.h"

#include "MfClassic_1K_7b_mock.h"
#include "MfClassic_Mini_4b_mock.h"

#include <string.h>

#include <zephyr/ztest.h>

static classic_data_t s_data;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	s_session.tag.valid = true;
	classic_data_reset(&s_data);
}

static void assert_tx_equals(size_t index, const uint8_t *exp, size_t exp_len)
{
	const uint8_t *tx = NULL;
	size_t tx_len = 0U;

	zassert_ok(nfc_session_mock_get_tx(index, &tx, &tx_len), "tx index %zu missing", index);
	zassert_equal(tx_len, exp_len, "tx index %zu len", index);
	zassert_mem_equal(tx, exp, exp_len, "tx index %zu mismatch", index);
}

#if defined(CONFIG_NFC_CLASSIC_TEST_VARIANT_1K_7B)
static const uint8_t s_uid_1k_7b[] = {0x04U, 0xDEU, 0xADU, 0xBEU, 0xEFU, 0xCAU, 0xFEU};

ZTEST(classic_poller_1k_7b, test_read_golden)
{
	uint8_t exp_block0[CLASSIC_BLOCK_SIZE];

	classic_data_reset(&s_data);
	zassert_ok(classic_deserialize(&s_data, classic_MfClassic_1K_7b_model,
				       CLASSIC_MFCLASSIC_1K_7B_MODEL_LEN));
	(void)memcpy(exp_block0, s_data.blocks[0], sizeof(exp_block0));

	poller_before(NULL);
	s_session.tag.uid.len = sizeof(s_uid_1k_7b);
	(void)memcpy(s_session.tag.uid.bytes, s_uid_1k_7b, sizeof(s_uid_1k_7b));
	nfc_session_mock_load(classic_MfClassic_1K_7b_read_steps,
			      CLASSIC_MFCLASSIC_1K_7B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(s_data.type, CLASSIC_TYPE_1K);
	zassert_equal(s_data.uid_len, sizeof(s_uid_1k_7b));
	zassert_mem_equal(s_data.uid, s_uid_1k_7b, sizeof(s_uid_1k_7b));
	zassert_mem_equal(s_data.blocks[0], exp_block0, sizeof(exp_block0));
}

ZTEST(classic_poller_1k_7b, test_read_tx_sequence)
{
	poller_before(NULL);
	s_session.tag.uid.len = sizeof(s_uid_1k_7b);
	(void)memcpy(s_session.tag.uid.bytes, s_uid_1k_7b, sizeof(s_uid_1k_7b));
	nfc_session_mock_load(classic_MfClassic_1K_7b_read_steps,
			      CLASSIC_MFCLASSIC_1K_7B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_tx_count(), CLASSIC_MFCLASSIC_1K_7B_TX_STEP_COUNT);
	for (size_t i = 0U; i < CLASSIC_MFCLASSIC_1K_7B_TX_STEP_COUNT; i++) {
		assert_tx_equals(i, classic_MfClassic_1K_7b_tx_steps[i],
				 classic_MfClassic_1K_7b_tx_lens[i]);
	}
}

ZTEST_SUITE(classic_poller_1k_7b, NULL, NULL, poller_before, NULL, NULL);
#endif /* CONFIG_NFC_CLASSIC_TEST_VARIANT_1K_7B */

#if defined(CONFIG_NFC_CLASSIC_TEST_VARIANT_MINI_4B)
static const uint8_t s_uid_mini_4b[] = {0x04U, 0xDEU, 0xADU, 0xCAU};

ZTEST(classic_poller_mini, test_read_golden)
{
	uint8_t exp_block0[CLASSIC_BLOCK_SIZE];

	classic_data_reset(&s_data);
	zassert_ok(classic_deserialize(&s_data, classic_MfClassic_Mini_4b_model,
				       CLASSIC_MFCLASSIC_MINI_4B_MODEL_LEN));
	(void)memcpy(exp_block0, s_data.blocks[0], sizeof(exp_block0));

	poller_before(NULL);
	s_session.tag.uid.len = sizeof(s_uid_mini_4b);
	(void)memcpy(s_session.tag.uid.bytes, s_uid_mini_4b, sizeof(s_uid_mini_4b));
	nfc_session_mock_load(classic_MfClassic_Mini_4b_read_steps,
			      CLASSIC_MFCLASSIC_MINI_4B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(s_data.type, CLASSIC_TYPE_MINI);
	zassert_equal(s_data.uid_len, sizeof(s_uid_mini_4b));
	zassert_mem_equal(s_data.uid, s_uid_mini_4b, sizeof(s_uid_mini_4b));
	zassert_mem_equal(s_data.blocks[0], exp_block0, sizeof(exp_block0));
}

ZTEST(classic_poller_mini, test_read_tx_sequence)
{
	poller_before(NULL);
	s_session.tag.uid.len = sizeof(s_uid_mini_4b);
	(void)memcpy(s_session.tag.uid.bytes, s_uid_mini_4b, sizeof(s_uid_mini_4b));
	nfc_session_mock_load(classic_MfClassic_Mini_4b_read_steps,
			      CLASSIC_MFCLASSIC_MINI_4B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_tx_count(), CLASSIC_MFCLASSIC_MINI_4B_TX_STEP_COUNT);
	for (size_t i = 0U; i < CLASSIC_MFCLASSIC_MINI_4B_TX_STEP_COUNT; i++) {
		assert_tx_equals(i, classic_MfClassic_Mini_4b_tx_steps[i],
				 classic_MfClassic_Mini_4b_tx_lens[i]);
	}
}

ZTEST_SUITE(classic_poller_mini, NULL, NULL, poller_before, NULL, NULL);
#endif /* CONFIG_NFC_CLASSIC_TEST_VARIANT_MINI_4B */
