/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — Headless L1 applet tests (P5 — Master Plan Part E.5).
 *
 * Proves the L1 applet API works without shell (log==NULL for callbacks,
 * nfc_applet_service.h functions with result structs). These tests call L1
 * directly, never touch struct shell *.
 */

#include "applets/nfc_applet_service.h"
#include "nfc_reader_session_mock.h"
#include "store/nfc_store.h"
#include "protocols/ndef/ndef.h"
#include "store_fixture.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

/* ------------------------------------------------------------------ */
/* Helper to set up mock session for scan_get_result tests            */
/* ------------------------------------------------------------------ */

static void set_mock_session(const uint8_t *uid, size_t uid_len,
			     uint8_t protocol, uint8_t interface, uint8_t mode_tech)
{
	nfc_reader_session_t session = {0};

	memcpy(session.tag.uid.bytes, uid, uid_len);
	session.tag.uid.len = uid_len;
	session.tag.protocol = protocol;
	session.tag.interface = interface;
	session.tag.mode_tech = mode_tech;
	session.tag.valid = true;
	nfc_reader_session_mock_set(&session);
}

/* ------------------------------------------------------------------ */
/* Mock store for get_card_meta                                       */
/* ------------------------------------------------------------------ */

static uint8_t s_saved_blob[512];
static size_t s_saved_len;

static int mock_save_cb(const char *tag, const uint8_t *blob, size_t len, void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(user_ctx);

	if (len > sizeof(s_saved_blob)) {
		return -ENOSPC;
	}
	memcpy(s_saved_blob, blob, len);
	s_saved_len = len;
	return 0;
}

static int mock_load_cb(const char *tag, uint8_t *out, size_t max, size_t *out_len,
			void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(user_ctx);

	if (s_saved_len == 0U) {
		return -ENOENT;
	}
	if (s_saved_len > max) {
		return -ENOSPC;
	}
	memcpy(out, s_saved_blob, s_saved_len);
	*out_len = s_saved_len;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Test suite setup                                                   */
/* ------------------------------------------------------------------ */

static void *suite_setup(void)
{
	s_saved_len = 0U;
	nfc_reader_session_mock_clear();
	nfc_store_init(NULL);
	nfc_store_register_save_cb(mock_save_cb, NULL);
	nfc_store_register_load_cb(mock_load_cb, NULL);
	return NULL;
}

static void before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	s_saved_len = 0U;
	nfc_reader_session_mock_clear();
}

ZTEST_SUITE(nfc_applet_headless, NULL, suite_setup, before_each, NULL, NULL);

/* ------------------------------------------------------------------ */
/* scan_get_result tests (L1 API, no shell)                           */
/* ------------------------------------------------------------------ */

ZTEST(nfc_applet_headless, test_scan_get_result_no_session)
{
	nfc_applet_scan_result_t result;
	int ret;

	nfc_reader_session_mock_clear();
	ret = nfc_applet_scan_get_result(&result);
	zassert_equal(ret, -ENOENT, "Expected -ENOENT when no session active");
}

ZTEST(nfc_applet_headless, test_scan_get_result_null_out)
{
	int ret;

	ret = nfc_applet_scan_get_result(NULL);
	zassert_equal(ret, -EINVAL, "Expected -EINVAL for NULL output");
}

ZTEST(nfc_applet_headless, test_scan_get_result_populated)
{
	static const uint8_t uid[] = {0x04, 0xA2, 0x24, 0x33, 0x44, 0x55, 0x66};
	nfc_applet_scan_result_t result;
	int ret;

	set_mock_session(uid, sizeof(uid), 0x02, 0x01, 0xAB);

	ret = nfc_applet_scan_get_result(&result);
	zassert_ok(ret, "scan_get_result should succeed with active session");
	zassert_true(result.valid, "Result should be valid");
	zassert_equal(result.uid.len, sizeof(uid), "UID length mismatch");
	zassert_mem_equal(result.uid.bytes, uid, sizeof(uid), "UID bytes mismatch");
	zassert_equal(result.protocol, 0x02, "Protocol mismatch");
	zassert_equal(result.interface, 0x01, "Interface mismatch");
	zassert_equal(result.mode_tech, 0xAB, "Mode/tech mismatch");
}

/* ------------------------------------------------------------------ */
/* get_card_meta tests (L1 API, no shell)                             */
/* ------------------------------------------------------------------ */

ZTEST(nfc_applet_headless, test_get_card_meta_missing_slot)
{
	nfc_applet_card_meta_t meta;
	int ret;

	ret = nfc_applet_get_card_meta("missing", &meta);
	zassert_equal(ret, -ENOENT, "Expected -ENOENT for missing slot");
	zassert_false(meta.present, "meta.present should be false for missing slot");
}

ZTEST(nfc_applet_headless, test_get_card_meta_from_golden)
{
	nfc_applet_card_meta_t meta;
	int ret;

	/* Load a golden NDEF blob into the mock store */
	zassert_true(STORE_FIXTURE_NDEF_EMPTY_CARD_LEN <= sizeof(s_saved_blob));
	memcpy(s_saved_blob, store_fixture_ndef_empty_card, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);
	s_saved_len = STORE_FIXTURE_NDEF_EMPTY_CARD_LEN;

	ret = nfc_applet_get_card_meta("golden", &meta);
	zassert_ok(ret, "get_card_meta should succeed for valid slot");
	zassert_true(meta.present, "meta.present should be true");
	zassert_equal(meta.persist_id, NFC_PERSIST_ID_NDEF, "persist_id should be NDEF");
	zassert_not_null(meta.protocol_name, "protocol_name should be set");
}

/* ------------------------------------------------------------------ */
/* verify_compare (already tested in test_verify_diff.c, but add      */
/* one headless L1 sanity check)                                       */
/* ------------------------------------------------------------------ */

ZTEST(nfc_applet_headless, test_verify_compare_headless)
{
	ndef_data_t expected = {0};
	ndef_data_t actual = {0};
	int ret;

	/* Empty NDEF models should match */
	ndef_data_reset(&expected);
	expected.cc_len = NFC_NDEF_CC_LEN;
	expected.ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;

	actual = expected;

	ret = nfc_applet_verify_compare(&expected, &actual, NULL, NULL);
	zassert_ok(ret, "Identical models should compare equal");
}

ZTEST(nfc_applet_headless, test_verify_compare_headless_mismatch)
{
	ndef_data_t expected = {0};
	ndef_data_t actual = {0};
	int ret;

	ndef_data_reset(&expected);
	expected.cc_len = NFC_NDEF_CC_LEN;
	expected.ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN + 3;
	expected.ndef_file[0] = 0x00;
	expected.ndef_file[1] = 0x03;
	expected.ndef_file[2] = 0xD1;
	expected.ndef_file[3] = 0x01;
	expected.ndef_file[4] = 0x00;

	ndef_data_reset(&actual);
	actual.cc_len = NFC_NDEF_CC_LEN;
	actual.ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN + 3;
	actual.ndef_file[0] = 0x00;
	actual.ndef_file[1] = 0x03;
	actual.ndef_file[2] = 0xFF; /* Different byte */
	actual.ndef_file[3] = 0x01;
	actual.ndef_file[4] = 0x00;

	ret = nfc_applet_verify_compare(&expected, &actual, NULL, NULL);
	zassert_equal(ret, -EBADMSG, "Different models should return -EBADMSG");
}
