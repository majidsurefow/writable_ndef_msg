/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Tier C — listener SELECT (wave5-ndef §8 Task 5 / 8).
 */

#include "listener_test_helpers.h"

#include <string.h>

#include <zephyr/ztest.h>

static const uint8_t NDEF_AID[] = {0xD2U, 0x76U, 0x00U, 0x00U, 0x85U, 0x01U, 0x01U};

static void *listener_select_setup(void)
{
	listener_test_reset(true);
	return NULL;
}

ZTEST(ndef_listener_select, test_on_select_sends_9000)
{
	const nfc_service_t *svc = ndef_listener_get();

	nfc_response_spy_reset();
	svc->on_select(NDEF_AID, sizeof(NDEF_AID), NULL);
	zassert_equal(nfc_response_spy_call_count(), 1U);
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_select, test_on_select_resets_file_sel)
{
	const nfc_service_t *svc = ndef_listener_get();

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	svc->on_select(NDEF_AID, sizeof(NDEF_AID), NULL);
	listener_assert_sw_u16(NFC_SW_OK);
	listener_read_nofile_check();
}

ZTEST(ndef_listener_select, test_select_file_cc)
{
	listener_select_cc();
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_select, test_select_file_ndef)
{
	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);
}

ZTEST(ndef_listener_select, test_select_file_notfound)
{
	static const uint8_t cmd[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, 0xE1U, 0x05U};

	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_FILE_NOT_FOUND);
}

ZTEST(ndef_listener_select, test_select_file_badlen)
{
	static const uint8_t cmd[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x01U, 0xE1U};

	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_WRONG_LENGTH);
}

ZTEST(ndef_listener_select, test_select_file_badp1)
{
	static const uint8_t cmd[] = {0x00U, 0xA4U, 0x04U, 0x0CU, 0x02U, 0xE1U, 0x03U};

	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_WRONG_P1P2);
}

ZTEST(ndef_listener_select, test_select_file_badp2)
{
	static const uint8_t cmd[] = {0x00U, 0xA4U, 0x00U, 0x04U, 0x02U, 0xE1U, 0x03U};

	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_WRONG_P1P2);
}

ZTEST(ndef_listener_select, test_on_deselect_clears_file_sel)
{
	const nfc_service_t *svc = ndef_listener_get();

	listener_select_cc();
	listener_assert_sw_u16(NFC_SW_OK);
	svc->on_deselect(NULL);
	listener_read_nofile_check();
}

ZTEST(ndef_listener_select, test_on_field_off_clears_file_sel)
{
	const nfc_service_t *svc = ndef_listener_get();

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);
	svc->on_field_off(NULL);
	listener_read_nofile_check();
}

static void listener_suite_before(void *fixture)
{
	ARG_UNUSED(fixture);

	const nfc_service_t *svc = ndef_listener_get();

	svc->on_field_off(NULL);
	nfc_response_spy_reset();
}

ZTEST_SUITE(ndef_listener_select, NULL, listener_select_setup, listener_suite_before, NULL, NULL);
