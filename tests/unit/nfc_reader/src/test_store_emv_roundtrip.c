/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — EMV nfc_store envelope save/load.
 */

#include "applets/nfc_applet_policy.h"
#include "emv_listener.h"
#include "emv.h"
#include "store/nfc_store.h"
#include "emv_store_fixture.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static uint8_t s_saved_blob[2048];
static size_t s_saved_len;

static void load_golden_blob(const uint8_t *blob, size_t len)
{
	zassert_true(len <= sizeof(s_saved_blob));
	(void)memcpy(s_saved_blob, blob, len);
	s_saved_len = len;
}

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

static void *suite_setup(void)
{
	s_saved_len = 0U;
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(mock_save_cb, NULL);
	(void)nfc_store_register_load_cb(mock_load_cb, NULL);
	return NULL;
}

ZTEST_SUITE(nfc_reader_emv_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_emv_store, test_store_load_roundtrip)
{
	const nfc_service_t *svcs[] = { emv_listener_get() };
	emv_card_image_t expected;
	emv_card_image_t loaded;
	uint8_t body[512];
	size_t body_len = 0U;
	int ret;

	emv_card_image_load_default(&expected);
	zassert_ok(emv_listener_init(&expected));

	ret = nfc_store_save("emv_rt", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	(void)emv_listener_shutdown();
	zassert_ok(emv_listener_init(NULL));

	ret = nfc_store_load("emv_rt", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ret = emv_listener_get()->serialize(body, sizeof(body), &body_len, NULL);
	zassert_ok(ret);
	ret = emv_deserialize(&loaded, body, body_len);
	zassert_ok(ret);
	zassert_equal(loaded.pan_len, expected.pan_len);
	zassert_equal(loaded.record_count, expected.record_count);
}

ZTEST(nfc_reader_emv_store, test_store_load_golden_emv_card)
{
	const nfc_service_t *svcs[] = { emv_listener_get() };
	emv_card_image_t expected;
	uint8_t body[512];
	uint8_t expected_body[512];
	size_t body_len = 0U;
	size_t expected_len = 0U;
	int ret;

	emv_card_image_load_default(&expected);
	load_golden_blob(store_fixture_emv_card, STORE_FIXTURE_EMV_CARD_LEN);

	(void)emv_listener_shutdown();
	zassert_ok(emv_listener_init(NULL));

	ret = nfc_store_load("golden", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ret = emv_listener_get()->serialize(body, sizeof(body), &body_len, NULL);
	zassert_ok(ret);
	ret = emv_serialize(&expected, expected_body, sizeof(expected_body), &expected_len);
	zassert_ok(ret);
	zassert_equal(body_len, expected_len);
	zassert_mem_equal(body, expected_body, expected_len);
}

ZTEST(nfc_reader_emv_store, test_store_peek_emulate_allowed)
{
	uint8_t persist_id = NFC_PERSIST_ID_EMV;
	uint8_t flags = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE;

	zassert_ok(nfc_applet_check_emulate(persist_id, flags));
}
