/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared helpers for NDEF Tier C listener tests.
 */

#ifndef TESTS_UNIT_NFC_NDEF_LISTENER_TEST_HELPERS_H_
#define TESTS_UNIT_NFC_NDEF_LISTENER_TEST_HELPERS_H_

#include "ndef_listener.h"
#include "nfc_response_spy.h"
#include "nfc_test_apdu.h"

#include <string.h>

#include <zephyr/ztest.h>

static inline void listener_test_reset(bool writable)
{
	ndef_listener_config_t cfg = { .writable = writable };

	nfc_response_spy_reset();
	ndef_listener_shutdown();
	zassert_ok(ndef_listener_init(NULL, &cfg));
}

static inline void listener_assert_sw(uint8_t sw1, uint8_t sw2)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 2U);
	zassert_equal(buf[len - 2U], sw1);
	zassert_equal(buf[len - 1U], sw2);
}

static inline void listener_assert_sw_u16(uint16_t sw)
{
	listener_assert_sw((uint8_t)(sw >> 8U), (uint8_t)(sw & 0xFFU));
}

static inline void listener_send_apdu_bytes(const uint8_t *apdu, size_t apdu_len)
{
	uint8_t storage[64];
	nfc_apdu_t parsed;
	const nfc_service_t *svc = ndef_listener_get();

	zassert_ok(nfc_test_apdu_from_bytes(apdu, apdu_len, storage, sizeof(storage), &parsed));
	svc->on_apdu(&parsed, NULL);
}

static inline void listener_select_cc(void)
{
	static const uint8_t cmd[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, 0xE1U, 0x03U};

	listener_send_apdu_bytes(cmd, sizeof(cmd));
}

static inline void listener_select_ndef(void)
{
	static const uint8_t cmd[] = {0x00U, 0xA4U, 0x00U, 0x0CU, 0x02U, 0xE1U, 0x04U};

	listener_send_apdu_bytes(cmd, sizeof(cmd));
}

static inline void listener_read_nofile_check(void)
{
	static const uint8_t cmd[] = {0x00U, 0xB0U, 0x00U, 0x00U, 0x0FU};

	nfc_response_spy_reset();
	listener_send_apdu_bytes(cmd, sizeof(cmd));
	listener_assert_sw_u16(NFC_SW_NO_EF);
}

#endif /* TESTS_UNIT_NFC_NDEF_LISTENER_TEST_HELPERS_H_ */
