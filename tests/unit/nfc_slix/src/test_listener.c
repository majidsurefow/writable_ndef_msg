/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/slix/slix_listener.h"
#include "protocols/slix/slix_poller_i.h"

#include "Slix_cap_default_mock.h"
#include "Slix_cap_accept_all_pass_mock.h"
#include "nfc_response_spy.h"
#include "nfc_session_mock.h"
#include "nfc_virtual_rf.h"

#include <string.h>

#include <zephyr/ztest.h>

static slix_data_t s_model;
static nfc_reader_session_t s_session;

static void listener_reset(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_response_spy_reset();
	nfc_session_mock_reset();
	s_session.active = true;
	(void)slix_listener_shutdown();
}

static void listener_load_golden(void)
{
	(void)memset(&s_model, 0, sizeof(s_model));
	zassert_ok(slix_deserialize(&s_model, slix_Slix_cap_default_model,
				    SLIX_SLIX_CAP_DEFAULT_MODEL_LEN));
	zassert_ok(slix_listener_init(&s_model));
}

static void listener_send(const uint8_t *tx, size_t tx_len)
{
	slix_listener_get()->on_tag_cmd(tx, tx_len, NULL);
}

static void listener_assert_rx_flag_ok(void)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_true(len >= 1U);
	zassert_equal(buf[0], 0U);
}

ZTEST(slix_listener, test_parent_inventory)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send(slix_Slix_cap_default_step0_tx, sizeof(slix_Slix_cap_default_step0_tx));
	listener_assert_rx_flag_ok();
}

ZTEST(slix_listener, test_password_unlock_loopback)
{
	uint16_t random = 0U;
	int ret;

	listener_reset(NULL);
	slix_data_reset(&s_model);
	zassert_ok(slix_deserialize(&s_model, slix_Slix_cap_accept_all_pass_model,
				    SLIX_SLIX_CAP_ACCEPT_ALL_PASS_MODEL_LEN));
	zassert_ok(slix_listener_init(&s_model));
	zassert_ok(nfc_virtual_rf_enable(slix_listener_get()));

	ret = slix_poller_get_random_number(&s_session, &random);
	zassert_ok(ret);

	ret = slix_poller_set_password(&s_session, s_model.parent.uid, SLIX_PASSWORD_READ,
				       (const uint8_t[]){ 0x11U, 0x22U, 0x33U, 0x44U }, random);
	zassert_ok(ret);

	nfc_virtual_rf_disable();
}

ZTEST_SUITE(slix_listener, NULL, NULL, listener_reset, NULL, NULL);
