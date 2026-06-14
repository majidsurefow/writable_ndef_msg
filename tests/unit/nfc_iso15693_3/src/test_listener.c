/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/iso15693_3/iso15693_3_listener.h"

#include "Slix_cap_default_mock.h"
#include "nfc_response_spy.h"

#include <string.h>

#include <zephyr/ztest.h>

static iso15693_3_data_t s_model;

static void listener_reset(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_response_spy_reset();
	(void)iso15693_3_listener_shutdown();
}

static void listener_load_golden(void)
{
	(void)memset(&s_model, 0, sizeof(s_model));
	zassert_ok(iso15693_3_deserialize(&s_model, iso15693_Slix_cap_default_model,
					  ISO15693_SLIX_CAP_DEFAULT_MODEL_LEN));
	zassert_ok(iso15693_3_listener_init(&s_model));
}

static void listener_send(const uint8_t *tx, size_t tx_len)
{
	iso15693_3_listener_get()->on_tag_cmd(tx, tx_len, NULL);
}

static void listener_assert_rx_equals(const uint8_t *exp, size_t exp_len)
{
	const uint8_t *buf = NULL;
	size_t len = 0U;

	zassert_ok(nfc_response_spy_last(&buf, &len));
	zassert_equal(len, exp_len);
	zassert_mem_equal(buf, exp, exp_len);
}

ZTEST(iso15693_listener, test_inventory)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send(iso15693_Slix_cap_default_step0_tx, sizeof(iso15693_Slix_cap_default_step0_tx));
	listener_assert_rx_equals(iso15693_Slix_cap_default_step0_rx,
				  sizeof(iso15693_Slix_cap_default_step0_rx));
}

ZTEST(iso15693_listener, test_get_sys_info)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send(iso15693_Slix_cap_default_step1_tx, sizeof(iso15693_Slix_cap_default_step1_tx));
	listener_assert_rx_equals(iso15693_Slix_cap_default_step1_rx,
				  sizeof(iso15693_Slix_cap_default_step1_rx));
}

ZTEST(iso15693_listener, test_read_block0)
{
	listener_reset(NULL);
	listener_load_golden();
	listener_send(iso15693_Slix_cap_default_step2_tx, sizeof(iso15693_Slix_cap_default_step2_tx));
	listener_assert_rx_equals(iso15693_Slix_cap_default_step2_rx,
				  sizeof(iso15693_Slix_cap_default_step2_rx));
}

ZTEST_SUITE(iso15693_listener, NULL, NULL, listener_reset, NULL, NULL);
