/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E+ — virtual RF loopback: load → emulate → poller read → save → compare.
 * Flipper analogue: mf_ultralight_reader_test — virtual RF on QEMU
 * (NDEF has no Flipper protocols/ndef/; provenance: cookbook §5.1 + NXP RW_NDEF_T4T)
 */

#include "applets/nfc_applet.h"
#include "ndef_listener.h"
#include "ndef_poller.h"
#include "protocols/ndef/ndef.h"
#include "store/nfc_store.h"
#include "store_fixture.h"

#include "nfc_session_mock.h"
#include "nfc_virtual_rf.h"

#include <errno.h>
#include <string.h>

#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

static ndef_data_t s_read;
static ndef_data_t s_clone_model;
static ndef_data_t s_expected;
static uint8_t s_saved_blob[512];
static size_t s_saved_len;
static nfc_reader_session_t s_session;

static int clone_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return ndef_serialize(&s_clone_model, out, out_max, out_len);
}

static int clone_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return ndef_deserialize(&s_clone_model, in, in_len);
}

static nfc_service_t s_clone_svc = {
	.serialize = clone_serialize,
	.deserialize = clone_deserialize,
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
	uint16_t mle = (uint16_t)(CONFIG_NFC_NDEF_MAX_SIZE + NFC_NDEF_NLEN_FIELD_LEN);
	uint16_t mlc = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE;
	uint16_t max_ndef = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE;

	ndef_data_reset(data);
	data->cc[0] = 0x00U;
	data->cc[1] = NFC_NDEF_CC_LEN;
	data->cc[2] = 0x20U;
	data->cc[3] = (uint8_t)(mle >> 8U);
	data->cc[4] = (uint8_t)(mle & 0xFFU);
	data->cc[5] = (uint8_t)(mlc >> 8U);
	data->cc[6] = (uint8_t)(mlc & 0xFFU);
	data->cc[7] = 0x04U;
	data->cc[8] = 0x06U;
	data->cc[9] = 0xE1U;
	data->cc[10] = 0x04U;
	data->cc[11] = (uint8_t)(max_ndef >> 8U);
	data->cc[12] = (uint8_t)(max_ndef & 0xFFU);
	data->cc[13] = 0x00U;
	data->cc[14] = 0xFFU;
	data->cc_len = NFC_NDEF_CC_LEN;
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = 0U;
	data->ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;
}

static void *suite_setup(void)
{
	s_saved_len = 0U;
	nfc_session_mock_reset();
	nfc_virtual_rf_disable();
	(void)nfc_store_init(NULL);
	(void)nfc_store_register_save_cb(mock_save_cb, NULL);
	(void)nfc_store_register_load_cb(mock_load_cb, NULL);
	return NULL;
}

static void loopback_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	nfc_virtual_rf_disable();
	ndef_listener_shutdown();
	ndef_data_reset(&s_read);
	ndef_data_reset(&s_clone_model);
	s_saved_len = 0U;
	s_session.active = false;
}

ZTEST_SUITE(nfc_reader_loopback, NULL, suite_setup, loopback_before, NULL, NULL);

ZTEST(nfc_reader_loopback, test_virtual_loopback_empty_card)
{
	const nfc_service_t *listener = ndef_listener_get();
	const nfc_service_t *load_svcs[] = { listener };
	const nfc_service_t *save_svcs[] = { &s_clone_svc };
	uint16_t crc_stored;
	uint16_t crc_calc;
	int ret;

	/* 1. Load golden envelope → listener model (emulate path). */
	zassert_ok(ndef_listener_init(NULL, &(ndef_listener_config_t){ .writable = false }));
	load_golden_blob(store_fixture_ndef_empty_card, STORE_FIXTURE_NDEF_EMPTY_CARD_LEN);
	ret = nfc_store_load("golden", load_svcs, ARRAY_SIZE(load_svcs));
	zassert_ok(ret);

	/* 2. Virtual RF: poller TX routed to listener on_apdu / on_select. */
	zassert_ok(nfc_virtual_rf_enable(listener));

	/* 3. Active reader session (discover OK — no HAL on QEMU). */
	s_session.active = true;

	/* 4. Poller detect + read → ndef_data_t. */
	zassert_ok(ndef_poller_detect(&s_session));
	ret = ndef_poller_read(&s_session, &s_read);
	zassert_ok(ret);

	/* 5. Save cloned read via store envelope. */
	set_empty_ndef_model(&s_expected);
	s_clone_model = s_read;
	ret = nfc_store_save("cloned", save_svcs, ARRAY_SIZE(save_svcs));
	zassert_ok(ret);

	/* 6. Model fields match poller read + envelope valid (golden uses zero CC placeholder). */
	zassert_ok(nfc_applet_verify_compare(&s_expected, &s_read, NULL, NULL));
	zassert_true(s_saved_len >= NFC_STORE_ENVELOPE_OVERHEAD);
	zassert_equal(s_saved_blob[0], NFC_STORE_BLOB_MAGIC_0);
	zassert_equal(s_saved_blob[1], NFC_STORE_BLOB_MAGIC_1);
	zassert_equal(s_saved_blob[2], NFC_STORE_BLOB_VERSION);
	crc_stored = (uint16_t)s_saved_blob[s_saved_len - 2U] |
		     ((uint16_t)s_saved_blob[s_saved_len - 1U] << 8U);
	crc_calc = crc16_ccitt(0xFFFFU, s_saved_blob, s_saved_len - NFC_STORE_BLOB_CRC_SIZE);
	zassert_equal(crc_calc, crc_stored);

	nfc_virtual_rf_disable();
}
