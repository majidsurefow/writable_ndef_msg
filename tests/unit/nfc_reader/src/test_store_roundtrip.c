/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E — nfc_store envelope save/load roundtrip (cookbook §14.6a).
 */

#include "applets/nfc_applet_policy.h"
#include "hal/nfc_transport.h"
#include "protocols/ndef/ndef.h"
#include "store/nfc_store.h"
#if IS_ENABLED(CONFIG_NFC_STORE_RAM)
#include "store/nfc_store_ram.h"
#endif
#include "store_fixture.h"

#include <errno.h>
#include <stdio.h>
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

static void load_golden_blob(const uint8_t *blob, size_t len)
{
	zassert_true(len <= sizeof(s_saved_blob));
	(void)memcpy(s_saved_blob, blob, len);
	s_saved_len = len;
}

static void set_empty_ndef_model(ndef_data_t *data)
{
	ndef_data_reset(data);
	data->cc_len = NFC_NDEF_CC_LEN;
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = 0U;
	data->ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;
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

static const nfc_transport_caps_t s_test_hal_caps = {
	.roles = NFC_ROLE_READER | NFC_ROLE_LISTEN,
	.technologies = NFC_TECH_ALL_READER,
	.tier = NFC_TIER_SMART,
};

const nfc_transport_caps_t *nfc_transport_get_capabilities(void)
{
	return &s_test_hal_caps;
}

static void register_mock_hooks(void)
{
	(void)nfc_store_register_save_cb(mock_save_cb, NULL);
	(void)nfc_store_register_load_cb(mock_load_cb, NULL);
}

#if IS_ENABLED(CONFIG_NFC_STORE_RAM)
static void store_hooks_rule_before(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(data);

	if (strcmp(test->test_suite_name, "nfc_reader_store") != 0) {
		return;
	}

	if (strncmp(test->name, "test_store_ram_", 15) == 0) {
		return;
	}

	s_saved_len = 0U;
	register_mock_hooks();
}

ZTEST_RULE(store_hooks_rule, store_hooks_rule_before, NULL);
#endif /* CONFIG_NFC_STORE_RAM */

static void *suite_setup(void)
{
	s_saved_len = 0U;
	(void)nfc_store_init(NULL);
	register_mock_hooks();
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

ZTEST(nfc_reader_store, test_store_load_golden_empty_card)
{
	const nfc_service_t *svcs[] = { &s_svc };
	ndef_data_t expected;
	ndef_data_t loaded;
	int ret;

	set_empty_ndef_model(&expected);
	load_golden_blob(store_fixture_ndef_empty_card, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);

	ndef_data_reset(&s_model);
	ret = nfc_store_load("golden", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	loaded = s_model;
	zassert_mem_equal(&loaded.cc, &expected.cc, NFC_NDEF_CC_LEN);
	zassert_equal(loaded.cc_len, expected.cc_len);
	zassert_equal(loaded.ndef_file_len, expected.ndef_file_len);
	zassert_mem_equal(loaded.ndef_file, expected.ndef_file, loaded.ndef_file_len);
}

ZTEST(nfc_reader_store, test_store_save_matches_golden_empty_card)
{
	const nfc_service_t *svcs[] = { &s_svc };
	int ret;

	set_empty_ndef_model(&s_model);
	ret = nfc_store_save("empty", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_saved_len, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);
	zassert_mem_equal(s_saved_blob, store_fixture_ndef_empty_card,
			  STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);
}

ZTEST(nfc_reader_store, test_store_peek_golden_empty_card)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	load_golden_blob(store_fixture_ndef_empty_card, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);
	ret = nfc_store_peek_entry_meta("golden", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_NDEF);
	zassert_equal(flags, NFC_STORE_ENTRY_FLAG_READER_CAPTURED |
			       NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE);
}

ZTEST(nfc_reader_store, test_store_peek_missing_slot)
{
	uint8_t persist_id;
	uint8_t flags;

	s_saved_len = 0U;
	zassert_equal(nfc_store_peek_entry_meta("missing", &persist_id, &flags), -ENOENT);
}

ZTEST(nfc_reader_store, test_store_peek_rejects_read_only_partial)
{
	uint8_t persist_id = NFC_PERSIST_ID_NDEF;
	uint8_t flags = NFC_STORE_ENTRY_FLAG_READER_CAPTURED |
			NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL;

	zassert_equal(nfc_applet_check_emulate(persist_id, flags), -ENOTSUP);
}

#if IS_ENABLED(CONFIG_NFC_STORE_RAM)

ZTEST(nfc_reader_store, test_store_ram_save_load_roundtrip)
{
	const nfc_service_t *svcs[] = { &s_svc };
	ndef_data_t loaded;
	int ret;

	nfc_store_ram_reset();
	zassert_ok(nfc_store_ram_init());

	fill_fixture_model(&s_model);
	ret = nfc_store_save("ram1", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ndef_data_reset(&s_model);
	ret = nfc_store_load("ram1", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	loaded = s_model;
	fill_fixture_model(&s_model);
	zassert_mem_equal(&loaded.cc, &s_model.cc, NFC_NDEF_CC_LEN);
	zassert_equal(loaded.ndef_file_len, s_model.ndef_file_len);
	zassert_mem_equal(loaded.ndef_file, s_model.ndef_file, loaded.ndef_file_len);
}

ZTEST(nfc_reader_store, test_store_ram_multiple_slots)
{
	const nfc_service_t *svcs[] = { &s_svc };
	int ret;

	if (CONFIG_NFC_STORE_RAM_SLOT_COUNT < 2) {
		ztest_test_skip();
	}

	nfc_store_ram_reset();
	zassert_ok(nfc_store_ram_init());

	fill_fixture_model(&s_model);
	ret = nfc_store_save("a", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	fill_fixture_model(&s_model);
	ret = nfc_store_save("b", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ndef_data_reset(&s_model);
	ret = nfc_store_load("b", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.ndef_file_len, (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + 5U));

	ndef_data_reset(&s_model);
	ret = nfc_store_load("a", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);
	zassert_equal(s_model.ndef_file_len, (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + 5U));
}

ZTEST(nfc_reader_store, test_store_ram_peek_emulate_ready)
{
	const nfc_service_t *svcs[] = { &s_svc };
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	nfc_store_ram_reset();
	zassert_ok(nfc_store_ram_init());

	set_empty_ndef_model(&s_model);
	ret = nfc_store_save("em1", svcs, ARRAY_SIZE(svcs));
	zassert_ok(ret);

	ret = nfc_store_peek_entry_meta("em1", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_NDEF);
	zassert_equal(nfc_applet_check_emulate(persist_id, flags), 0);
}

ZTEST(nfc_reader_store, test_store_ram_missing_slot)
{
	nfc_store_ram_reset();
	zassert_ok(nfc_store_ram_init());
	zassert_equal(nfc_store_ram_load_cb("missing", s_saved_blob, sizeof(s_saved_blob),
					    &s_saved_len, NULL),
		      -ENOENT);
}

ZTEST(nfc_reader_store, test_store_ram_slot_full)
{
	const nfc_service_t *svcs[] = { &s_svc };
	char tag[CONFIG_NFC_STORE_MAX_TAG_LEN];
	int ret;

	nfc_store_ram_reset();
	zassert_ok(nfc_store_ram_init());

	set_empty_ndef_model(&s_model);
	for (int i = 0; i < CONFIG_NFC_STORE_RAM_SLOT_COUNT; i++) {
		(void)snprintk(tag, sizeof(tag), "s%d", i);
		ret = nfc_store_save(tag, svcs, ARRAY_SIZE(svcs));
		zassert_ok(ret, "save slot %d failed: %d", i, ret);
	}

	ret = nfc_store_save("overflow", svcs, ARRAY_SIZE(svcs));
	zassert_equal(ret, -EIO);
}

#endif /* CONFIG_NFC_STORE_RAM */
