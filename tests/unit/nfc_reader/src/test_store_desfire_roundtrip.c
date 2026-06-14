/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — DESFire nfc_store envelope save/load.
 */

#include "applets/nfc_applet_policy.h"
#include "desfire_listener.h"
#include "store/nfc_store.h"

#include "Desfire_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static uint8_t s_saved_blob[2048];
static size_t s_saved_len;

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

ZTEST_SUITE(nfc_reader_desfire_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_desfire_store, test_store_load_roundtrip)
{
	const nfc_service_t *svcs[] = { desfire_listener_get() };
	desfire_data_t expected;
	desfire_data_t loaded;
	uint8_t body[512];
	size_t body_len = 0U;
	int ret;

	zassert_ok(desfire_deserialize(&expected, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN));
	zassert_ok(desfire_listener_init(&expected));

	ret = nfc_store_save("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	(void)desfire_listener_shutdown();
	zassert_ok(desfire_listener_init(NULL));

	ret = nfc_store_load("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ret = desfire_listener_get()->serialize(body, sizeof(body), &body_len, NULL);
	zassert_ok(ret);
	ret = desfire_deserialize(&loaded, body, body_len);
	zassert_ok(ret);
	zassert_mem_equal(loaded.uid, expected.uid, DESFIRE_UID_SIZE);
	zassert_equal(loaded.app_count, expected.app_count);
}

ZTEST(nfc_reader_desfire_store, test_store_peek_emulate_allowed)
{
	uint8_t persist_id = NFC_PERSIST_ID_DESFIRE;
	uint8_t flags = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE;

	zassert_ok(nfc_applet_check_emulate(persist_id, flags));
}
