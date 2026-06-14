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
static felica_data_t s_expected; /* Static to avoid large stack allocation */
static nfc_reader_session_t s_session;

static void poller_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_session_mock_reset();
	s_session.active = true;
	felica_data_reset(&s_data);
}

static void assert_tx_equals(size_t index, const uint8_t *exp, size_t exp_len)
{
	const uint8_t *tx = NULL;
	size_t tx_len = 0U;

	zassert_ok(nfc_session_mock_get_tx(index, &tx, &tx_len), "tx step %zu missing", index);
	zassert_equal(tx_len, exp_len, "tx step %zu len %zu exp %zu", index, tx_len, exp_len);
	zassert_mem_equal(tx, exp, exp_len, "tx step %zu payload", index);
}

ZTEST(felica_poller, test_detect_poll)
{
	poller_before(NULL);
	nfc_session_mock_load(felica_Felica_read_steps, FELICA_FELICA_READ_STEP_COUNT);
	zassert_ok(felica_poller_detect(&s_session));
	zassert_equal(nfc_session_mock_tx_count(), 1U);
}

ZTEST(felica_poller, test_detect_poll_tx_framed)
{
	poller_before(NULL);
	nfc_session_mock_load(felica_Felica_read_steps, FELICA_FELICA_READ_STEP_COUNT);
	zassert_ok(felica_poller_detect(&s_session));
	assert_tx_equals(0U, felica_Felica_step0_tx, sizeof(felica_Felica_step0_tx));
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

ZTEST(felica_poller, test_detect_crc_fail)
{
	static const uint8_t bad_crc_poll[] = {
		0x12U, 0x01U, 0x29U, 0x9FU, 0xFAU, 0x53U, 0xABU, 0x75U, 0x87U, 0x6EU,
		0x00U, 0xF1U, 0x00U, 0x00U, 0x00U, 0x01U, 0x43U, 0x00U, 0x00U, 0x00U,
	};
	static const nfc_session_mock_step_t script[] = {
		{.rx = bad_crc_poll, .rx_len = sizeof(bad_crc_poll), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(felica_poller_detect(&s_session), -ENOTSUP);
}

ZTEST(felica_poller, test_read_golden)
{
	int ret;

	(void)memset(&s_expected, 0, sizeof(s_expected));
	poller_before(NULL);
	nfc_session_mock_load(felica_Felica_read_steps, FELICA_FELICA_READ_STEP_COUNT);
	ret = felica_deserialize(&s_expected, felica_Felica_model, FELICA_FELICA_MODEL_LEN);
	zassert_ok(ret);

	zassert_ok(felica_poller_read(&s_session, &s_data));
	zassert_equal(s_data.blocks_total, s_expected.blocks_total);
	zassert_equal(s_data.blocks_read, s_expected.blocks_read);
	zassert_mem_equal(s_data.idm, s_expected.idm, FELICA_IDM_SIZE);
	zassert_mem_equal(s_data.pmm, s_expected.pmm, FELICA_PMM_SIZE);
	zassert_mem_equal(s_data.blocks[0].data, s_expected.blocks[0].data, FELICA_BLOCK_DATA_SIZE);
	zassert_mem_equal(s_data.blocks[14].data, s_expected.blocks[14].data, FELICA_BLOCK_DATA_SIZE);
}

ZTEST(felica_poller, test_read_tx_sequence)
{
	size_t step;

	poller_before(NULL);
	nfc_session_mock_load(felica_Felica_read_steps, FELICA_FELICA_READ_STEP_COUNT);
	zassert_ok(felica_poller_read(&s_session, &s_data));
	zassert_equal(nfc_session_mock_tx_count(), FELICA_FELICA_READ_TX_STEP_COUNT);

	for (step = 0U; step < FELICA_FELICA_READ_TX_STEP_COUNT; step++) {
		assert_tx_equals(step, felica_Felica_read_tx_steps[step],
				 felica_Felica_read_tx_lens[step]);
	}
}

ZTEST(felica_poller, test_read_crc_fail)
{
	static const uint8_t bad_crc_read_rx[] = {
		0x1DU, 0x07U, 0x29U, 0x9FU, 0xFAU, 0x53U, 0xABU, 0x75U, 0x87U, 0x6EU,
		0x00U, 0x00U, 0x01U, 0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U,
		0x77U, 0x88U, 0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU, 0x00U,
		0x00U, 0x00U,
	};
	static const nfc_session_mock_step_t script[] = {
		{.rx = felica_Felica_step0_rx, .rx_len = sizeof(felica_Felica_step0_rx), .err = 0},
		{.rx = felica_Felica_step1_rx, .rx_len = sizeof(felica_Felica_step1_rx), .err = 0},
		{.rx = bad_crc_read_rx, .rx_len = sizeof(bad_crc_read_rx), .err = 0},
	};

	poller_before(NULL);
	nfc_session_mock_load(script, ARRAY_SIZE(script));
	zassert_equal(felica_poller_read(&s_session, &s_data), -EIO);
}

ZTEST_SUITE(felica_poller, NULL, NULL, poller_before, NULL, NULL);
