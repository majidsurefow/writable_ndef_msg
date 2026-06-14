/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier B — 4K variant poller goldens (isolated build for QEMU RAM).
 */

#include "protocols/classic/classic_poller.h"
#include "protocols/classic/classic_i.h"
#include "nfc_session_mock.h"

#include "MfClassic_4K_4b_mock.h"

#include <string.h>

#include <zephyr/ztest.h>

static classic_data_t s_data;
static nfc_reader_session_t s_session;

static const uint8_t s_uid_4k_4b[] = {0x04U, 0x4BU, 0x4BU, 0x4BU};

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	s_session.tag.valid = true;
	s_session.tag.uid.len = sizeof(s_uid_4k_4b);
	(void)memcpy(s_session.tag.uid.bytes, s_uid_4k_4b, sizeof(s_uid_4k_4b));
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

ZTEST(classic_poller_4k, test_read_golden)
{
	uint8_t exp_block0[CLASSIC_BLOCK_SIZE];

	classic_data_reset(&s_data);
	zassert_ok(classic_deserialize(&s_data, classic_MfClassic_4K_4b_model,
				       CLASSIC_MFCLASSIC_4K_4B_MODEL_LEN));
	(void)memcpy(exp_block0, s_data.blocks[0], sizeof(exp_block0));

	poller_before(NULL);
	nfc_session_mock_load(classic_MfClassic_4K_4b_read_steps,
			      CLASSIC_MFCLASSIC_4K_4B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(s_data.type, CLASSIC_TYPE_4K);
	zassert_equal(s_data.uid_len, sizeof(s_uid_4k_4b));
	zassert_mem_equal(s_data.uid, s_uid_4k_4b, sizeof(s_uid_4k_4b));
	zassert_mem_equal(s_data.blocks[0], exp_block0, sizeof(exp_block0));
}

ZTEST(classic_poller_4k, test_read_tx_sequence)
{
	poller_before(NULL);
	nfc_session_mock_load(classic_MfClassic_4K_4b_read_steps,
			      CLASSIC_MFCLASSIC_4K_4B_READ_STEP_COUNT);
	zassert_ok(classic_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_tx_count(), CLASSIC_MFCLASSIC_4K_4B_TX_STEP_COUNT);
	for (size_t i = 0U; i < CLASSIC_MFCLASSIC_4K_4B_TX_STEP_COUNT; i++) {
		assert_tx_equals(i, classic_MfClassic_4K_4b_tx_steps[i],
				 classic_MfClassic_4K_4b_tx_lens[i]);
	}
}

ZTEST_SUITE(classic_poller_4k, NULL, NULL, poller_before, NULL, NULL);
