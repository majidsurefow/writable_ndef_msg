/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — FeliCa nfc_store envelope save/load + E+ virtual loopback.
 */

#include "applets/nfc_applet_policy.h"
#include "protocols/felica/felica.h"
#include "store/nfc_store.h"

#include "Felica_mock.h"
#include "felica_store_fixture.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

static felica_data_t s_model;
static uint8_t s_saved_blob[2048];
static size_t s_saved_len;

static int mock_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return felica_serialize(&s_model, out, out_max, out_len);
}

static int mock_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return felica_deserialize(&s_model, in, in_len);
}

static nfc_service_t s_svc = {
	.serialize = mock_serialize,
	.deserialize = mock_deserialize,
	.persist_id = NFC_PERSIST_ID_FELICA,
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

static void load_golden_model(felica_data_t *data)
{
	int ret;

	felica_data_reset(data);
	ret = felica_deserialize(data, felica_Felica_model, FELICA_FELICA_MODEL_LEN);
	zassert_ok(ret);
}

static void *suite_setup(void)
{
	s_saved_len = 0U;
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(mock_save_cb, NULL);
	(void)nfc_store_register_load_cb(mock_load_cb, NULL);
	return NULL;
}

ZTEST_SUITE(nfc_reader_felica_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_felica_store, test_store_load_golden)
{
	const nfc_service_t *svcs[] = { &s_svc };
	felica_data_t expected;
	int ret;

	load_golden_model(&expected);
	load_golden_blob(store_fixture_felica_card, STORE_FIXTURE_FELICA_CARD_LEN);

	felica_data_reset(&s_model);
	ret = nfc_store_load("golden", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_mem_equal(s_model.idm, expected.idm, FELICA_IDM_SIZE);
	zassert_equal(s_model.blocks_total, expected.blocks_total);
}

ZTEST(nfc_reader_felica_store, test_store_load_roundtrip)
{
	const nfc_service_t *svcs[] = { &s_svc };
	felica_data_t loaded;
	int ret;

	load_golden_model(&s_model);
	ret = nfc_store_save("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	felica_data_reset(&s_model);
	ret = nfc_store_load("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	loaded = s_model;
	load_golden_model(&s_model);
	zassert_mem_equal(loaded.idm, s_model.idm, FELICA_IDM_SIZE);
	zassert_equal(loaded.blocks_total, s_model.blocks_total);
}

ZTEST(nfc_reader_felica_store, test_store_peek_clone_only_no_emulate)
{
	uint8_t persist_id = NFC_PERSIST_ID_FELICA;
	uint8_t flags = NFC_STORE_ENTRY_FLAG_READER_CAPTURED;

	zassert_equal(nfc_applet_check_emulate(persist_id, flags), -ENOTSUP);
}

ZTEST(nfc_reader_felica_store, test_store_peek_golden)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	load_golden_blob(store_fixture_felica_card, STORE_FIXTURE_FELICA_CARD_LEN);
	ret = nfc_store_peek_entry_meta("golden", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_FELICA);
	zassert_equal(flags, NFC_STORE_ENTRY_FLAG_HAND_AUTHORED);
}
