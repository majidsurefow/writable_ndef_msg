/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ultralight Tier B — poller detect/read (cookbook §14.3 / §5.2).
 */

#include "ultralight_fixture.h"
#include "ultralight_poller.h"
#include "ultralight_poller_i.h"
#include "nfc_session_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static ultralight_data_t s_data;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	static const uint8_t fixed_rnda[ULTRALIGHT_C_AUTH_RND_BLOCK_SIZE] = {
		0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
	};

	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	ultralight_data_reset(&s_data);
	ultralight_poller_i_test_set_fixed_rnda(fixed_rnda);
}

static void assert_tx_equals(size_t index, const uint8_t *exp, size_t exp_len)
{
	const uint8_t *tx = NULL;
	size_t tx_len = 0U;

	zassert_ok(nfc_session_mock_get_tx(index, &tx, &tx_len), "tx step %zu missing", index);
	zassert_equal(tx_len, exp_len, "tx step %zu len", index);
	zassert_mem_equal(tx, exp, exp_len, "tx step %zu payload", index);
}

static void assert_read_tx_golden(const uint8_t *const *tx_steps, const size_t *tx_lens,
				  size_t tx_count)
{
	size_t step;

	zassert_equal(nfc_session_mock_tx_count(), tx_count);
	for (step = 0U; step < tx_count; step++) {
		assert_tx_equals(step, tx_steps[step], tx_lens[step]);
	}
}

ZTEST(ultralight_poller, test_detect_ul11)
{
	poller_before(NULL);
	ultralight_fixture_load_detect(ultralight_Ultralight_11_read_steps,
					 ULTRALIGHT_DETECT_STEP_INDEX_UL11);
	zassert_ok(ultralight_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
}

ZTEST(ultralight_poller, test_detect_enotsup_short_rx)
{
	static const uint8_t short_rx[] = {0x04U};
	static const nfc_session_mock_step_t script[] = {
		{.rx = short_rx, .rx_len = sizeof(short_rx), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(ultralight_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(ultralight_poller, test_read_ul11_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ultralight_11_read_steps,
			      ULTRALIGHT_ULTRALIGHT_11_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 20U);
	zassert_equal(s_data.pages_read, 20U);
	zassert_equal(s_data.type, UL_TYPE_UL11);
	zassert_mem_equal(s_data.pages[0], ultralight_Ultralight_11_model + 4U, 4U);
	assert_read_tx_golden(ultralight_Ultralight_11_read_tx_steps,
			      ultralight_Ultralight_11_read_tx_lens,
			      ULTRALIGHT_ULTRALIGHT_11_READ_TX_STEP_COUNT);
}

ZTEST(ultralight_poller, test_read_ul21_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ultralight_21_read_steps,
			      ULTRALIGHT_ULTRALIGHT_21_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 41U);
	zassert_equal(s_data.type, UL_TYPE_UL21);
	assert_read_tx_golden(ultralight_Ultralight_21_read_tx_steps,
			      ultralight_Ultralight_21_read_tx_lens,
			      ULTRALIGHT_ULTRALIGHT_21_READ_TX_STEP_COUNT);
}

ZTEST(ultralight_poller, test_read_ultralight_c_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ultralight_C_read_steps,
			      ULTRALIGHT_ULTRALIGHT_C_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 48U);
	zassert_equal(s_data.type, UL_TYPE_MFUL_C);
	assert_read_tx_golden(ultralight_Ultralight_C_read_tx_steps,
			      ultralight_Ultralight_C_read_tx_lens,
			      ULTRALIGHT_ULTRALIGHT_C_READ_TX_STEP_COUNT);
}

ZTEST(ultralight_poller, test_read_ntag213_locked_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ntag213_locked_read_steps,
			      ULTRALIGHT_NTAG213_LOCKED_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 45U);
	zassert_equal(s_data.pages_read, 4U);
	zassert_equal(s_data.type, UL_TYPE_NTAG213);
	assert_read_tx_golden(ultralight_Ntag213_locked_read_tx_steps,
			      ultralight_Ntag213_locked_read_tx_lens,
			      ULTRALIGHT_NTAG213_LOCKED_READ_TX_STEP_COUNT);
}

ZTEST(ultralight_poller, test_read_ntag215_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ntag215_read_steps,
			      ULTRALIGHT_NTAG215_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 135U);
	zassert_equal(s_data.type, UL_TYPE_NTAG215);
	assert_read_tx_golden(ultralight_Ntag215_read_tx_steps, ultralight_Ntag215_read_tx_lens,
			      ULTRALIGHT_NTAG215_READ_TX_STEP_COUNT);
}

ZTEST(ultralight_poller, test_read_ntag216_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ntag216_read_steps,
			      ULTRALIGHT_NTAG216_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 231U);
	zassert_equal(s_data.type, UL_TYPE_NTAG216);
	assert_read_tx_golden(ultralight_Ntag216_read_tx_steps, ultralight_Ntag216_read_tx_lens,
			      ULTRALIGHT_NTAG216_READ_TX_STEP_COUNT);
}

ZTEST(ultralight_poller, test_detect_inactive_session)
{
	poller_before(NULL);
	s_session.active = false;
	zassert_equal(ultralight_poller_detect(&s_session), -EINVAL);
}

ZTEST(ultralight_poller, test_read_null_out)
{
	poller_before(NULL);
	zassert_equal(ultralight_poller_read(&s_session, NULL), -EINVAL);
}

ZTEST_SUITE(ultralight_poller, NULL, NULL, poller_before, NULL, NULL);
