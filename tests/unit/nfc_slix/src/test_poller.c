/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * SLIX Tier B — poller detect/read for CAP variants (cookbook §14.3 / §5.9).
 */

#include "protocols/slix/slix_poller.h"
#include "nfc_session_mock.h"

#include "Slix_cap_default_mock.h"
#include "Slix_cap_missed_mock.h"
#include "Slix_cap_accept_all_pass_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static slix_data_t s_data;
static slix_data_t s_expected;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	slix_data_reset(&s_data);
}

ZTEST(slix_poller, test_detect_inventory)
{
	poller_before(NULL);
	nfc_session_mock_load(slix_Slix_cap_default_read_steps, SLIX_SLIX_CAP_DEFAULT_READ_STEP_COUNT);
	zassert_ok(slix_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
}

ZTEST(slix_poller, test_read_default_golden)
{
	int ret;

	poller_before(NULL);
	nfc_session_mock_load(slix_Slix_cap_default_read_steps, SLIX_SLIX_CAP_DEFAULT_READ_STEP_COUNT);
	ret = slix_deserialize(&s_expected, slix_Slix_cap_default_model, SLIX_SLIX_CAP_DEFAULT_MODEL_LEN);
	zassert_ok(ret);

	zassert_ok(slix_poller_read(&s_session, &s_data));
	zassert_equal(s_data.parent.block_count, s_expected.parent.block_count);
	zassert_mem_equal(s_data.parent.uid, s_expected.parent.uid, ISO15693_UID_SIZE);
	zassert_equal(s_data.capabilities, SLIX_CAP_DEFAULT);
	zassert_mem_equal(s_data.signature, s_expected.signature, SLIX_SIGNATURE_SIZE);
}

ZTEST(slix_poller, test_read_missed_cap)
{
	poller_before(NULL);
	nfc_session_mock_load(slix_Slix_cap_missed_read_steps, SLIX_SLIX_CAP_MISSED_READ_STEP_COUNT);
	zassert_ok(slix_poller_read(&s_session, &s_data));
	zassert_equal(s_data.capabilities, SLIX_CAP_MISSED);
}

ZTEST(slix_poller, test_read_accept_all_cap)
{
	poller_before(NULL);
	nfc_session_mock_load(slix_Slix_cap_accept_all_pass_read_steps,
			      SLIX_SLIX_CAP_ACCEPT_ALL_PASS_READ_STEP_COUNT);
	zassert_ok(slix_poller_read(&s_session, &s_data));
	zassert_equal(s_data.capabilities, SLIX_CAP_ACCEPT_ALL);
}

ZTEST_SUITE(slix_poller, NULL, NULL, poller_before, NULL, NULL);
