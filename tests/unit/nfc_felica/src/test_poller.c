/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * FeliCa Tier B — poller detect/read (cookbook §14.3 / §5.7).
 */

#include "protocols/felica/felica_poller.h"
#include "nfc_session_mock.h"

#include "Felica_mock.h"

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

static felica_data_t s_data;
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	felica_data_reset(&s_data);
}

ZTEST(felica_poller, test_detect_poll)
{
	poller_before(NULL);
	nfc_session_mock_load(felica_Felica_read_steps, FELICA_FELICA_READ_STEP_COUNT);
	zassert_ok(felica_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
}

ZTEST(felica_poller, test_detect_enotsup_short_rx)
{
	static const uint8_t short_rx[] = {0x01U, 0x02U, 0x03U};
	static const nfc_session_mock_step_t script[] = {
		{.rx = short_rx, .rx_len = sizeof(short_rx), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(felica_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(felica_poller, test_read_golden)
{
	felica_data_t expected;
	int ret;

	poller_before(NULL);
	nfc_session_mock_load(felica_Felica_read_steps, FELICA_FELICA_READ_STEP_COUNT);
	ret = felica_deserialize(&expected, felica_Felica_model, FELICA_FELICA_MODEL_LEN);
	zassert_ok(ret);

	zassert_ok(felica_poller_read(&s_session, &s_data));
	zassert_equal(s_data.blocks_total, expected.blocks_total);
	zassert_mem_equal(s_data.idm, expected.idm, FELICA_IDM_SIZE);
	zassert_mem_equal(s_data.pmm, expected.pmm, FELICA_PMM_SIZE);
	zassert_mem_equal(s_data.blocks[0].data, expected.blocks[0].data, FELICA_BLOCK_DATA_SIZE);
	zassert_mem_equal(s_data.blocks[14].data, expected.blocks[14].data, FELICA_BLOCK_DATA_SIZE);
}

ZTEST_SUITE(felica_poller, NULL, NULL, poller_before, NULL, NULL);
