/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_FIXTURES_EMV_EMV_MOCK_H_
#define TESTS_FIXTURES_EMV_EMV_MOCK_H_

#include <stddef.h>
#include <stdint.h>

#include "nfc_session_mock.h"

static const uint8_t emv_ppse_fci_ok[] = {0x6FU, 0x05U, 0x84U, 0x0EU, 0x32U, 0x90U, 0x00U};
static const uint8_t emv_app_fci_ok[] = {0x6FU, 0x05U, 0x84U, 0x07U, 0xA0U, 0x90U, 0x00U};
static const uint8_t emv_gpo_ok[] = {0x80U, 0x06U, 0x00U, 0x00U, 0x08U, 0x01U, 0x01U, 0x00U,
				     0x90U, 0x00U};
static const uint8_t emv_record_ok[] = {0x70U, 0x03U, 0x57U, 0x01U, 0x00U, 0x90U, 0x00U};

static const nfc_session_mock_step_t emv_Emv_read_steps[] = {
	{.rx = emv_ppse_fci_ok, .rx_len = sizeof(emv_ppse_fci_ok), .err = 0},
	{.rx = emv_app_fci_ok, .rx_len = sizeof(emv_app_fci_ok), .err = 0},
	{.rx = emv_gpo_ok, .rx_len = sizeof(emv_gpo_ok), .err = 0},
	{.rx = emv_record_ok, .rx_len = sizeof(emv_record_ok), .err = 0},
};

#define EMV_EMV_READ_STEP_COUNT ARRAY_SIZE(emv_Emv_read_steps)

#endif /* TESTS_FIXTURES_EMV_EMV_MOCK_H_ */
