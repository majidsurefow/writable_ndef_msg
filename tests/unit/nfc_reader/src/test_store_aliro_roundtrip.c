/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — Aliro nfc_store envelope save/load.
 */

#include "aliro_listener.h"
#include "applets/nfc_applet_policy.h"
#include "store/nfc_store.h"

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

ZTEST_SUITE(nfc_reader_aliro_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_aliro_store, test_store_load_roundtrip)
{
	const nfc_service_t *svcs[] = { aliro_listener_get() };
	aliro_data_t expected;
	aliro_data_t loaded;
	uint8_t body[768];
	size_t body_len = 0U;
	int ret;

	aliro_data_reset(&expected);
	expected.protocol_major = 0U;
	expected.protocol_minor = 9U;
	expected.credential_pubkey[0] = 0x04U;
	expected.transcript_len = 2U;
	expected.transcript[0] = 0xAAU;
	expected.transcript[1] = 0xBBU;

	zassert_ok(aliro_listener_init(&expected));

	ret = nfc_store_save("aliro_rt", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	(void)aliro_listener_shutdown();
	zassert_ok(aliro_listener_init(NULL));

	ret = nfc_store_load("aliro_rt", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ret = aliro_listener_get()->serialize(body, sizeof(body), &body_len, NULL);
	zassert_ok(ret);
	ret = aliro_deserialize(&loaded, body, body_len);
	zassert_ok(ret);
	zassert_equal(loaded.transcript_len, expected.transcript_len);
	zassert_mem_equal(loaded.transcript, expected.transcript, expected.transcript_len);
}

ZTEST(nfc_reader_aliro_store, test_store_peek_emulate_allowed)
{
	uint8_t persist_id = NFC_PERSIST_ID_ALIRO;
	uint8_t flags = NFC_STORE_ENTRY_FLAG_HAND_AUTHORED | NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE;

	zassert_ok(nfc_applet_check_emulate(persist_id, flags));
}
