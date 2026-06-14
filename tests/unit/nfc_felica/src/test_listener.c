/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * FeliCa Tier C — virtual listener (cookbook §14.4 / §5.7).
 */

#include "protocols/felica/felica_listener.h"

#include "Felica_mock.h"
#include "felica_store_fixture.h"
#include "nfc_response_spy.h"
#include "nfc_virtual_rf.h"

#include "protocols/felica/felica_poller.h"
#include "protocols/felica/felica_poller_i.h"

#include <string.h>

#include <zephyr/ztest.h>

static felica_data_t s_model;

static void listener_reset(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_response_spy_reset();
	(void)felica_listener_shutdown();
	zassert_ok(felica_listener_init(NULL));
}

static void listener_load_golden(void)
{
	(void)memset(&s_model, 0, sizeof(s_model));
	zassert_ok(felica_deserialize(&s_model, felica_Felica_model, FELICA_FELICA_MODEL_LEN));
	zassert_ok(felica_listener_init(&s_model));
}

static void listener_send_tag_cmd(const uint8_t *tx, size_t tx_len)
{
	const nfc_service_t *svc = felica_listener_get();

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

ZTEST(felica_listener, test_poll_response)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send_tag_cmd(felica_Felica_step0_tx, sizeof(felica_Felica_step0_tx));
	listener_assert_rx_equals(felica_Felica_step0_rx, sizeof(felica_Felica_step0_rx));
}

ZTEST(felica_listener, test_read_block0)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send_tag_cmd(felica_Felica_step1_tx, sizeof(felica_Felica_step1_tx));
	listener_assert_rx_equals(felica_Felica_step1_rx, sizeof(felica_Felica_step1_rx));
}

ZTEST(felica_listener, test_read_block14)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send_tag_cmd(felica_Felica_step15_tx, sizeof(felica_Felica_step15_tx));
	listener_assert_rx_equals(felica_Felica_step15_rx, sizeof(felica_Felica_step15_rx));
}

ZTEST(felica_listener, test_poll_crc_fail_silent)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send_tag_cmd(felica_Felica_step0_tx, sizeof(felica_Felica_step0_tx) - 1U);
	zassert_equal(nfc_response_spy_call_count(), 0U);
}

ZTEST(felica_listener, test_read_wrong_idm_silent)
{
	static uint8_t bad_tx[sizeof(felica_Felica_step1_tx)];

	listener_reset(NULL);
	listener_load_golden();
	(void)memcpy(bad_tx, felica_Felica_step1_tx, sizeof(bad_tx));
	bad_tx[4] ^= 0xFFU;
	listener_send_tag_cmd(bad_tx, sizeof(bad_tx));
	zassert_equal(nfc_response_spy_call_count(), 0U);
}

ZTEST(felica_listener, test_load_store_envelope_body)
{
	static const uint8_t *body = &store_fixture_felica_card[10];
	static const size_t body_len = 525U;

	listener_reset(NULL);
	zassert_ok(felica_listener_get()->deserialize(body, body_len, NULL));
	listener_send_tag_cmd(felica_Felica_step0_tx, sizeof(felica_Felica_step0_tx));
	listener_assert_rx_equals(felica_Felica_step0_rx, sizeof(felica_Felica_step0_rx));
}

ZTEST(felica_listener, test_poller_poll_virtual_rf)
{
	nfc_reader_session_t session = {.active = true};
	const felica_poller_polling_command_t cmd = {
		.system_code = FELICA_SYSTEM_CODE_ANY,
		.request_code = 0U,
		.time_slot = FELICA_TIME_SLOT_1,
	};
	felica_poller_polling_response_t resp;

	listener_reset(NULL);
	listener_load_golden();
	zassert_ok(nfc_virtual_rf_enable(felica_listener_get()));
	zassert_ok(felica_poller_polling(&session, &cmd, &resp, K_MSEC(5000)));
	zassert_equal(resp.pmm[1], 0xF1U);
	nfc_virtual_rf_disable();
}

static felica_data_t s_listener_expected;
static felica_data_t s_listener_read;

ZTEST(felica_listener, test_poller_read_virtual_rf)
{
	nfc_reader_session_t session = {.active = true};

	listener_reset(NULL);
	listener_load_golden();
	felica_data_reset(&s_listener_expected);
	zassert_ok(felica_deserialize(&s_listener_expected, felica_Felica_model,
				       FELICA_FELICA_MODEL_LEN));

	zassert_ok(nfc_virtual_rf_enable(felica_listener_get()));
	felica_data_reset(&s_listener_read);
	zassert_ok(felica_poller_read(&session, &s_listener_read));
	zassert_ok(felica_compare(&s_listener_expected, &s_listener_read));
	nfc_virtual_rf_disable();
}

ZTEST_SUITE(felica_listener, NULL, NULL, listener_reset, NULL, NULL);
