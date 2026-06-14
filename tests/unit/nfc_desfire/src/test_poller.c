/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire Tier B — poller detect/read (cookbook §14.3 / §5.4).
 */

#include "desfire_poller.h"
#include "nfc_session_mock.h"

#include "Desfire_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static desfire_data_t s_data;
static desfire_data_t s_expected; /* Static to avoid ~450 byte stack allocation */
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	desfire_data_reset(&s_data);
}

ZTEST(desfire_poller, test_detect_ok)
{
	poller_before(NULL);
	nfc_session_mock_load(desfire_Desfire_read_steps, DESFIRE_DESFIRE_READ_STEP_COUNT);
	zassert_ok(desfire_poller_detect(&s_session));
}

ZTEST(desfire_poller, test_detect_enotsup_on_bad_version)
{
	static const uint8_t bad_status[] = {0x91U, 0x1CU};
	static const nfc_session_mock_step_t script[] = {
		desfire_Desfire_read_steps[0],
		desfire_Desfire_read_steps[1],
		{.rx = bad_status, .rx_len = sizeof(bad_status), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(desfire_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(desfire_poller, test_read_golden)
{
	int ret;

	(void)memset(&s_expected, 0, sizeof(s_expected));
	poller_before(NULL);
	nfc_session_mock_load(desfire_Desfire_read_steps, DESFIRE_DESFIRE_READ_STEP_COUNT);
	ret = desfire_deserialize(&s_expected, desfire_Desfire_model, DESFIRE_DESFIRE_MODEL_LEN);
	zassert_ok(ret);

	zassert_ok(desfire_poller_read(&s_session, &s_data));
	zassert_mem_equal(s_data.uid, s_expected.uid, DESFIRE_UID_SIZE);
	zassert_equal(s_data.free_memory, s_expected.free_memory);
	zassert_equal(s_data.master_key_settings, s_expected.master_key_settings);
}

ZTEST(desfire_poller, test_read_partial_without_keys)
{
	poller_before(NULL);
	nfc_session_mock_load(desfire_Desfire_read_steps, DESFIRE_DESFIRE_READ_STEP_COUNT);
	zassert_ok(desfire_poller_read(&s_session, &s_data));
	zassert_equal(s_data.app_count, 0U);
}

ZTEST_SUITE(desfire_poller, NULL, NULL, poller_before, NULL, NULL);
