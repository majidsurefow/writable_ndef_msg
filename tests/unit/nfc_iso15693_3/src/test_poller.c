/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * ISO15693-3 Tier B — poller detect/read (cookbook §14.3 / §5.8).
 */

#include "protocols/iso15693_3/iso15693_3_poller.h"
#include "nfc_session_mock.h"

#include "Slix_cap_default_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static iso15693_3_data_t s_data;
static iso15693_3_data_t s_expected;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	iso15693_3_data_reset(&s_data);
}

static void assert_tx_matches(size_t idx, const uint8_t *exp, size_t exp_len)
{
	const uint8_t *tx = NULL;
	size_t tx_len = 0U;
	int ret;

	ret = nfc_session_mock_get_tx(idx, &tx, &tx_len);
	zassert_ok(ret, "tx index %zu missing", idx);
	zassert_true(tx_len >= exp_len, "tx %zu too short", idx);
	zassert_mem_equal(tx, exp, exp_len, "tx %zu prefix mismatch", idx);
}

ZTEST(iso15693_poller, test_detect_inventory)
{
	poller_before(NULL);
	nfc_session_mock_load(iso15693_Slix_cap_default_read_steps,
			      ISO15693_SLIX_CAP_DEFAULT_READ_STEP_COUNT);
	zassert_ok(iso15693_3_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
	assert_tx_matches(0U, iso15693_Slix_cap_default_step0_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP0_TX_LEN);
}

ZTEST(iso15693_poller, test_read_golden)
{
	int ret;

	(void)memset(&s_expected, 0, sizeof(s_expected));
	poller_before(NULL);
	nfc_session_mock_load(iso15693_Slix_cap_default_read_steps,
			      ISO15693_SLIX_CAP_DEFAULT_READ_STEP_COUNT);
	ret = iso15693_3_deserialize(&s_expected, iso15693_Slix_cap_default_model,
				     ISO15693_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_ok(ret);

	zassert_ok(iso15693_3_poller_read(&s_session, &s_data));
	zassert_equal(s_data.block_count, s_expected.block_count);
	zassert_equal(s_data.block_size, s_expected.block_size);
	zassert_mem_equal(s_data.uid, s_expected.uid, ISO15693_UID_SIZE);
	zassert_mem_equal(s_data.block_data, s_expected.block_data,
			  (size_t)s_expected.block_count * (size_t)s_expected.block_size);
	zassert_mem_equal(s_data.block_security, s_expected.block_security, s_expected.block_count);

	zassert_equal(nfc_session_mock_tx_count(), 85U);
	assert_tx_matches(0U, iso15693_Slix_cap_default_step0_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP0_TX_LEN);
	assert_tx_matches(1U, iso15693_Slix_cap_default_step1_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP1_TX_LEN);
	assert_tx_matches(2U, iso15693_Slix_cap_default_step2_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP2_TX_LEN);
	assert_tx_matches(41U, iso15693_Slix_cap_default_step41_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP41_TX_LEN);
	assert_tx_matches(82U, iso15693_Slix_cap_default_step82_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP82_TX_LEN);
	assert_tx_matches(83U, iso15693_Slix_cap_default_step83_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP83_TX_LEN);
	assert_tx_matches(84U, iso15693_Slix_cap_default_step84_tx,
			  ISO15693_SLIX_CAP_DEFAULT_STEP84_TX_LEN);
}

ZTEST_SUITE(iso15693_poller, NULL, NULL, poller_before, NULL, NULL);
