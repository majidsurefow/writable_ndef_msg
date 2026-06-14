/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire Tier C — partial listener (cookbook §14.4 / §5.4).
 */

#include "desfire_listener.h"

#include "Desfire_mock.h"
#include "nfc_response_spy.h"
#include "nfc_test_apdu.h"

#include <string.h>

#include <zephyr/ztest.h>

static void listener_reset(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_response_spy_reset();
	(void)desfire_listener_shutdown();
	zassert_ok(desfire_listener_init(NULL));
}

static void listener_load_golden(void)
{
	desfire_data_t model;

	zassert_ok(desfire_deserialize(&model, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN));
	zassert_ok(desfire_listener_init(&model));
}

static void listener_send_native(const uint8_t *apdu, size_t apdu_len)
{
	uint8_t storage[64];
	nfc_apdu_t parsed;
	const nfc_service_t *svc = desfire_listener_get();

	zassert_ok(nfc_test_apdu_from_bytes(apdu, apdu_len, storage, sizeof(storage), &parsed));
	svc->on_apdu(&parsed, NULL);
}

static void listener_assert_status(uint8_t sw2)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 2U);
	zassert_equal(buf[len - 2U], DESFIRE_SW1);
	zassert_equal(buf[len - 1U], sw2);
}

ZTEST(desfire_listener, test_on_select_iso9000)
{
	const nfc_service_t *svc = desfire_listener_get();

	listener_reset(NULL);
	nfc_response_spy_reset();
	const uint8_t *buf = NULL;
	size_t len = 0U;

	svc->on_select(desfire_aid, DESFIRE_AID_LEN, NULL);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 2U);
	zassert_equal(buf[len - 2U], 0x90U);
	zassert_equal(buf[len - 1U], 0x00U);
	zassert_equal(desfire_listener_auth_state(), DESFIRE_AUTH_STATE_IDLE);
}

ZTEST(desfire_listener, test_get_version_chained)
{
	static const uint8_t get_version[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_VERSION,
					      0x00U, 0x00U, 0x00U};
	static const uint8_t af[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_ADDITIONAL_FRAME, 0x00U,
				     0x00U, 0x00U};

	listener_reset(NULL);
	listener_load_golden();

	listener_send_native(get_version, sizeof(get_version));
	listener_assert_status(DESFIRE_STATUS_ADDITIONAL_FRAME);

	listener_send_native(af, sizeof(af));
	listener_assert_status(DESFIRE_STATUS_ADDITIONAL_FRAME);

	listener_send_native(af, sizeof(af));
	listener_assert_status(DESFIRE_STATUS_OK);
}

ZTEST(desfire_listener, test_auth_without_key_rejects)
{
	static const uint8_t auth[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_AUTHENTICATE_AES,
				       0x00U, 0x00U, 0x01U, 0x00U};

	listener_reset(NULL);
	listener_load_golden();
	listener_send_native(auth, sizeof(auth));
	listener_assert_status(DESFIRE_STATUS_AUTH_ERROR);
}

ZTEST(desfire_listener, test_field_off_resets_auth)
{
	const nfc_service_t *svc = desfire_listener_get();

	listener_reset(NULL);
	svc->on_field_off(NULL);
	zassert_equal(desfire_listener_auth_state(), DESFIRE_AUTH_STATE_IDLE);
}

ZTEST(desfire_listener, test_illegal_ins_rejects)
{
	static const uint8_t bad[] = {DESFIRE_CLA_NATIVE, 0xFFU, 0x00U, 0x00U, 0x00U};

	listener_reset(NULL);
	listener_send_native(bad, sizeof(bad));
	listener_assert_status(DESFIRE_STATUS_ILLEGAL_CMD);
}

ZTEST_SUITE(desfire_listener, NULL, NULL, listener_reset, NULL, NULL);
