/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Tier B — poller detect/read (cookbook §14.3 / §5.1).
 */

#include "ndef_fixture.h"
#include "ndef_poller.h"
#include "nfc_session_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static ndef_data_t s_data;
static nfc_reader_session_t s_session;

static const uint8_t TX_SELECT_V2[] = {
	0x00U, 0xA4U, 0x04U, 0x00U, 0x07U, 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x01U, 0x00U,
};

static const uint8_t TX_SELECT_V1[] = {
	0x00U, 0xA4U, 0x04U, 0x00U, 0x07U, 0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x00U,
};

static const uint8_t TX_SELECT_CC[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, 0xE1U, 0x03U};
static const uint8_t TX_READ_CC[] = {0x00U, 0xB0U, 0x00U, 0x00U, 0x0FU};
static const uint8_t TX_SELECT_NDEF[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, 0xE1U, 0x04U};
static const uint8_t TX_READ_NLEN[] = {0x00U, 0xB0U, 0x00U, 0x00U, 0x02U};
static const uint8_t TX_READ_URI[] = {0x00U, 0xB0U, 0x00U, 0x02U, 0x05U};
static const uint8_t TX_READ_CHUNK0[] = {0x00U, 0xB0U, 0x00U, 0x02U, 0xFDU};
static const uint8_t TX_READ_CHUNK1[] = {0x00U, 0xB0U, 0x00U, 0xFFU, 0x2FU};

static void *ndef_poller_setup(void)
{
	ndef_fixture_init_read_scripts();
	return NULL;
}

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	ndef_data_reset(&s_data);
}

static void assert_tx_equals(size_t index, const uint8_t *exp, size_t exp_len)
{
	const uint8_t *tx = NULL;
	size_t tx_len = 0U;

	zassert_ok(nfc_session_mock_get_tx(index, &tx, &tx_len));
	zassert_equal(tx_len, exp_len);
	zassert_mem_equal(tx, exp, exp_len);
}

ZTEST(ndef_poller, test_detect_v2_success)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_detect_v2, ARRAY_SIZE(ndef_fixture_detect_v2));
	zassert_ok(ndef_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
	assert_tx_equals(0U, TX_SELECT_V2, sizeof(TX_SELECT_V2));
}

ZTEST(ndef_poller, test_detect_v1_fallback)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_detect_v1_fallback,
			      ARRAY_SIZE(ndef_fixture_detect_v1_fallback));
	zassert_ok(ndef_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 2U);
	assert_tx_equals(0U, TX_SELECT_V2, sizeof(TX_SELECT_V2));
	assert_tx_equals(1U, TX_SELECT_V1, sizeof(TX_SELECT_V1));
}

ZTEST(ndef_poller, test_detect_enotsup)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_detect_enotsup, ARRAY_SIZE(ndef_fixture_detect_enotsup));
	zassert_equal(ndef_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(ndef_poller, test_detect_eio)
{
	static const nfc_session_mock_step_t script[] = {
		{.rx = NULL, .rx_len = 0U, .err = -EIO},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(ndef_poller_detect(&s_session), -EIO);
}

ZTEST(ndef_poller, test_detect_timeout)
{
	static const nfc_session_mock_step_t script[] = {
		{.rx = NULL, .rx_len = 0U, .err = -ETIMEDOUT},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(ndef_poller_detect(&s_session), -ETIMEDOUT);
}

ZTEST(ndef_poller, test_detect_inactive_session)
{
	poller_before(NULL);
	s_session.active = false;
	zassert_equal(ndef_poller_detect(&s_session), -EINVAL);
}

ZTEST(ndef_poller, test_read_empty)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_empty_steps, ARRAY_SIZE(ndef_fixture_read_empty_steps));
	zassert_ok(ndef_poller_read(&s_session, &s_data));
	zassert_equal(s_data.cc_len, NFC_NDEF_CC_LEN);
	zassert_mem_equal(s_data.cc, ndef_fixture_cc_std, NFC_NDEF_CC_LEN);
	zassert_equal(s_data.ndef_file_len, NFC_NDEF_NLEN_FIELD_LEN);
	zassert_equal(s_data.ndef_file[0], 0U);
	zassert_equal(s_data.ndef_file[1], 0U);
}

ZTEST(ndef_poller, test_read_uri_5byte)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_uri_steps, ARRAY_SIZE(ndef_fixture_read_uri_steps));
	zassert_ok(ndef_poller_read(&s_session, &s_data));
	zassert_equal(s_data.ndef_file_len,
		      (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + sizeof(ndef_fixture_uri_payload)));
	zassert_mem_equal(&s_data.ndef_file[2], ndef_fixture_uri_payload,
			  sizeof(ndef_fixture_uri_payload));
}

ZTEST(ndef_poller, test_read_chunk_transport_cap)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_chunk_steps, ARRAY_SIZE(ndef_fixture_read_chunk_steps));
	zassert_ok(ndef_poller_read(&s_session, &s_data));
	zassert_equal(s_data.ndef_file[0], 0x01U);
	zassert_equal(s_data.ndef_file[1], 0x2CU);
	zassert_equal(s_data.ndef_file_len, (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + NDEF_FIXTURE_CHUNK_NLEN));
	zassert_equal(s_data.ndef_file[2], 0x00U);
	zassert_equal(s_data.ndef_file[2U + 252U], (uint8_t)252U);
	zassert_equal(s_data.ndef_file[2U + 253U], (uint8_t)253U);
	zassert_equal(s_data.ndef_file[2U + 299U], (uint8_t)((299U) & 0xFFU));
	assert_tx_equals(5U, TX_READ_CHUNK0, sizeof(TX_READ_CHUNK0));
	assert_tx_equals(6U, TX_READ_CHUNK1, sizeof(TX_READ_CHUNK1));
}

ZTEST(ndef_poller, test_read_sw_error)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_sw_error_steps,
			      ARRAY_SIZE(ndef_fixture_read_sw_error_steps));
	zassert_equal(ndef_poller_read(&s_session, &s_data), -EIO);
}

ZTEST(ndef_poller, test_read_nlen_overflow)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_nlen_overflow_steps,
			      ARRAY_SIZE(ndef_fixture_read_nlen_overflow_steps));
	zassert_equal(ndef_poller_read(&s_session, &s_data), -ENOSPC);
}

ZTEST(ndef_poller, test_read_truncated)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_truncated_steps,
			      ARRAY_SIZE(ndef_fixture_read_truncated_steps));
	zassert_equal(ndef_poller_read(&s_session, &s_data), -EIO);
}

ZTEST(ndef_poller, test_read_tx_sequence)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_uri_steps, ARRAY_SIZE(ndef_fixture_read_uri_steps));
	zassert_ok(ndef_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_tx_count(), 6U);
	assert_tx_equals(0U, TX_SELECT_V2, sizeof(TX_SELECT_V2));
	assert_tx_equals(1U, TX_SELECT_CC, sizeof(TX_SELECT_CC));
	assert_tx_equals(2U, TX_READ_CC, sizeof(TX_READ_CC));
	assert_tx_equals(3U, TX_SELECT_NDEF, sizeof(TX_SELECT_NDEF));
	assert_tx_equals(4U, TX_READ_NLEN, sizeof(TX_READ_NLEN));
	assert_tx_equals(5U, TX_READ_URI, sizeof(TX_READ_URI));
}

ZTEST(ndef_poller, test_read_no_session_end)
{
	poller_before(NULL);

	nfc_session_mock_load(ndef_fixture_read_empty_steps, ARRAY_SIZE(ndef_fixture_read_empty_steps));
	zassert_ok(ndef_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_session_end_count(), 0U);
}

ZTEST(ndef_poller, test_read_inactive_session)
{
	poller_before(NULL);
	s_session.active = false;
	zassert_equal(ndef_poller_read(&s_session, &s_data), -EINVAL);
}

ZTEST_SUITE(ndef_poller, NULL, ndef_poller_setup, poller_before, NULL, NULL);
