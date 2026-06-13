/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Tier C — listener UPDATE BINARY (wave5-ndef §8 Task 7).
 */

#include "listener_test_helpers.h"

#include <string.h>

#include <zephyr/ztest.h>

#define NDEF_FILE_BUF_SIZE (NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE)

static void *listener_update_setup(void)
{
	listener_test_reset(true);
	return NULL;
}

ZTEST(ndef_listener_update, test_update_ndef_ok)
{
	static const uint8_t payload[] = {0xAAU, 0xBBU, 0xCCU, 0xDDU};
	static const uint8_t cmd[] = {
		0x00U, 0xD6U, 0x00U, 0x00U, 0x04U, 0xAAU, 0xBBU, 0xCCU, 0xDDU,
	};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x04U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, sizeof(payload) + 2U);
	zassert_mem_equal(buf, payload, sizeof(payload));
}

ZTEST(ndef_listener_update, test_update_nlen_then_read)
{
	static const uint8_t nlen[] = {0x00U, 0x05U};
	static const uint8_t msg[] = {0xD1U, 0x01U, 0x03U, 0x55U, 0x01U};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	listener_send_apdu_bytes(
		(const uint8_t[]){0x00U, 0xD6U, 0x00U, 0x00U, 0x02U, 0x00U, 0x05U}, 7U);
	listener_assert_sw_u16(NFC_SW_OK);

	listener_send_apdu_bytes(
		(const uint8_t[]){0x00U, 0xD6U, 0x00U, 0x02U, 0x05U, 0xD1U, 0x01U, 0x03U, 0x55U, 0x01U},
		10U);
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x07U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, 9U);
	zassert_mem_equal(buf, nlen, sizeof(nlen));
	zassert_mem_equal(&buf[2], msg, sizeof(msg));
}

ZTEST(ndef_listener_update, test_update_nofile)
{
	listener_send_apdu_bytes(
		(const uint8_t[]){0x00U, 0xD6U, 0x00U, 0x00U, 0x01U, 0x00U}, 6U);
	listener_assert_sw_u16(NFC_SW_NO_EF);
}

ZTEST(ndef_listener_update, test_update_cc)
{
	listener_select_cc();
	listener_assert_sw_u16(NFC_SW_OK);

	listener_send_apdu_bytes(
		(const uint8_t[]){0x00U, 0xD6U, 0x00U, 0x00U, 0x01U, 0x00U}, 6U);
	listener_assert_sw_u16(NFC_SW_CONDITIONS_NOT_SAT);
}

ZTEST(ndef_listener_update, test_update_readonly)
{
	ndef_listener_config_t cfg = { .writable = false };

	ndef_listener_shutdown();
	zassert_ok(ndef_listener_init(NULL, &cfg));

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	listener_send_apdu_bytes(
		(const uint8_t[]){0x00U, 0xD6U, 0x00U, 0x00U, 0x01U, 0x00U}, 6U);
	listener_assert_sw_u16(NFC_SW_CONDITIONS_NOT_SAT);
}

ZTEST(ndef_listener_update, test_update_lc_zero)
{
	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xD6U, 0x00U, 0x00U, 0x00U}, 5U);
	listener_assert_sw_u16(NFC_SW_WRONG_LENGTH);
}

ZTEST(ndef_listener_update, test_update_overflow)
{
	static const uint8_t data = 0U;
	nfc_apdu_t apdu = {
		.cla = 0x00U,
		.ins = NFC_INS_UPDATE_BINARY,
		.p1 = 0x00U,
		.p2 = 0x00U,
		.data = &data,
		.lc = (uint16_t)(NDEF_FILE_BUF_SIZE + 1U),
		.le = 0U,
		.has_le = false,
		.extended = false,
	};
	const nfc_service_t *svc = ndef_listener_get();

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	svc->on_apdu(&apdu, NULL);
	listener_assert_sw_u16(NFC_SW_WRONG_LENGTH);
}

ZTEST(ndef_listener_update, test_update_at_boundary)
{
	uint8_t offset_hi = (uint8_t)((NDEF_FILE_BUF_SIZE - 1U) >> 8U);
	uint8_t offset_lo = (uint8_t)((NDEF_FILE_BUF_SIZE - 1U) & 0xFFU);
	const uint8_t cmd[] = {0x00U, 0xD6U, offset_hi, offset_lo, 0x01U, 0x5AU};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes(
		(const uint8_t[]){0x00U, 0xB0U, offset_hi, offset_lo, 0x01U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, 3U);
	zassert_equal(buf[0], 0x5AU);
}

static void listener_suite_before(void *fixture)
{
	ARG_UNUSED(fixture);

	const nfc_service_t *svc = ndef_listener_get();

	svc->on_field_off(NULL);
	nfc_response_spy_reset();
}

ZTEST_SUITE(ndef_listener_update, NULL, listener_update_setup, listener_suite_before, NULL, NULL);
