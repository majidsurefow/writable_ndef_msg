/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E+ — virtual RF loopback via nfc_virtual_loopback harness.
 */

#include "applets/nfc_applet.h"
#include "ndef_listener.h"
#include "ndef_poller.h"
#include "protocols/ndef/ndef.h"
#include "protocols/ultralight/ultralight.h"
#include "protocols/ultralight/ultralight_listener.h"
#include "store/nfc_store.h"
#include "store_fixture.h"
#include "ultralight_store_fixture.h"

#include "nfc_virtual_loopback.h"

#include "Ultralight_11_mock.h"
#include "Ultralight_21_mock.h"
#include "Ultralight_C_mock.h"
#include "Ntag213_locked_mock.h"
#include "Ntag215_mock.h"
#include "Ntag216_mock.h"

#include <string.h>

#include <zephyr/ztest.h>

static ndef_data_t s_read;
static ndef_data_t s_clone_model;
static ndef_data_t s_expected;

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

static int ndef_listener_setup(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return ndef_listener_init(NULL, &(ndef_listener_config_t){ .writable = false });
}

static void ndef_listener_teardown(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)ndef_listener_shutdown();
}

static int ndef_copy_read(void *save_model, const void *read_out, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	*(ndef_data_t *)save_model = *(const ndef_data_t *)read_out;
	return 0;
}

static int ndef_compare(const void *expected, const void *actual, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return nfc_applet_verify_compare(expected, actual, NULL, NULL);
}

static int ndef_poller_detect_wrap(nfc_reader_session_t *session)
{
	return ndef_poller_detect(session);
}

static int ndef_poller_read_wrap(nfc_reader_session_t *session, void *read_out)
{
	return ndef_poller_read(session, read_out);
}

static int ultralight_listener_setup(void *user_ctx)
{
	int ret;

	ARG_UNUSED(user_ctx);

	(void)ultralight_listener_shutdown();
	(void)ndef_listener_shutdown();

	ret = ultralight_listener_init();
	if (ret == -EALREADY) {
		ret = 0;
	}

	return ret;
}

static void ultralight_listener_teardown(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)ultralight_listener_shutdown();
}

static void fill_synthesized_cc(ndef_data_t *data)
{
	uint16_t mle = (uint16_t)(CONFIG_NFC_NDEF_MAX_SIZE + NFC_NDEF_NLEN_FIELD_LEN);
	uint16_t mlc = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE;
	uint16_t max_ndef = (uint16_t)CONFIG_NFC_NDEF_MAX_SIZE;

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
}

static void build_expected_ndef_from_ul_model(const uint8_t *model, size_t model_len,
					      ndef_data_t *expected)
{
	ultralight_data_t ul;
	const uint8_t *msg = NULL;
	size_t msg_len = 0U;

	ndef_data_reset(expected);
	fill_synthesized_cc(expected);

	zassert_ok(ultralight_deserialize(&ul, model, model_len));
	zassert_ok(ultralight_extract_ndef(&ul, &msg, &msg_len));

	if ((msg != NULL) && (msg_len > 0U)) {
		expected->ndef_file[0] = (uint8_t)(msg_len >> 8U);
		expected->ndef_file[1] = (uint8_t)(msg_len & 0xFFU);
		(void)memcpy(&expected->ndef_file[2], msg, msg_len);
		expected->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + msg_len);
	} else {
		expected->ndef_file[0] = 0U;
		expected->ndef_file[1] = 0U;
		expected->ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;
	}
}

static int ultralight_ndef_compare(const void *expected, const void *actual, void *user_ctx)
{
	const ndef_data_t *exp = expected;
	const ndef_data_t *act = actual;
	uint16_t exp_nlen;
	uint16_t act_nlen;

	ARG_UNUSED(user_ctx);

	if ((exp == NULL) || (act == NULL)) {
		return -EINVAL;
	}

	exp_nlen = (uint16_t)(((uint16_t)exp->ndef_file[0] << 8U) | (uint16_t)exp->ndef_file[1]);
	act_nlen = (uint16_t)(((uint16_t)act->ndef_file[0] << 8U) | (uint16_t)act->ndef_file[1]);

	if (exp_nlen != act_nlen) {
		return -EBADMSG;
	}

	if (exp_nlen > 0U) {
		if (memcmp(&exp->ndef_file[2], &act->ndef_file[2], exp_nlen) != 0) {
			return -EBADMSG;
		}
	}

	return 0;
}

static void run_ultralight_loopback(const uint8_t *golden, size_t golden_len,
				    const uint8_t *model, size_t model_len,
				    const char *golden_slot, const char *output_slot)
{
	int ret;

	build_expected_ndef_from_ul_model(model, model_len, &s_expected);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = ultralight_listener_get(),
		.save_svc = &s_clone_svc,
		.golden_blob = golden,
		.golden_blob_len = golden_len,
		.golden_slot = golden_slot,
		.output_slot = output_slot,
		.poller_detect = ndef_poller_detect_wrap,
		.poller_read = ndef_poller_read_wrap,
		.read_out = &s_read,
		.listener_setup = ultralight_listener_setup,
		.listener_teardown = ultralight_listener_teardown,
		.save_model = &s_clone_model,
		.copy_read_to_save = ndef_copy_read,
		.expected = &s_expected,
		.compare = ultralight_ndef_compare,
		.verify_envelope = true,
	});
	zassert_ok(ret);
}

static void set_empty_ndef_model(ndef_data_t *data)
{
	ndef_data_reset(data);
	fill_synthesized_cc(data);
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = 0U;
	data->ndef_file_len = NFC_NDEF_NLEN_FIELD_LEN;
}

static void set_uri_5byte_ndef_model(ndef_data_t *data)
{
	static const uint8_t uri[] = {0xD1U, 0x01U, 0x03U, 0x55U, 0x01U};

	ndef_data_reset(data);
	fill_synthesized_cc(data);
	data->ndef_file[0] = 0U;
	data->ndef_file[1] = (uint8_t)sizeof(uri);
	(void)memcpy(&data->ndef_file[2], uri, sizeof(uri));
	data->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + sizeof(uri));
}

static void set_chunk_255_ndef_model(ndef_data_t *data)
{
	ndef_data_reset(data);
	fill_synthesized_cc(data);
	data->ndef_file[0] = 0x01U;
	data->ndef_file[1] = 0x2CU;
	for (uint16_t i = 0U; i < 300U; i++) {
		data->ndef_file[2U + i] = (uint8_t)(i & 0xFFU);
	}
	data->ndef_file_len = (uint16_t)(NFC_NDEF_NLEN_FIELD_LEN + 300U);
}

static void *suite_setup(void)
{
	zassert_ok(nfc_virtual_loopback_init());
	return NULL;
}

static void loopback_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_virtual_loopback_reset();
	(void)ultralight_listener_shutdown();
	(void)ndef_listener_shutdown();
	ndef_data_reset(&s_read);
	ndef_data_reset(&s_clone_model);
	ndef_data_reset(&s_expected);
}

ZTEST_SUITE(nfc_reader_loopback, NULL, suite_setup, loopback_before, NULL, NULL);

ZTEST(nfc_reader_loopback, test_virtual_loopback_empty_card)
{
	const uint8_t *saved_blob;
	size_t saved_len;
	int ret;

	set_empty_ndef_model(&s_expected);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = ndef_listener_get(),
		.save_svc = &s_clone_svc,
		.golden_blob = store_fixture_ndef_empty_card,
		.golden_blob_len = STORE_FIXTURE_NDEF_EMPTY_CARD_LEN,
		.golden_slot = "golden",
		.output_slot = "cloned",
		.poller_detect = ndef_poller_detect_wrap,
		.poller_read = ndef_poller_read_wrap,
		.read_out = &s_read,
		.listener_setup = ndef_listener_setup,
		.listener_teardown = ndef_listener_teardown,
		.save_model = &s_clone_model,
		.copy_read_to_save = ndef_copy_read,
		.expected = &s_expected,
		.compare = ndef_compare,
		.verify_envelope = true,
	});
	zassert_ok(ret);

	zassert_ok(nfc_virtual_loopback_last_saved(&saved_blob, &saved_len));
	zassert_true(saved_len >= NFC_STORE_ENVELOPE_OVERHEAD);
	zassert_equal(saved_blob[0], NFC_STORE_BLOB_MAGIC_0);
	zassert_equal(saved_blob[1], NFC_STORE_BLOB_MAGIC_1);
	zassert_equal(saved_blob[2], NFC_STORE_BLOB_VERSION);
}

/* Flipper intent: listener_start → poller_sync_read → data equal (NDEF URI, cookbook §5.1). */
ZTEST(nfc_reader_loopback, test_virtual_loopback_uri_5byte)
{
	const uint8_t *saved_blob;
	size_t saved_len;
	int ret;

	set_uri_5byte_ndef_model(&s_expected);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = ndef_listener_get(),
		.save_svc = &s_clone_svc,
		.golden_blob = store_fixture_ndef_uri_5byte_card,
		.golden_blob_len = STORE_FIXTURE_NDEF_URI_5BYTE_CARD_LEN,
		.golden_slot = "golden_uri",
		.output_slot = "cloned_uri",
		.poller_detect = ndef_poller_detect_wrap,
		.poller_read = ndef_poller_read_wrap,
		.read_out = &s_read,
		.listener_setup = ndef_listener_setup,
		.listener_teardown = ndef_listener_teardown,
		.save_model = &s_clone_model,
		.copy_read_to_save = ndef_copy_read,
		.expected = &s_expected,
		.compare = ndef_compare,
		.verify_envelope = true,
	});
	zassert_ok(ret);

	zassert_ok(nfc_virtual_loopback_last_saved(&saved_blob, &saved_len));
	zassert_true(saved_len >= NFC_STORE_ENVELOPE_OVERHEAD);
	zassert_equal(saved_blob[0], NFC_STORE_BLOB_MAGIC_0);
	zassert_equal(saved_blob[1], NFC_STORE_BLOB_MAGIC_1);
	zassert_equal(saved_blob[2], NFC_STORE_BLOB_VERSION);
}

/* Flipper intent: chunked poller read through virtual RF (Tier B chunk_255 parity). */
ZTEST(nfc_reader_loopback, test_virtual_loopback_chunk_255)
{
	const uint8_t *saved_blob;
	size_t saved_len;
	int ret;

	set_chunk_255_ndef_model(&s_expected);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = ndef_listener_get(),
		.save_svc = &s_clone_svc,
		.golden_blob = store_fixture_ndef_chunk_255_card,
		.golden_blob_len = STORE_FIXTURE_NDEF_CHUNK_255_CARD_LEN,
		.golden_slot = "golden_chunk",
		.output_slot = "cloned_chunk",
		.poller_detect = ndef_poller_detect_wrap,
		.poller_read = ndef_poller_read_wrap,
		.read_out = &s_read,
		.listener_setup = ndef_listener_setup,
		.listener_teardown = ndef_listener_teardown,
		.save_model = &s_clone_model,
		.copy_read_to_save = ndef_copy_read,
		.expected = &s_expected,
		.compare = ndef_compare,
		.verify_envelope = true,
	});
	zassert_ok(ret);

	zassert_ok(nfc_virtual_loopback_last_saved(&saved_blob, &saved_len));
	zassert_true(saved_len >= NFC_STORE_ENVELOPE_OVERHEAD);
	zassert_equal(saved_blob[0], NFC_STORE_BLOB_MAGIC_0);
	zassert_equal(saved_blob[1], NFC_STORE_BLOB_MAGIC_1);
	zassert_equal(saved_blob[2], NFC_STORE_BLOB_VERSION);
}

/* Flipper Ultralight variants — T4 adapter emulate + ndef poller verify-back (§5.2). */
ZTEST(nfc_reader_loopback, test_virtual_loopback_ultralight_11)
{
	run_ultralight_loopback(store_fixture_ultralight_11_card,
				STORE_FIXTURE_ULTRALIGHT_11_CARD_LEN,
				ultralight_Ultralight_11_model,
				ULTRALIGHT_ULTRALIGHT_11_MODEL_LEN, "golden_ul11",
				"cloned_ul11");
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_ultralight_21)
{
	run_ultralight_loopback(store_fixture_ultralight_21_card,
				STORE_FIXTURE_ULTRALIGHT_21_CARD_LEN,
				ultralight_Ultralight_21_model,
				ULTRALIGHT_ULTRALIGHT_21_MODEL_LEN, "golden_ul21",
				"cloned_ul21");
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_ultralight_c)
{
	run_ultralight_loopback(store_fixture_ultralight_c_card,
				STORE_FIXTURE_ULTRALIGHT_C_CARD_LEN,
				ultralight_Ultralight_C_model,
				ULTRALIGHT_ULTRALIGHT_C_MODEL_LEN, "golden_ulc",
				"cloned_ulc");
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_ntag213_locked)
{
	run_ultralight_loopback(store_fixture_ntag213_locked_card,
				STORE_FIXTURE_NTAG213_LOCKED_CARD_LEN,
				ultralight_Ntag213_locked_model,
				ULTRALIGHT_NTAG213_LOCKED_MODEL_LEN, "golden_213",
				"cloned_213");
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_ntag215)
{
	run_ultralight_loopback(store_fixture_ntag215_card,
				STORE_FIXTURE_NTAG215_CARD_LEN, ultralight_Ntag215_model,
				ULTRALIGHT_NTAG215_MODEL_LEN, "golden_215", "cloned_215");
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_ntag216)
{
	run_ultralight_loopback(store_fixture_ntag216_card,
				STORE_FIXTURE_NTAG216_CARD_LEN, ultralight_Ntag216_model,
				ULTRALIGHT_NTAG216_MODEL_LEN, "golden_216", "cloned_216");
}
