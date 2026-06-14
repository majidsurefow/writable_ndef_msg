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

static desfire_data_t s_model; /* Static to avoid ~450 byte stack allocation */

static void listener_reset(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_response_spy_reset();
	(void)desfire_listener_shutdown();
	zassert_ok(desfire_listener_init(NULL));
}

static void listener_load_golden(void)
{
	(void)memset(&s_model, 0, sizeof(s_model));
	zassert_ok(desfire_deserialize(&s_model, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN));
	zassert_ok(desfire_listener_init(&s_model));
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

ZTEST(desfire_listener, test_select_read_data_golden)
{
	static const uint8_t select_app[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_SELECT_APPLICATION,
					     0x00U, 0x00U, 0x03U, 0x01U, 0x02U, 0x03U};
	static const uint8_t read_data[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_READ_DATA, 0x00U, 0x00U,
					    0x07U, 0x01U, 0x00U, 0x00U, 0x00U, 0x0FU, 0x00U, 0x00U};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_reset(NULL);
	listener_load_golden();

	listener_send_native(select_app, sizeof(select_app));
	listener_assert_status(DESFIRE_STATUS_OK);

	listener_send_native(read_data, sizeof(read_data));
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 17U);
	zassert_equal(buf[len - 2U], DESFIRE_SW1);
	zassert_equal(buf[len - 1U], DESFIRE_STATUS_OK);
	zassert_mem_equal(buf, "HELLO DESFIRE!!", 15U);
}

ZTEST(desfire_listener, test_get_value_golden)
{
	static const uint8_t select_app[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_SELECT_APPLICATION,
					     0x00U, 0x00U, 0x03U, 0x01U, 0x02U, 0x03U};
	static const uint8_t get_value[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_GET_VALUE, 0x00U, 0x00U,
					    0x01U, 0x02U};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_reset(NULL);
	(void)memset(&s_model, 0, sizeof(s_model));
	zassert_ok(desfire_deserialize(&s_model, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN));
	s_model.apps[0].file_ids[0] = 0x02U;
	s_model.apps[0].file_settings[0].type = DESFIRE_FILE_TYPE_VALUE;
	s_model.apps[0].file_settings[0].access_rights = 0x0E0EU;
	s_model.apps[0].file_data[0][0] = 0x10U;
	s_model.apps[0].file_data[0][1] = 0x27U;
	s_model.apps[0].file_data[0][2] = 0x00U;
	s_model.apps[0].file_data[0][3] = 0x00U;
	s_model.apps[0].file_data_len[0] = 4U;
	zassert_ok(desfire_listener_init(&s_model));

	listener_send_native(select_app, sizeof(select_app));
	listener_assert_status(DESFIRE_STATUS_OK);

	listener_send_native(get_value, sizeof(get_value));
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 6U);
	zassert_equal(buf[len - 2U], DESFIRE_SW1);
	zassert_equal(buf[len - 1U], DESFIRE_STATUS_OK);
	zassert_equal(buf[0], 0x10U);
	zassert_equal(buf[1], 0x27U);
}

ZTEST(desfire_listener, test_read_records_golden)
{
	static const uint8_t select_app[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_SELECT_APPLICATION,
					     0x00U, 0x00U, 0x03U, 0x01U, 0x02U, 0x03U};
	static const uint8_t read_records[] = {DESFIRE_CLA_NATIVE, DESFIRE_CMD_READ_RECORDS, 0x00U,
					     0x00U, 0x07U, 0x03U, 0x00U, 0x00U, 0x00U,
					     0x04U, 0x00U, 0x00U};
	const uint8_t *buf = NULL;
	size_t len = 0U;

	listener_reset(NULL);
	(void)memset(&s_model, 0, sizeof(s_model));
	zassert_ok(desfire_deserialize(&s_model, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN));
	s_model.apps[0].file_ids[0] = 0x03U;
	s_model.apps[0].file_settings[0].type = DESFIRE_FILE_TYPE_LINEAR_REC;
	s_model.apps[0].file_settings[0].settings.record.record_size = 4U;
	s_model.apps[0].file_settings[0].access_rights = 0x0E0EU;
	s_model.apps[0].file_data[0][0] = 0xAAU;
	s_model.apps[0].file_data[0][1] = 0xBBU;
	s_model.apps[0].file_data[0][2] = 0xCCU;
	s_model.apps[0].file_data[0][3] = 0xDDU;
	s_model.apps[0].file_data_len[0] = 4U;
	zassert_ok(desfire_listener_init(&s_model));

	listener_send_native(select_app, sizeof(select_app));
	listener_assert_status(DESFIRE_STATUS_OK);

	listener_send_native(read_records, sizeof(read_records));
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 6U);
	zassert_equal(buf[len - 2U], DESFIRE_SW1);
	zassert_equal(buf[len - 1U], DESFIRE_STATUS_OK);
	zassert_equal(buf[0], 0xAAU);
	zassert_equal(buf[1], 0xBBU);
	zassert_equal(buf[2], 0xCCU);
	zassert_equal(buf[3], 0xDDU);
}

ZTEST(desfire_listener, test_illegal_ins_rejects)
{
	static const uint8_t bad[] = {DESFIRE_CLA_NATIVE, 0xFFU, 0x00U, 0x00U, 0x00U};

	listener_reset(NULL);
	listener_send_native(bad, sizeof(bad));
	listener_assert_status(DESFIRE_STATUS_ILLEGAL_CMD);
}

ZTEST_SUITE(desfire_listener, NULL, NULL, listener_reset, NULL, NULL);
