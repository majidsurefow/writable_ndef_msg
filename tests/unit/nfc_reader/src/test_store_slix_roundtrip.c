/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — SLIX nfc_store envelope save/load (clone-only, 3 CAP variants).
 */

#include "applets/nfc_applet_policy.h"
#include "protocols/slix/slix.h"
#include "store/nfc_store.h"

#include "Slix_cap_default_mock.h"
#include "slix_store_fixture.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static slix_data_t s_model;
static slix_data_t s_expected;
static uint8_t s_saved_blob[4096];
static size_t s_saved_len;

static int mock_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return slix_serialize(&s_model, out, out_max, out_len);
}

static int mock_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return slix_deserialize(&s_model, in, in_len);
}

static nfc_service_t s_svc = {
	.serialize = mock_serialize,
	.deserialize = mock_deserialize,
	.persist_id = NFC_PERSIST_ID_SLIX,
};

static int mock_save_cb(const char *tag, const uint8_t *blob, size_t len, void *user_ctx)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(user_ctx);

	if (len > sizeof(s_saved_blob)) {
		return -ENOSPC;
	}

	(void)memcpy(s_saved_blob, blob, len);
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

	(void)memcpy(out, s_saved_blob, s_saved_len);
	*out_len = s_saved_len;
	return 0;
}

static void load_golden_blob(const uint8_t *blob, size_t len)
{
	zassert_true(len <= sizeof(s_saved_blob));
	(void)memcpy(s_saved_blob, blob, len);
	s_saved_len = len;
}

static void *suite_setup(void)
{
	s_saved_len = 0U;
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(mock_save_cb, NULL);
	(void)nfc_store_register_load_cb(mock_load_cb, NULL);
	return NULL;
}

ZTEST_SUITE(nfc_reader_slix_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_slix_store, test_store_load_golden_default)
{
	const nfc_service_t *svcs[] = { &s_svc };
	int ret;

	slix_data_reset(&s_expected);
	zassert_ok(slix_deserialize(&s_expected, slix_Slix_cap_default_model,
				    SLIX_SLIX_CAP_DEFAULT_MODEL_LEN));
	load_golden_blob(store_fixture_slix_cap_default_card,
			 STORE_FIXTURE_SLIX_CAP_DEFAULT_CARD_LEN);

	slix_data_reset(&s_model);
	ret = nfc_store_load("golden", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.capabilities, s_expected.capabilities);
	zassert_equal(s_model.parent.block_count, s_expected.parent.block_count);
}

ZTEST(nfc_reader_slix_store, test_store_load_golden_missed)
{
	const nfc_service_t *svcs[] = { &s_svc };
	int ret;

	load_golden_blob(store_fixture_slix_cap_missed_card,
			 STORE_FIXTURE_SLIX_CAP_MISSED_CARD_LEN);
	slix_data_reset(&s_model);
	ret = nfc_store_load("missed", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.capabilities, SLIX_CAP_MISSED);
}

ZTEST(nfc_reader_slix_store, test_store_load_golden_accept_all)
{
	const nfc_service_t *svcs[] = { &s_svc };
	int ret;

	load_golden_blob(store_fixture_slix_cap_accept_all_pass_card,
			 STORE_FIXTURE_SLIX_CAP_ACCEPT_ALL_PASS_CARD_LEN);
	slix_data_reset(&s_model);
	ret = nfc_store_load("accept", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.capabilities, SLIX_CAP_ACCEPT_ALL);
}

ZTEST(nfc_reader_slix_store, test_store_peek_clone_only_no_emulate)
{
	zassert_equal(nfc_applet_check_emulate(NFC_PERSIST_ID_SLIX,
					       NFC_STORE_ENTRY_FLAG_READER_CAPTURED),
		      -ENOTSUP);
}

ZTEST(nfc_reader_slix_store, test_store_roundtrip_default)
{
	const nfc_service_t *svcs[] = { &s_svc };
	int ret;

	zassert_ok(slix_deserialize(&s_model, slix_Slix_cap_default_model,
				    SLIX_SLIX_CAP_DEFAULT_MODEL_LEN));
	ret = nfc_store_save("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	slix_data_reset(&s_model);
	ret = nfc_store_load("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.capabilities, SLIX_CAP_DEFAULT);
}
