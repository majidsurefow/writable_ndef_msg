/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E+ — Classic 4K only (256-block model; isolated from aggregate reader RAM budget).
 */

#include "protocols/classic/classic.h"
#include "protocols/classic/classic_listener.h"
#include "protocols/classic/classic_poller.h"
#include "store/nfc_store.h"

#include "classic_store_fixture.h"
#include "nfc_virtual_rf.h"
#include "nfc_stack/nfc_stack.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#define CLASSIC_4K_CARD_BODY_OFF 10U

static classic_data_t s_read;
static classic_data_t s_golden;
static uint8_t s_saved_blob[4352];
static size_t s_saved_len;

static const uint8_t s_uid_4k_7b[] = {0x04U, 0x4BU, 0x4BU, 0x4BU, 0x4BU, 0xCAU, 0xFEU};

nfc_stack_state_t nfc_stack_get_state(void)
{
	return NFC_STACK_STATE_INITIALIZED;
}

static int classic_body_from_card_4k_4b(const uint8_t **body, size_t *body_len)
{
	if (STORE_FIXTURE_MFCLASSIC_4K_4B_CARD_LEN <= CLASSIC_4K_CARD_BODY_OFF) {
		return -EBADMSG;
	}

	*body = &store_fixture_mfclassic_4k_4b_card[CLASSIC_4K_CARD_BODY_OFF];
	*body_len = STORE_FIXTURE_MFCLASSIC_4K_4B_CARD_LEN - CLASSIC_4K_CARD_BODY_OFF - 2U;
	return 0;
}

static int classic_body_from_card_4k_7b(const uint8_t **body, size_t *body_len)
{
	if (STORE_FIXTURE_MFCLASSIC_4K_7B_CARD_LEN <= CLASSIC_4K_CARD_BODY_OFF) {
		return -EBADMSG;
	}

	*body = &store_fixture_mfclassic_4k_7b_card[CLASSIC_4K_CARD_BODY_OFF];
	*body_len = STORE_FIXTURE_MFCLASSIC_4K_7B_CARD_LEN - CLASSIC_4K_CARD_BODY_OFF - 2U;
	return 0;
}

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

ZTEST_SUITE(nfc_reader_classic_4k, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(nfc_reader_classic_4k, test_store_peek_golden_4k_4b)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	zassert_true(STORE_FIXTURE_MFCLASSIC_4K_4B_CARD_LEN <= sizeof(s_saved_blob));
	(void)memcpy(s_saved_blob, store_fixture_mfclassic_4k_4b_card,
		     STORE_FIXTURE_MFCLASSIC_4K_4B_CARD_LEN);
	s_saved_len = STORE_FIXTURE_MFCLASSIC_4K_4B_CARD_LEN;

	ret = nfc_store_peek_entry_meta("golden", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_CLASSIC);
	zassert_equal(flags, NFC_STORE_ENTRY_FLAG_HAND_AUTHORED);
}

ZTEST(nfc_reader_classic_4k, test_virtual_rf_poller_read_4k_4b)
{
	const uint8_t *body = NULL;
	size_t body_len = 0U;
	nfc_reader_session_t session = {
		.active = true,
		.tag.valid = true,
	};

	zassert_ok(classic_body_from_card_4k_4b(&body, &body_len));
	classic_data_reset(&s_golden);
	zassert_ok(classic_deserialize(&s_golden, body, body_len));

	session.tag.uid.len = s_golden.uid_len;
	(void)memcpy(session.tag.uid.bytes, s_golden.uid, s_golden.uid_len);

	(void)classic_listener_shutdown();
	zassert_ok(classic_listener_init(&s_golden));
	zassert_ok(nfc_virtual_rf_enable(classic_listener_get()));

	classic_data_reset(&s_read);
	zassert_ok(classic_poller_read(&session, &s_read));

	(void)classic_listener_shutdown();
	zassert_ok(classic_compare(&s_golden, &s_read));
	nfc_virtual_rf_disable();
}

ZTEST(nfc_reader_classic_4k, test_store_peek_golden_4k_7b)
{
	uint8_t persist_id;
	uint8_t flags;
	int ret;

	zassert_true(STORE_FIXTURE_MFCLASSIC_4K_7B_CARD_LEN <= sizeof(s_saved_blob));
	(void)memcpy(s_saved_blob, store_fixture_mfclassic_4k_7b_card,
		     STORE_FIXTURE_MFCLASSIC_4K_7B_CARD_LEN);
	s_saved_len = STORE_FIXTURE_MFCLASSIC_4K_7B_CARD_LEN;

	ret = nfc_store_peek_entry_meta("golden", &persist_id, &flags);
	zassert_ok(ret);
	zassert_equal(persist_id, NFC_PERSIST_ID_CLASSIC);
	zassert_equal(flags, NFC_STORE_ENTRY_FLAG_HAND_AUTHORED);
}

ZTEST(nfc_reader_classic_4k, test_virtual_rf_poller_read_4k_7b)
{
	const uint8_t *body = NULL;
	size_t body_len = 0U;
	nfc_reader_session_t session = {
		.active = true,
		.tag.valid = true,
	};

	zassert_ok(classic_body_from_card_4k_7b(&body, &body_len));
	classic_data_reset(&s_golden);
	zassert_ok(classic_deserialize(&s_golden, body, body_len));

	session.tag.uid.len = sizeof(s_uid_4k_7b);
	(void)memcpy(session.tag.uid.bytes, s_uid_4k_7b, sizeof(s_uid_4k_7b));

	(void)classic_listener_shutdown();
	zassert_ok(classic_listener_init(&s_golden));
	zassert_ok(nfc_virtual_rf_enable(classic_listener_get()));

	classic_data_reset(&s_read);
	zassert_ok(classic_poller_read(&session, &s_read));

	(void)classic_listener_shutdown();
	zassert_ok(classic_compare(&s_golden, &s_read));
	nfc_virtual_rf_disable();
}
