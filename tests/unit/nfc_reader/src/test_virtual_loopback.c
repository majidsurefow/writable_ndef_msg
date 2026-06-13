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
#include "store/nfc_store.h"
#include "store_fixture.h"

#include "nfc_virtual_loopback.h"

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
	zassert_ok(nfc_virtual_loopback_init());
	return NULL;
}

static void loopback_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_virtual_loopback_reset();
	ndef_data_reset(&s_read);
	ndef_data_reset(&s_clone_model);
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
