/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Ultralight Tier B — poller detect/read (cookbook §14.3 / §5.2).
 */

#include "ultralight_fixture.h"
#include "ultralight_poller.h"
#include "nfc_session_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static ultralight_data_t s_data;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	ultralight_data_reset(&s_data);
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
}

ZTEST(ultralight_poller, test_read_ul21_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ultralight_21_read_steps,
			      ULTRALIGHT_ULTRALIGHT_21_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 41U);
	zassert_equal(s_data.type, UL_TYPE_UL21);
}

ZTEST(ultralight_poller, test_read_ultralight_c_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ultralight_C_read_steps,
			      ULTRALIGHT_ULTRALIGHT_C_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 48U);
	zassert_equal(s_data.type, UL_TYPE_MFUL_C);
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
}

ZTEST(ultralight_poller, test_read_ntag215_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ntag215_read_steps,
			      ULTRALIGHT_NTAG215_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 135U);
	zassert_equal(s_data.type, UL_TYPE_NTAG215);
}

ZTEST(ultralight_poller, test_read_ntag216_golden)
{
	poller_before(NULL);
	nfc_session_mock_load(ultralight_Ntag216_read_steps,
			      ULTRALIGHT_NTAG216_READ_STEP_COUNT);
	zassert_ok(ultralight_poller_read(&s_session, &s_data));
	zassert_equal(s_data.pages_total, 231U);
	zassert_equal(s_data.type, UL_TYPE_NTAG216);
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
