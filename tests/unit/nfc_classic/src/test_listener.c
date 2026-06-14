/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier C — virtual listener with Crypto1 (cookbook §14.4 / §5.3).
 */

#include "protocols/classic/classic_listener.h"
#include "protocols/classic/classic_poller.h"

#include "MfClassic_1K_4b_mock.h"
#include "classic_store_fixture.h"
#include "nfc_response_spy.h"
#include "nfc_virtual_rf.h"

#include <string.h>

#include <zephyr/ztest.h>

static classic_data_t s_listener_model;
static classic_data_t s_listener_read;
static classic_data_t s_listener_expected;

static void listener_reset(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_response_spy_reset();
	(void)classic_listener_shutdown();
	zassert_ok(classic_listener_init(NULL));
}

static void listener_load_golden(void)
{
	classic_data_reset(&s_listener_model);
	zassert_ok(classic_deserialize(&s_listener_model, classic_MfClassic_1K_4b_model,
				       CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN));
	zassert_ok(classic_listener_init(&s_listener_model));
}

static void listener_send_tag_cmd(const uint8_t *tx, size_t tx_len)
{
	const nfc_service_t *svc = classic_listener_get();

	svc->on_tag_cmd(tx, tx_len, NULL);
}

static void listener_assert_rx_equals(const uint8_t *exp, size_t exp_len)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, exp_len);
	zassert_mem_equal(buf, exp, exp_len);
}

ZTEST(classic_listener, test_auth_nt_sector0)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send_tag_cmd(classic_MfClassic_1K_4b_step2_tx,
			      CLASSIC_MFCLASSIC_1K_4B_STEP2_TX_LEN);
	listener_assert_rx_equals(classic_MfClassic_1K_4b_step2_rx,
				  sizeof(classic_MfClassic_1K_4b_step2_rx));
}

ZTEST(classic_listener, test_auth_invalid_block_silent)
{
	static const uint8_t probe[] = {0x60U, 0xFEU};

	listener_reset(NULL);
	listener_load_golden();
	listener_send_tag_cmd(probe, sizeof(probe));
	zassert_equal(nfc_response_spy_call_count(), 0U);
}

ZTEST(classic_listener, test_load_store_envelope_body)
{
	const uint8_t *body = &store_fixture_mfclassic_1k_4b_card[10];

	listener_reset(NULL);
	zassert_ok(classic_listener_get()->deserialize(body, CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN, NULL));
	listener_send_tag_cmd(classic_MfClassic_1K_4b_step2_tx,
			      CLASSIC_MFCLASSIC_1K_4B_STEP2_TX_LEN);
	listener_assert_rx_equals(classic_MfClassic_1K_4b_step2_rx,
				  sizeof(classic_MfClassic_1K_4b_step2_rx));
}

ZTEST(classic_listener, test_poller_read_virtual_rf)
{
	nfc_reader_session_t session = {
		.active = true,
		.tag.valid = true,
		.tag.uid.len = 4U,
	};

	session.tag.uid.bytes[0] = 0x04U;
	session.tag.uid.bytes[1] = 0xDEU;
	session.tag.uid.bytes[2] = 0xADU;
	session.tag.uid.bytes[3] = 0xBEU;

	listener_reset(NULL);
	listener_load_golden();
	classic_data_reset(&s_listener_expected);
	zassert_ok(classic_deserialize(&s_listener_expected, classic_MfClassic_1K_4b_model,
				       CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN));

	zassert_ok(nfc_virtual_rf_enable(classic_listener_get()));
	classic_data_reset(&s_listener_read);
	zassert_ok(classic_poller_read(&session, &s_listener_read));
	zassert_ok(classic_compare(&s_listener_expected, &s_listener_read));
	nfc_virtual_rf_disable();
}

ZTEST_SUITE(classic_listener, NULL, NULL, listener_reset, NULL, NULL);
