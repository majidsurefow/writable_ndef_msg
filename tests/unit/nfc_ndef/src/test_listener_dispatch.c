/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Tier C — listener dispatch + cross-tier (wave5-ndef §8 Task 12).
 */

#include "listener_test_helpers.h"

#include "ndef_fixture.h"
#include "ndef.h"

#include <string.h>

#include <zephyr/ztest.h>

static ndef_data_t s_model; /* Static to avoid stack allocation */

static void *listener_dispatch_setup(void)
{
	ndef_fixture_init_read_scripts();
	listener_test_reset(true);
	return NULL;
}

ZTEST(ndef_listener_dispatch, test_unknown_ins)
{
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0x01U, 0x00U, 0x00U}, 4U);
	listener_assert_sw_u16(NFC_SW_INS_NOT_SUPPORTED);
}

ZTEST(ndef_listener_dispatch, test_bad_cla)
{
	listener_send_apdu_bytes((const uint8_t[]){0x80U, 0xB0U, 0x00U, 0x00U, 0x01U}, 5U);
	listener_assert_sw_u16(NFC_SW_CLA_NOT_SUPPORTED);
}

ZTEST(ndef_listener_dispatch, test_deserialize_then_read)
{
	uint8_t blob[1U + NFC_NDEF_CC_LEN + NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE];
	size_t blob_len = 0U;
	const nfc_service_t *svc = ndef_listener_get();
	const uint8_t *buf = NULL;
	size_t len = 0U;

	(void)memset(&s_model, 0, sizeof(s_model));
	ndef_data_reset(&s_model);
	(void)memcpy(s_model.cc, ndef_fixture_cc_std, NFC_NDEF_CC_LEN);
	s_model.cc_len = NFC_NDEF_CC_LEN;
	s_model.ndef_file[0] = 0x00U;
	s_model.ndef_file[1] = (uint8_t)sizeof(ndef_fixture_uri_payload);
	(void)memcpy(&s_model.ndef_file[2], ndef_fixture_uri_payload,
		     sizeof(ndef_fixture_uri_payload));
	s_model.ndef_file_len =
		(uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + sizeof(ndef_fixture_uri_payload));

	zassert_ok(ndef_serialize(&s_model, blob, sizeof(blob), &blob_len));

	ndef_listener_shutdown();
	zassert_ok(ndef_listener_init(NULL, &(ndef_listener_config_t){ .writable = true }));
	zassert_ok(svc->deserialize(blob, blob_len, NULL));

	listener_select_ndef();
	listener_assert_sw_u16(NFC_SW_OK);

	nfc_response_spy_reset();
	listener_send_apdu_bytes((const uint8_t[]){0x00U, 0xB0U, 0x00U, 0x00U, 0x07U}, 5U);
	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, 9U);
	zassert_equal(buf[0], 0x00U);
	zassert_equal(buf[1], (uint8_t)sizeof(ndef_fixture_uri_payload));
	zassert_mem_equal(&buf[2], ndef_fixture_uri_payload, sizeof(ndef_fixture_uri_payload));
	listener_assert_sw_u16(NFC_SW_OK);
}

static void listener_suite_before(void *fixture)
{
	ARG_UNUSED(fixture);

	const nfc_service_t *svc = ndef_listener_get();

	svc->on_field_off(NULL);
	nfc_response_spy_reset();
}

ZTEST_SUITE(ndef_listener_dispatch, NULL, listener_dispatch_setup, listener_suite_before, NULL, NULL);
