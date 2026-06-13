/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF poller Tier B fixtures — TX/RX scripts (cookbook §14.3 / §5.1).
 */

#ifndef TESTS_FIXTURES_NDEF_NDEF_FIXTURE_H_
#define TESTS_FIXTURES_NDEF_NDEF_FIXTURE_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define NDEF_FIXTURE_CHUNK_FIRST 253U
#define NDEF_FIXTURE_CHUNK_SECOND 47U
#define NDEF_FIXTURE_CHUNK_NLEN (NDEF_FIXTURE_CHUNK_FIRST + NDEF_FIXTURE_CHUNK_SECOND)

static const uint8_t ndef_fixture_sw_ok[] = {
#include "v2_detect.inc"
};

static const uint8_t ndef_fixture_sw_v2_fail[] = {
#include "v1_fallback.inc"
};

static const uint8_t ndef_fixture_uri_payload[] = {
#include "uri_5byte_script.inc"
};

/* T4T CC: MLe=255, MLc=255, FileID=E104, MaxNDEF=100 */
static const uint8_t ndef_fixture_cc_std[] = {
	0x00U, 0x0FU, 0x20U, 0x00U, 0xFFU, 0x00U, 0xFFU, 0x04U, 0x06U, 0xE1U, 0x04U,
	0x00U, 0x64U, 0x00U, 0x00U,
};

/* T4T CC: MLe=768 (0x0300), MaxNDEF=500 — forces 255 B transport chunk cap */
static const uint8_t ndef_fixture_cc_chunk[] = {
	0x00U, 0x0FU, 0x20U, 0x03U, 0x00U, 0x00U, 0xFFU, 0x04U, 0x06U, 0xE1U, 0x04U,
	0x01U, 0xF4U, 0x00U, 0x00U,
};

static const uint8_t ndef_fixture_rx_select_v2_ok[] = {0x90U, 0x00U};
static const uint8_t ndef_fixture_rx_select_v1_ok[] = {0x90U, 0x00U};
static const uint8_t ndef_fixture_rx_select_v2_fail[] = {0x6AU, 0x82U};
static const uint8_t ndef_fixture_rx_select_v1_fail[] = {0x6AU, 0x82U};

static const nfc_session_mock_step_t ndef_fixture_detect_v2[] = {
	{.rx = ndef_fixture_rx_select_v2_ok, .rx_len = sizeof(ndef_fixture_rx_select_v2_ok), .err = 0},
};

static const nfc_session_mock_step_t ndef_fixture_detect_v1_fallback[] = {
	{.rx = ndef_fixture_rx_select_v2_fail, .rx_len = sizeof(ndef_fixture_rx_select_v2_fail),
	 .err = 0},
	{.rx = ndef_fixture_rx_select_v1_ok, .rx_len = sizeof(ndef_fixture_rx_select_v1_ok), .err = 0},
};

static const nfc_session_mock_step_t ndef_fixture_detect_enotsup[] = {
	{.rx = ndef_fixture_rx_select_v2_fail, .rx_len = sizeof(ndef_fixture_rx_select_v2_fail),
	 .err = 0},
	{.rx = ndef_fixture_rx_select_v1_fail, .rx_len = sizeof(ndef_fixture_rx_select_v1_fail),
	 .err = 0},
};

static uint8_t ndef_fixture_cc_read_rx[17];
static uint8_t ndef_fixture_nlen_empty_rx[4];
static uint8_t ndef_fixture_nlen_uri_rx[4];
static uint8_t ndef_fixture_nlen_chunk_rx[4];
static uint8_t ndef_fixture_uri_body_rx[7];
static uint8_t ndef_fixture_chunk_rx0[255];
static uint8_t ndef_fixture_chunk_rx1[49];
static uint8_t ndef_fixture_nlen_overflow_rx[4];
static uint8_t ndef_fixture_truncated_body_rx[4];

static nfc_session_mock_step_t ndef_fixture_read_empty_steps[5];
static nfc_session_mock_step_t ndef_fixture_read_uri_steps[6];
static nfc_session_mock_step_t ndef_fixture_read_chunk_steps[7];
static nfc_session_mock_step_t ndef_fixture_read_sw_error_steps[4];
static nfc_session_mock_step_t ndef_fixture_read_nlen_overflow_steps[5];
static nfc_session_mock_step_t ndef_fixture_read_truncated_steps[6];

static void ndef_fixture_append_sw(uint8_t *rx, size_t data_len, size_t *rx_len)
{
	rx[data_len] = 0x90U;
	rx[data_len + 1U] = 0x00U;
	*rx_len = data_len + 2U;
}

static void ndef_fixture_build_read_empty(void)
{
	(void)memcpy(ndef_fixture_cc_read_rx, ndef_fixture_cc_std, sizeof(ndef_fixture_cc_std));
	ndef_fixture_append_sw(ndef_fixture_cc_read_rx, sizeof(ndef_fixture_cc_std),
			       &ndef_fixture_read_empty_steps[2].rx_len);
	ndef_fixture_read_empty_steps[2].rx = ndef_fixture_cc_read_rx;
	ndef_fixture_read_empty_steps[2].err = 0;

	ndef_fixture_nlen_empty_rx[0] = 0x00U;
	ndef_fixture_nlen_empty_rx[1] = 0x00U;
	ndef_fixture_append_sw(ndef_fixture_nlen_empty_rx, 2U,
			       &ndef_fixture_read_empty_steps[4].rx_len);
	ndef_fixture_read_empty_steps[4].rx = ndef_fixture_nlen_empty_rx;
	ndef_fixture_read_empty_steps[4].err = 0;

	ndef_fixture_read_empty_steps[0] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_ok,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_ok),
		.err = 0,
	};
	ndef_fixture_read_empty_steps[1] = ndef_fixture_read_empty_steps[0];
	ndef_fixture_read_empty_steps[3] = ndef_fixture_read_empty_steps[0];
}

static void ndef_fixture_build_read_uri(void)
{
	(void)memcpy(ndef_fixture_cc_read_rx, ndef_fixture_cc_std, sizeof(ndef_fixture_cc_std));
	ndef_fixture_append_sw(ndef_fixture_cc_read_rx, sizeof(ndef_fixture_cc_std),
			       &ndef_fixture_read_uri_steps[2].rx_len);
	ndef_fixture_read_uri_steps[2].rx = ndef_fixture_cc_read_rx;
	ndef_fixture_read_uri_steps[2].err = 0;

	ndef_fixture_nlen_uri_rx[0] = 0x00U;
	ndef_fixture_nlen_uri_rx[1] = (uint8_t)sizeof(ndef_fixture_uri_payload);
	ndef_fixture_append_sw(ndef_fixture_nlen_uri_rx, 2U, &ndef_fixture_read_uri_steps[4].rx_len);
	ndef_fixture_read_uri_steps[4].rx = ndef_fixture_nlen_uri_rx;
	ndef_fixture_read_uri_steps[4].err = 0;

	(void)memcpy(ndef_fixture_uri_body_rx, ndef_fixture_uri_payload,
		     sizeof(ndef_fixture_uri_payload));
	ndef_fixture_append_sw(ndef_fixture_uri_body_rx, sizeof(ndef_fixture_uri_payload),
			       &ndef_fixture_read_uri_steps[5].rx_len);
	ndef_fixture_read_uri_steps[5].rx = ndef_fixture_uri_body_rx;
	ndef_fixture_read_uri_steps[5].err = 0;

	ndef_fixture_read_uri_steps[0] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_ok,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_ok),
		.err = 0,
	};
	ndef_fixture_read_uri_steps[1] = ndef_fixture_read_uri_steps[0];
	ndef_fixture_read_uri_steps[3] = ndef_fixture_read_uri_steps[0];
}

static void ndef_fixture_build_read_chunk(void)
{
	size_t rx_len;

	(void)memcpy(ndef_fixture_cc_read_rx, ndef_fixture_cc_chunk, sizeof(ndef_fixture_cc_chunk));
	ndef_fixture_append_sw(ndef_fixture_cc_read_rx, sizeof(ndef_fixture_cc_chunk), &rx_len);
	ndef_fixture_read_chunk_steps[2].rx = ndef_fixture_cc_read_rx;
	ndef_fixture_read_chunk_steps[2].rx_len = rx_len;
	ndef_fixture_read_chunk_steps[2].err = 0;

	ndef_fixture_nlen_chunk_rx[0] = 0x01U;
	ndef_fixture_nlen_chunk_rx[1] = 0x2CU;
	ndef_fixture_append_sw(ndef_fixture_nlen_chunk_rx, 2U, &rx_len);
	ndef_fixture_read_chunk_steps[4].rx = ndef_fixture_nlen_chunk_rx;
	ndef_fixture_read_chunk_steps[4].rx_len = rx_len;
	ndef_fixture_read_chunk_steps[4].err = 0;

	for (size_t i = 0U; i < NDEF_FIXTURE_CHUNK_FIRST; i++) {
		ndef_fixture_chunk_rx0[i] = (uint8_t)(i & 0xFFU);
	}
	ndef_fixture_append_sw(ndef_fixture_chunk_rx0, NDEF_FIXTURE_CHUNK_FIRST, &rx_len);
	ndef_fixture_read_chunk_steps[5].rx = ndef_fixture_chunk_rx0;
	ndef_fixture_read_chunk_steps[5].rx_len = rx_len;
	ndef_fixture_read_chunk_steps[5].err = 0;

	for (size_t i = 0U; i < NDEF_FIXTURE_CHUNK_SECOND; i++) {
		ndef_fixture_chunk_rx1[i] = (uint8_t)((i + NDEF_FIXTURE_CHUNK_FIRST) & 0xFFU);
	}
	ndef_fixture_append_sw(ndef_fixture_chunk_rx1, NDEF_FIXTURE_CHUNK_SECOND, &rx_len);
	ndef_fixture_read_chunk_steps[6].rx = ndef_fixture_chunk_rx1;
	ndef_fixture_read_chunk_steps[6].rx_len = rx_len;
	ndef_fixture_read_chunk_steps[6].err = 0;

	ndef_fixture_read_chunk_steps[0] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_ok,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_ok),
		.err = 0,
	};
	ndef_fixture_read_chunk_steps[1] = ndef_fixture_read_chunk_steps[0];
	ndef_fixture_read_chunk_steps[3] = ndef_fixture_read_chunk_steps[0];
}

static void ndef_fixture_build_read_sw_error(void)
{
	(void)memcpy(ndef_fixture_cc_read_rx, ndef_fixture_cc_std, sizeof(ndef_fixture_cc_std));
	ndef_fixture_append_sw(ndef_fixture_cc_read_rx, sizeof(ndef_fixture_cc_std),
			       &ndef_fixture_read_sw_error_steps[2].rx_len);
	ndef_fixture_read_sw_error_steps[2].rx = ndef_fixture_cc_read_rx;
	ndef_fixture_read_sw_error_steps[2].err = 0;

	ndef_fixture_read_sw_error_steps[0] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_ok,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_ok),
		.err = 0,
	};
	ndef_fixture_read_sw_error_steps[1] = ndef_fixture_read_sw_error_steps[0];
	ndef_fixture_read_sw_error_steps[3] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_fail,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_fail),
		.err = 0,
	};
}

static void ndef_fixture_build_read_nlen_overflow(void)
{
	(void)memcpy(ndef_fixture_cc_read_rx, ndef_fixture_cc_std, sizeof(ndef_fixture_cc_std));
	ndef_fixture_append_sw(ndef_fixture_cc_read_rx, sizeof(ndef_fixture_cc_std),
			       &ndef_fixture_read_nlen_overflow_steps[2].rx_len);
	ndef_fixture_read_nlen_overflow_steps[2].rx = ndef_fixture_cc_read_rx;
	ndef_fixture_read_nlen_overflow_steps[2].err = 0;

	ndef_fixture_nlen_overflow_rx[0] = 0x02U;
	ndef_fixture_nlen_overflow_rx[1] = 0x00U;
	ndef_fixture_append_sw(ndef_fixture_nlen_overflow_rx, 2U,
			       &ndef_fixture_read_nlen_overflow_steps[4].rx_len);
	ndef_fixture_read_nlen_overflow_steps[4].rx = ndef_fixture_nlen_overflow_rx;
	ndef_fixture_read_nlen_overflow_steps[4].err = 0;

	ndef_fixture_read_nlen_overflow_steps[0] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_ok,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_ok),
		.err = 0,
	};
	ndef_fixture_read_nlen_overflow_steps[1] = ndef_fixture_read_nlen_overflow_steps[0];
	ndef_fixture_read_nlen_overflow_steps[3] = ndef_fixture_read_nlen_overflow_steps[0];
}

static void ndef_fixture_build_read_truncated(void)
{
	(void)memcpy(ndef_fixture_cc_read_rx, ndef_fixture_cc_std, sizeof(ndef_fixture_cc_std));
	ndef_fixture_append_sw(ndef_fixture_cc_read_rx, sizeof(ndef_fixture_cc_std),
			       &ndef_fixture_read_truncated_steps[2].rx_len);
	ndef_fixture_read_truncated_steps[2].rx = ndef_fixture_cc_read_rx;
	ndef_fixture_read_truncated_steps[2].err = 0;

	ndef_fixture_nlen_uri_rx[0] = 0x00U;
	ndef_fixture_nlen_uri_rx[1] = (uint8_t)sizeof(ndef_fixture_uri_payload);
	ndef_fixture_append_sw(ndef_fixture_nlen_uri_rx, 2U, &ndef_fixture_read_truncated_steps[4].rx_len);
	ndef_fixture_read_truncated_steps[4].rx = ndef_fixture_nlen_uri_rx;
	ndef_fixture_read_truncated_steps[4].err = 0;

	ndef_fixture_truncated_body_rx[0] = 0xD1U;
	ndef_fixture_append_sw(ndef_fixture_truncated_body_rx, 1U,
			       &ndef_fixture_read_truncated_steps[5].rx_len);
	ndef_fixture_read_truncated_steps[5].rx = ndef_fixture_truncated_body_rx;
	ndef_fixture_read_truncated_steps[5].err = 0;

	ndef_fixture_read_truncated_steps[0] = (nfc_session_mock_step_t){
		.rx = ndef_fixture_rx_select_v2_ok,
		.rx_len = sizeof(ndef_fixture_rx_select_v2_ok),
		.err = 0,
	};
	ndef_fixture_read_truncated_steps[1] = ndef_fixture_read_truncated_steps[0];
	ndef_fixture_read_truncated_steps[3] = ndef_fixture_read_truncated_steps[0];
}

static void ndef_fixture_init_read_scripts(void)
{
	ndef_fixture_build_read_empty();
	ndef_fixture_build_read_uri();
	ndef_fixture_build_read_chunk();
	ndef_fixture_build_read_sw_error();
	ndef_fixture_build_read_nlen_overflow();
	ndef_fixture_build_read_truncated();
}

#endif /* TESTS_FIXTURES_NDEF_NDEF_FIXTURE_H_ */
