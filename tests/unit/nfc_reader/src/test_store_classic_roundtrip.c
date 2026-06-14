/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — classic nfc_store envelope save/load (clone-only, no loopback).
 */

#include "applets/nfc_applet_policy.h"
#include "protocols/classic/classic.h"
#include "store/nfc_store.h"

#include "MfClassic_1K_4b_mock.h"
#include "classic_store_fixture.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

static classic_data_t s_model;
/*
 * NOTE: classic_data_t is ~4KB. These tests work because s_model is shared
 * (via serialize/deserialize callbacks) and local variables are assigned
 * directly from it, avoiding duplicate large structs on stack simultaneously.
 */
static uint8_t s_saved_blob[2048];
static size_t s_saved_len;

static int mock_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return classic_serialize(&s_model, out, out_max, out_len);
}

static int mock_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return classic_deserialize(&s_model, in, in_len);
}

static nfc_service_t s_svc = {
	.serialize = mock_serialize,
	.deserialize = mock_deserialize,
	.persist_id = NFC_PERSIST_ID_CLASSIC,
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

static void load_golden_model(classic_data_t *data)
{
	int ret;

	classic_data_reset(data);
	ret = classic_deserialize(data, classic_MfClassic_1K_4b_model,
				  CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN);
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

ZTEST_SUITE(nfc_reader_classic_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_classic_store, test_store_load_golden_mfclassic_1k_4b)
{
	const nfc_service_t *svcs[] = { &s_svc };
	classic_data_t expected;
	int ret;

	load_golden_model(&expected);
	load_golden_blob(store_fixture_mfclassic_1k_4b_card,
			 STORE_FIXTURE_MFCLASSIC_1K_4B_CARD_LEN);

	classic_data_reset(&s_model);
	ret = nfc_store_load("golden", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.type, expected.type);
	zassert_equal(s_model.uid_len, expected.uid_len);
	zassert_mem_equal(s_model.uid, expected.uid, expected.uid_len);
	zassert_mem_equal(s_model.blocks[0], expected.blocks[0], CLASSIC_BLOCK_SIZE);
}

ZTEST(nfc_reader_classic_store, test_store_load_roundtrip_mfclassic_1k_4b)
{
	const nfc_service_t *svcs[] = { &s_svc };
	classic_data_t loaded;
	uint8_t expected_blob[CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN];
	size_t expected_len = 0U;
	int ret;

	load_golden_model(&s_model);
	ret = classic_serialize(&s_model, expected_blob, sizeof(expected_blob), &expected_len);
	zassert_ok(ret);

	ret = nfc_store_save("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	classic_data_reset(&s_model);
	ret = nfc_store_load("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	loaded = s_model;
	load_golden_model(&s_model);
	zassert_equal(loaded.type, s_model.type);
	zassert_equal(loaded.uid_len, s_model.uid_len);
	zassert_mem_equal(loaded.uid, s_model.uid, loaded.uid_len);
	zassert_mem_equal(loaded.blocks[4], s_model.blocks[4], CLASSIC_BLOCK_SIZE);

	ret = classic_serialize(&loaded, s_saved_blob, sizeof(s_saved_blob), &s_saved_len);
	zassert_ok(ret);
	zassert_equal(s_saved_len, expected_len);
	zassert_mem_equal(s_saved_blob, expected_blob, expected_len);
}

ZTEST(nfc_reader_classic_store, test_store_save_reader_captured_envelope)
{
	const nfc_service_t *svcs[] = { &s_svc };
	uint16_t crc_stored;
	uint16_t crc_calc;
	int ret;

	load_golden_model(&s_model);
	ret = nfc_store_save("clone", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_true(s_saved_len >= NFC_STORE_ENVELOPE_OVERHEAD);
	zassert_equal(s_saved_blob[0], NFC_STORE_BLOB_MAGIC_0);
	zassert_equal(s_saved_blob[1], NFC_STORE_BLOB_MAGIC_1);
	zassert_equal(s_saved_blob[2], NFC_STORE_BLOB_VERSION);
	zassert_equal(s_saved_blob[6], NFC_PERSIST_ID_CLASSIC);
	zassert_equal(s_saved_blob[7], NFC_STORE_ENTRY_FLAG_READER_CAPTURED);

	crc_stored = (uint16_t)s_saved_blob[s_saved_len - 2U] |
		     ((uint16_t)s_saved_blob[s_saved_len - 1U] << 8U);
	crc_calc = crc16_ccitt(0xFFFFU, s_saved_blob, s_saved_len - NFC_STORE_BLOB_CRC_SIZE);
	zassert_equal(crc_calc, crc_stored);
}

ZTEST(nfc_reader_classic_store, test_store_peek_golden_mfclassic_1k_4b)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	load_golden_blob(store_fixture_mfclassic_1k_4b_card,
			 STORE_FIXTURE_MFCLASSIC_1K_4B_CARD_LEN);
	ret = nfc_store_peek_entry_meta("golden", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_CLASSIC);
	zassert_equal(flags, NFC_STORE_ENTRY_FLAG_HAND_AUTHORED);
}

ZTEST(nfc_reader_classic_store, test_store_peek_clone_only_no_emulate)
{
	uint8_t persist_id = NFC_PERSIST_ID_CLASSIC;
	uint8_t flags = NFC_STORE_ENTRY_FLAG_READER_CAPTURED;

	zassert_equal(nfc_applet_check_emulate(persist_id, flags), -ENOTSUP);
}
