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
#include "desfire_store_fixture.h"
#include "emv_store_fixture.h"
#include "aliro_store_fixture.h"

#include "desfire_listener.h"
#include "desfire_poller.h"
#include "desfire.h"
#include "emv_listener.h"
#include "emv_poller.h"
#include "emv.h"
#include "aliro_listener.h"
#include "aliro_poller.h"
#include "aliro.h"
#include "aliro_vectors.h"

#include "nfc_virtual_loopback.h"

#include "Ultralight_11_mock.h"
#include "Ultralight_21_mock.h"
#include "Ultralight_C_mock.h"
#include "Ntag213_locked_mock.h"
#include "Ntag215_mock.h"
#include "Ntag216_mock.h"
#include "Desfire_mock.h"

#include <string.h>

#include <zephyr/ztest.h>

static int store_fixture_first_entry_body(const uint8_t *blob, size_t blob_len,
					  const uint8_t **body, uint16_t *body_len)
{
	uint16_t payload_len;
	uint16_t entry_len;

	if (blob_len < (NFC_STORE_ENVELOPE_OVERHEAD + NFC_STORE_ENTRY_OVERHEAD)) {
		return -EBADMSG;
	}
	if ((blob[0] != NFC_STORE_BLOB_MAGIC_0) || (blob[1] != NFC_STORE_BLOB_MAGIC_1)) {
		return -EBADMSG;
	}

	payload_len = (uint16_t)blob[4] | ((uint16_t)blob[5] << 8U);
	if ((size_t)(NFC_STORE_BLOB_HDR_SIZE + payload_len + NFC_STORE_BLOB_CRC_SIZE) != blob_len) {
		return -EBADMSG;
	}

	entry_len = (uint16_t)blob[8] | ((uint16_t)blob[9] << 8U);
	if ((size_t)(NFC_STORE_BLOB_HDR_SIZE + NFC_STORE_ENTRY_OVERHEAD + entry_len +
		      NFC_STORE_BLOB_CRC_SIZE) > blob_len) {
		return -EBADMSG;
	}

	*body = &blob[NFC_STORE_BLOB_HDR_SIZE + NFC_STORE_ENTRY_OVERHEAD];
	*body_len = entry_len;
	return 0;
}

static int emv_expected_from_golden(emv_card_image_t *expected)
{
	const uint8_t *body;
	uint16_t body_len;
	int ret;

	ret = store_fixture_first_entry_body(store_fixture_emv_card, STORE_FIXTURE_EMV_CARD_LEN,
					     &body, &body_len);
	if (ret != 0) {
		return ret;
	}

	return emv_deserialize(expected, body, body_len);
}

static ndef_data_t s_read;
static ndef_data_t s_clone_model;
static ndef_data_t s_expected;

static desfire_data_t s_desfire_clone;
static emv_card_image_t s_emv_clone;
static aliro_data_t s_aliro_clone;

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

	(void)ndef_listener_shutdown();
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
	(void)nfc_virtual_loopback_init();
	(void)ultralight_listener_shutdown();
	(void)ndef_listener_shutdown();
	(void)desfire_listener_shutdown();
	(void)emv_listener_shutdown();
	(void)aliro_listener_shutdown();
	ndef_data_reset(&s_read);
	ndef_data_reset(&s_clone_model);
	ndef_data_reset(&s_expected);
	desfire_data_reset(&s_desfire_clone);
	emv_card_image_reset(&s_emv_clone);
	aliro_data_reset(&s_aliro_clone);
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

/* ISO-DEP partial protocols — listener vtable loopback (cookbook §5.4–§5.6). */

static int desfire_clone_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return desfire_serialize(&s_desfire_clone, out, out_max, out_len);
}

static int desfire_clone_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return desfire_deserialize(&s_desfire_clone, in, in_len);
}

static nfc_service_t s_desfire_clone_svc = {
	.serialize = desfire_clone_serialize,
	.deserialize = desfire_clone_deserialize,
	.persist_id = NFC_PERSIST_ID_DESFIRE,
};

static int desfire_listener_setup(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)desfire_listener_shutdown();
	return desfire_listener_init(NULL);
}

static void desfire_listener_teardown(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)desfire_listener_shutdown();
}

static int desfire_copy_read(void *save_model, const void *read_out, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	*(desfire_data_t *)save_model = *(const desfire_data_t *)read_out;
	return 0;
}

static int desfire_compare(const void *expected, const void *actual, void *user_ctx)
{
	const desfire_data_t *exp = expected;
	const desfire_data_t *act = actual;

	ARG_UNUSED(user_ctx);

	if ((exp == NULL) || (act == NULL)) {
		return -EINVAL;
	}

	if (memcmp(exp->uid, act->uid, DESFIRE_UID_SIZE) != 0) {
		return -EBADMSG;
	}
	if (exp->free_memory != act->free_memory) {
		return -EBADMSG;
	}
	if (exp->master_key_settings != act->master_key_settings) {
		return -EBADMSG;
	}
	if (exp->app_count != act->app_count) {
		return -EBADMSG;
	}

	for (uint8_t ai = 0U; ai < exp->app_count; ai++) {
		const desfire_application_t *exp_app = &exp->apps[ai];
		const desfire_application_t *act_app = &act->apps[ai];

		if (memcmp(exp_app->app_id, act_app->app_id, DESFIRE_APP_ID_SIZE) != 0) {
			return -EBADMSG;
		}
		if (exp_app->file_count != act_app->file_count) {
			return -EBADMSG;
		}

		for (uint8_t fi = 0U; fi < exp_app->file_count; fi++) {
			if (exp_app->file_ids[fi] != act_app->file_ids[fi]) {
				return -EBADMSG;
			}
			if (exp_app->file_data_len[fi] != act_app->file_data_len[fi]) {
				return -EBADMSG;
			}
			if (memcmp(exp_app->file_data[fi], act_app->file_data[fi],
				   exp_app->file_data_len[fi]) != 0) {
				return -EBADMSG;
			}
		}
	}

	return 0;
}

static int desfire_poller_detect_wrap(nfc_reader_session_t *session)
{
	return desfire_poller_detect(session);
}

static int desfire_poller_read_wrap(nfc_reader_session_t *session, void *read_out)
{
	return desfire_poller_read(session, read_out);
}

/*
 * DESFire loopback uses static storage to avoid stack overflow that corrupts
 * ztest state when CONFIG_SHELL=n reduces the memory layout. The desfire_data_t
 * structure is ~450 bytes; two on stack caused corruption.
 */
static desfire_data_t s_desfire_loopback;

/* EMV / Aliro loopback: static storage avoids stack overflow with CONFIG_SHELL=n. */
static emv_card_image_t s_emv_loopback_read;
static emv_card_image_t s_emv_loopback_expected;
static aliro_data_t s_aliro_loopback_read;
static aliro_data_t s_aliro_loopback_expected;
static aliro_data_t s_aliro_auth_read;
static aliro_data_t s_aliro_auth_expected;

ZTEST(nfc_reader_loopback, test_virtual_loopback_desfire)
{
	int ret;

	desfire_data_reset(&s_desfire_loopback);
	ret = desfire_deserialize(&s_desfire_loopback, desfire_Desfire_model,
				  DESFIRE_DESFIRE_MODEL_LEN);
	zassert_ok(ret, "desfire_deserialize failed: %d", ret);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = desfire_listener_get(),
		.save_svc = &s_desfire_clone_svc,
		.golden_blob = store_fixture_desfire_card,
		.golden_blob_len = STORE_FIXTURE_DESFIRE_CARD_LEN,
		.golden_slot = "golden_desfire",
		.output_slot = "cloned_desfire",
		.poller_detect = desfire_poller_detect_wrap,
		.poller_read = desfire_poller_read_wrap,
		.read_out = &s_desfire_clone,
		.listener_setup = desfire_listener_setup,
		.listener_teardown = desfire_listener_teardown,
		.save_model = &s_desfire_clone,
		.copy_read_to_save = desfire_copy_read,
		.expected = &s_desfire_loopback,
		.compare = desfire_compare,
		.verify_envelope = true,
	});
	zassert_ok(ret, "nfc_virtual_loopback_run failed: %d", ret);
}

static int emv_clone_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return emv_serialize(&s_emv_clone, out, out_max, out_len);
}

static int emv_clone_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return emv_deserialize(&s_emv_clone, in, in_len);
}

static nfc_service_t s_emv_clone_svc = {
	.serialize = emv_clone_serialize,
	.deserialize = emv_clone_deserialize,
	.persist_id = NFC_PERSIST_ID_EMV,
};

static int emv_listener_setup(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)emv_listener_shutdown();
	return emv_listener_init(NULL);
}

static void emv_listener_teardown(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)emv_listener_shutdown();
}

static int emv_copy_read(void *save_model, const void *read_out, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	*(emv_card_image_t *)save_model = *(const emv_card_image_t *)read_out;
	return 0;
}

static int __maybe_unused emv_compare(const void *expected, const void *actual, void *user_ctx)
{
	const emv_card_image_t *exp = expected;
	const emv_card_image_t *act = actual;

	ARG_UNUSED(user_ctx);

	if ((exp == NULL) || (act == NULL)) {
		return -EINVAL;
	}

	if ((exp->app_aid_len != act->app_aid_len) ||
	    (memcmp(exp->app_aid, act->app_aid, exp->app_aid_len) != 0)) {
		return -EBADMSG;
	}

	if (exp->pan_len != act->pan_len) {
		return -EBADMSG;
	}
	if (exp->pan_len > 0U) {
		if (memcmp(exp->pan, act->pan, EMV_PAN_BYTES) != 0) {
			return -EBADMSG;
		}
	}

	if ((exp->track2_len != act->track2_len) ||
	    (memcmp(exp->track2, act->track2, exp->track2_len) != 0)) {
		return -EBADMSG;
	}

	if (memcmp(exp->afl, act->afl, EMV_AFL_BYTES) != 0) {
		return -EBADMSG;
	}

	if (exp->record_count != act->record_count) {
		return -EBADMSG;
	}

	for (uint8_t i = 0U; i < exp->record_count; i++) {
		if (exp->record_len[i] != act->record_len[i]) {
			return -EBADMSG;
		}
		if (memcmp(exp->record_data[i], act->record_data[i], exp->record_len[i]) != 0) {
			return -EBADMSG;
		}
	}

	return 0;
}

static int emv_poller_detect_wrap(nfc_reader_session_t *session)
{
	return emv_poller_detect(session);
}

static int emv_poller_read_wrap(nfc_reader_session_t *session, void *read_out)
{
	return emv_poller_read(session, read_out);
}

static int aliro_poller_detect_wrap(nfc_reader_session_t *session)
{
	return aliro_poller_detect(session);
}

static int aliro_poller_read_wrap(nfc_reader_session_t *session, void *read_out)
{
	return aliro_poller_read(session, read_out);
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_emv)
{
	int ret;

	emv_card_image_reset(&s_emv_loopback_read);
	zassert_ok(emv_expected_from_golden(&s_emv_loopback_expected), "emv golden deserialize");

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = emv_listener_get(),
		.save_svc = &s_emv_clone_svc,
		.golden_blob = store_fixture_emv_card,
		.golden_blob_len = STORE_FIXTURE_EMV_CARD_LEN,
		.golden_slot = "golden_emv",
		.output_slot = "cloned_emv",
		.poller_detect = emv_poller_detect_wrap,
		.poller_read = emv_poller_read_wrap,
		.read_out = &s_emv_loopback_read,
		.listener_setup = emv_listener_setup,
		.listener_teardown = emv_listener_teardown,
		.save_model = &s_emv_clone,
		.copy_read_to_save = emv_copy_read,
		.expected = &s_emv_loopback_expected,
		.compare = NULL,
		.verify_envelope = true,
	});
	zassert_ok(ret);
}

static int aliro_clone_serialize(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return aliro_serialize(&s_aliro_clone, out, out_max, out_len);
}

static int aliro_clone_deserialize(const uint8_t *in, size_t in_len, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	return aliro_deserialize(&s_aliro_clone, in, in_len);
}

static nfc_service_t s_aliro_clone_svc = {
	.serialize = aliro_clone_serialize,
	.deserialize = aliro_clone_deserialize,
	.persist_id = NFC_PERSIST_ID_ALIRO,
};

static int aliro_listener_setup(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)aliro_listener_shutdown();
	return aliro_listener_init(NULL);
}

static void aliro_listener_teardown(void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	(void)aliro_listener_shutdown();
}

static int aliro_copy_read(void *save_model, const void *read_out, void *user_ctx)
{
	ARG_UNUSED(user_ctx);

	*(aliro_data_t *)save_model = *(const aliro_data_t *)read_out;
	return 0;
}

static int aliro_compare(const void *expected, const void *actual, void *user_ctx)
{
	const aliro_data_t *exp = expected;
	const aliro_data_t *act = actual;

	ARG_UNUSED(user_ctx);

	if ((exp == NULL) || (act == NULL)) {
		return -EINVAL;
	}

	if (exp->transcript_len != act->transcript_len) {
		return -EBADMSG;
	}
	if (exp->transcript_len > 0U) {
		if (memcmp(exp->transcript, act->transcript, exp->transcript_len) != 0) {
			return -EBADMSG;
		}
	}

	return 0;
}

static int aliro_compare_auth_chain(const void *expected, const void *actual,
						   void *user_ctx)
{
	const aliro_data_t *exp = expected;
	const aliro_data_t *act = actual;
	int ret;

	ret = aliro_compare(expected, actual, user_ctx);
	if (ret != 0) {
		return ret;
	}

	if ((exp->protocol_major != act->protocol_major) ||
	    (exp->protocol_minor != act->protocol_minor) || (exp->features != act->features)) {
		return -EBADMSG;
	}

	if (memcmp(exp->credential_pubkey, act->credential_pubkey, ALIRO_P256_PUBLIC_KEY_SIZE) != 0) {
		return -EBADMSG;
	}

	return 0;
}

static void build_aliro_poller_expected(aliro_data_t *expected)
{
	size_t chunk_len = 0U;
	size_t offset = 0U;

	aliro_data_reset(expected);
	expected->protocol_major = 0U;
	expected->protocol_minor = 9U;
	aliro_vectors_build_select_rsp(&expected->transcript[offset], &chunk_len);
	offset += chunk_len;
	aliro_vectors_build_auth0_rsp(&expected->transcript[offset], &chunk_len);
	offset += chunk_len;
	aliro_vectors_build_auth1_rsp(&expected->transcript[offset], &chunk_len);
	offset += chunk_len;
	expected->transcript_len = (uint16_t)offset;
}

static void build_aliro_poller_expected_auth_chain(aliro_data_t *expected)
{
	build_aliro_poller_expected(expected);
	expected->credential_pubkey[0] = 0x04U;
	aliro_vectors_fill(&expected->credential_pubkey[1], ALIRO_P256_PUBLIC_KEY_SIZE - 1U,
			   ALIRO_TEST_CARD_PUBKEY_FILL);
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_aliro)
{
	int ret;

	aliro_data_reset(&s_aliro_loopback_read);
	build_aliro_poller_expected(&s_aliro_loopback_expected);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = aliro_listener_get(),
		.save_svc = &s_aliro_clone_svc,
		.golden_blob = store_fixture_aliro_card,
		.golden_blob_len = STORE_FIXTURE_ALIRO_CARD_LEN,
		.golden_slot = "golden_aliro",
		.output_slot = "cloned_aliro",
		.poller_detect = aliro_poller_detect_wrap,
		.poller_read = aliro_poller_read_wrap,
		.read_out = &s_aliro_loopback_read,
		.listener_setup = aliro_listener_setup,
		.listener_teardown = aliro_listener_teardown,
		.save_model = &s_aliro_clone,
		.copy_read_to_save = aliro_copy_read,
		.expected = &s_aliro_loopback_expected,
		.compare = aliro_compare,
		.verify_envelope = true,
	});
	zassert_ok(ret);
}

ZTEST(nfc_reader_loopback, test_virtual_loopback_aliro_auth_chain)
{
	int ret;

	aliro_data_reset(&s_aliro_auth_read);
	build_aliro_poller_expected_auth_chain(&s_aliro_auth_expected);

	ret = nfc_virtual_loopback_run(&(nfc_virtual_loopback_params_t){
		.listener_svc = aliro_listener_get(),
		.save_svc = &s_aliro_clone_svc,
		.golden_blob = store_fixture_aliro_card,
		.golden_blob_len = STORE_FIXTURE_ALIRO_CARD_LEN,
		.golden_slot = "golden_aliro_ch",
		.output_slot = "cloned_aliro_ch",
		.poller_detect = aliro_poller_detect_wrap,
		.poller_read = aliro_poller_read_wrap,
		.read_out = &s_aliro_auth_read,
		.listener_setup = aliro_listener_setup,
		.listener_teardown = aliro_listener_teardown,
		.save_model = &s_aliro_clone,
		.copy_read_to_save = aliro_copy_read,
		.expected = &s_aliro_auth_expected,
		.compare = aliro_compare_auth_chain,
		.verify_envelope = true,
	});
	zassert_ok(ret, "aliro auth chain ret=%d", ret);
}
