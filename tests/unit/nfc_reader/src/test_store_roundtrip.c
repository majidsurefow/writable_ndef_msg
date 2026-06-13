/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — nfc_store envelope save/load roundtrip (cookbook §14.6a).
 */

#include "applets/nfc_applet.h"
#include "protocols/ndef/ndef.h"
#include "store/nfc_store.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

static ndef_data_t s_model;
static uint8_t s_saved_blob[512];
static size_t s_saved_len;

static int mock_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return ndef_serialize(&s_model, out, out_max, out_len);
}

static int mock_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return ndef_deserialize(&s_model, in, in_len);
}

static nfc_service_t s_svc = {
	.serialize = mock_serialize,
	.deserialize = mock_deserialize,
	.persist_id = NFC_PERSIST_ID_NDEF,
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

static void fill_fixture_model(ndef_data_t *data)
{
	static const uint8_t uri[] = {0xD1U, 0x01U, 0x03U, 0x55U, 0x01U};

	ndef_data_reset(data);
	for (uint16_t i = 0U; i < NFC_NDEF_CC_LEN; i++) {
		data->cc[i] = (uint8_t)(0xE1U + (i % 3U));
	}
	data->cc_len = NFC_NDEF_CC_LEN;
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = (uint8_t)sizeof(uri);
	(void)memcpy(&data->ndef_file[2], uri, sizeof(uri));
	data->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + sizeof(uri));
}

static void *suite_setup(void)
{
	s_saved_len = 0U;
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(mock_save_cb, NULL);
	(void)nfc_store_register_load_cb(mock_load_cb, NULL);
	return NULL;
}

ZTEST_SUITE(nfc_reader_store, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_store, test_store_save_envelope)
{
	const nfc_service_t *svcs[] = { &s_svc };
	uint16_t crc_stored;
	uint16_t crc_calc;
	int ret;

	fill_fixture_model(&s_model);
	ret = nfc_store_save("demo", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_true(s_saved_len >= NFC_STORE_ENVELOPE_OVERHEAD);
	zassert_equal(s_saved_blob[0], NFC_STORE_BLOB_MAGIC_0);
	zassert_equal(s_saved_blob[1], NFC_STORE_BLOB_MAGIC_1);
	zassert_equal(s_saved_blob[2], NFC_STORE_BLOB_VERSION);

	crc_stored = (uint16_t)s_saved_blob[s_saved_len - 2U] |
		     ((uint16_t)s_saved_blob[s_saved_len - 1U] << 8U);
	crc_calc = crc16_ccitt(0xFFFFU, s_saved_blob, s_saved_len - NFC_STORE_BLOB_CRC_SIZE);
	zassert_equal(crc_calc, crc_stored);
}

ZTEST(nfc_reader_store, test_store_load_roundtrip)
{
	const nfc_service_t *svcs[] = { &s_svc };
	ndef_data_t loaded;
	int ret;

	fill_fixture_model(&s_model);
	ret = nfc_store_save("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ndef_data_reset(&s_model);
	ret = nfc_store_load("round", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	loaded = s_model;
	fill_fixture_model(&s_model);
	zassert_mem_equal(&loaded.cc, &s_model.cc, NFC_NDEF_CC_LEN);
	zassert_equal(loaded.ndef_file_len, s_model.ndef_file_len);
	zassert_mem_equal(loaded.ndef_file, s_model.ndef_file, loaded.ndef_file_len);
}
