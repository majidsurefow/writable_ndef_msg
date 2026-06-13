/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Tier C — listener READ BINARY (wave5-ndef §8 Task 6).
 */

#include "listener_test_helpers.h"

#include "ndef_fixture.h"

#include "hal/nfc_transport.h"

#include <string.h>

#include <zephyr/ztest.h>

static ndef_data_t s_read_model;

static void *listener_read_setup(void)
{
	ndef_listener_config_t cfg = { .writable = true };

	ndef_fixture_init_read_scripts();

	ndef_data_reset(&s_read_model);
	(void)memcpy(s_read_model.cc, ndef_fixture_cc_std, NFC_NDEF_CC_LEN);
	s_read_model.cc_len = NFC_NDEF_CC_LEN;
	s_read_model.ndef_file[0] = 0U;
	s_read_model.ndef_file[1] = 0U;
	s_read_model.ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;

	nfc_response_spy_reset();
	ndef_listener_shutdown();
	zassert_ok(ndef_listener_init(&s_read_model, &cfg));
	return NULL;
}

ZTEST(ndef_listener_read, test_read_cc_full)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_select_cc();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x0FU}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, NFC_NDEF_CC_LEN + 2U);
	zassert_mem_equal(buf, ndef_fixture_cc_std, NFC_NDEF_CC_LEN);
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_read, test_read_cc_partial)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_select_cc();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x02U, 0x03U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, 5U);
	zassert_equal(buf[0], ndef_fixture_cc_std[2]);
	zassert_equal(buf[1], ndef_fixture_cc_std[3]);
	zassert_equal(buf[2], ndef_fixture_cc_std[4]);
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_read, test_read_ndef_empty)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;
	size_t expect_data = (size_t)NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U;

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x00U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, expect_data + 2U);
	for (size_t i = 0U; i < expect_data; i++) {
		zassert_equal(buf[i], 0U);
	}
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_read, test_read_nofile)
{
	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x0FU}, 5U);
	listener_assert_sw_u16(NFC_SW_NO_EF);
}

ZTEST(ndef_listener_read, test_read_offset_beyond_eof)
{
	listener_select_cc();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, NFC_NDEF_CC_LEN, 0x01U}, 5U);
	listener_assert_sw_u16(NFC_SW_WRONG_OFFSET);
}

ZTEST(ndef_listener_read, test_read_le_zero_short)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;
	size_t expect_data = 256U;

	if (expect_data > (size_t)(NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U)) {
		expect_data = (size_t)(NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U);
	}

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x00U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, expect_data + 2U);
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_read, test_read_le_zero_ext)
{
	nfc_apdu_t apdu = {
		.cla = 0x00U,
		.ins = NFC_INS_READ_BINARY,
		.p1 = 0x00U,
		.p2 = 0x00U,
		.data = NULL,
		.lc = 0U,
		.le = 0U,
		.has_le = true,
		.extended = true,
	};
	const uint8_t *buf = NULL;
	size_t len = 0U;
	size_t expect_data = (size_t)NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U;
	const nfc_service_t *svc = ndef_listener_get();

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	svc->on_apdu(&apdu, NULL);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, expect_data + 2U);
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_read, test_read_no_le)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;
	size_t expect_data = (size_t)NFC_TRANSPORT_MAX_RESPONSE_LEN - 2U;
	uint8_t storage[8];
	nfc_apdu_t apdu = {
		.cla = 0x00U,
		.ins = NFC_INS_READ_BINARY,
		.p1 = 0x00U,
		.p2 = 0x00U,
		.data = NULL,
		.lc = 0U,
		.le = 0U,
		.has_le = false,
		.extended = false,
	};
	const nfc_service_t *svc = ndef_listener_get();

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	svc->on_apdu(&apdu, NULL);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, expect_data + 2U);
	zassert_true(len <= NFC_TRANSPORT_MAX_RESPONSE_LEN);
	listener_assert_sw_u16(NFC_SW_OK);
	ARG_UNUSED(storage);
}

ZTEST(ndef_listener_read, test_response_capped_255)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U}, 4U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len <= NFC_TRANSPORT_MAX_RESPONSE_LEN);
}

static void listener_suite_before(void *fixture)
{
	ARG_UNUSED(fixture);

	const nfc_service_t *svc = ndef_listener_get();

	svc->on_field_off(NULL);
	nfc_response_spy_reset();
}

ZTEST_SUITE(ndef_listener_read, NULL, listener_read_setup, listener_suite_before, NULL, NULL);
