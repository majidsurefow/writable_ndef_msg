/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Classic Tier B — poller scaffold (cookbook §14.13 Step 3 / §5.3).
 */

#include "protocols/classic/classic_poller.h"
#include "nfc_session_mock.h"

#include <errno.h>

#include <zephyr/ztest.h>

static classic_data_t s_data;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	classic_data_reset(&s_data);
}

ZTEST(classic_poller, test_detect_not_implemented)
{
	poller_before(NULL);
	zassert_equal(classic_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(classic_poller, test_read_not_implemented)
{
	poller_before(NULL);
	zassert_equal(classic_poller_read(&s_session, &s_data), -ENOTSUP);
}

ZTEST_SUITE(classic_poller, NULL, NULL, poller_before, NULL, NULL);
