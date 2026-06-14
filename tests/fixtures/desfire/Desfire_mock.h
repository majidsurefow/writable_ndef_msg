/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * DESFire golden model + poller script (wave5-desfire / cookbook §5.4).
 */

#ifndef TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_
#define TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_

#include <stddef.h>
#include <stdint.h>

#include "nfc_session_mock.h"

static const uint8_t desfire_Desfire_model[] = {
	0x01U, 0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U, 0x04U, 0x01U, 0x01U, 0x00U,
	0x18U, 0x05U, 0x01U, 0x04U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x01U, 0x24U, 0x60U, 0x1DU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x0BU, 0x01U, 0x01U, 0x02U, 0x03U, 0x0BU, 0x00U, 0x01U, 0x01U, 0x00U, 0x00U,
	0x0EU, 0x0EU, 0x0FU, 0x00U, 0x00U, 0x00U, 0x0FU, 0x00U, 0x48U, 0x45U, 0x4CU, 0x4CU,
	0x4FU, 0x20U, 0x44U, 0x45U, 0x53U, 0x46U, 0x49U, 0x52U, 0x45U, 0x21U, 0x21U,
};

#define DESFIRE_DESFIRE_MODEL_LEN ((size_t)sizeof(desfire_Desfire_model))

static const uint8_t desfire_select_ok[] = {0x90U, 0x00U};
static const uint8_t desfire_key_version_ok[] = {0x00U, 0x91U, 0x00U};
static const uint8_t desfire_version_frame0[] = {
	0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U, 0x91U, 0xAFU,
};
static const uint8_t desfire_version_frame1[] = {
	0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U, 0x91U, 0xAFU,
};
static const uint8_t desfire_version_frame2[] = {
	0x04U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x01U, 0x24U, 0x91U, 0x00U,
};
static const uint8_t desfire_free_memory_ok[] = {0x60U, 0x1DU, 0x00U, 0x00U, 0x91U, 0x00U};
static const uint8_t desfire_key_settings_ok[] = {0x0BU, 0x0FU, 0x91U, 0x00U};
static const uint8_t desfire_app_ids_ok[] = {0x01U, 0x02U, 0x03U, 0x91U, 0x00U};
static const uint8_t desfire_select_app_ok[] = {0x91U, 0x00U};
static const uint8_t desfire_app_key_settings_ok[] = {0x0BU, 0x00U, 0x91U, 0x00U};
static const uint8_t desfire_file_ids_ok[] = {0x01U, 0x91U, 0x00U};
static const uint8_t desfire_file_settings_ok[] = {
	0x00U, 0x00U, 0x0EU, 0x0EU, 0x0FU, 0x00U, 0x00U, 0x00U, 0x91U, 0x00U,
};
static const uint8_t desfire_read_data_ok[] = {
	0x48U, 0x45U, 0x4CU, 0x4CU, 0x4FU, 0x20U, 0x44U, 0x45U, 0x53U, 0x46U, 0x49U,
	0x52U, 0x45U, 0x21U, 0x21U, 0x91U, 0x00U,
};

static const nfc_session_mock_step_t desfire_Desfire_read_steps[] = {
	{.rx = desfire_select_ok, .rx_len = sizeof(desfire_select_ok), .err = 0},
	{.rx = desfire_key_version_ok, .rx_len = sizeof(desfire_key_version_ok), .err = 0},
	{.rx = desfire_version_frame0, .rx_len = sizeof(desfire_version_frame0), .err = 0},
	{.rx = desfire_version_frame1, .rx_len = sizeof(desfire_version_frame1), .err = 0},
	{.rx = desfire_version_frame2, .rx_len = sizeof(desfire_version_frame2), .err = 0},
	{.rx = desfire_free_memory_ok, .rx_len = sizeof(desfire_free_memory_ok), .err = 0},
	{.rx = desfire_key_settings_ok, .rx_len = sizeof(desfire_key_settings_ok), .err = 0},
	{.rx = desfire_app_ids_ok, .rx_len = sizeof(desfire_app_ids_ok), .err = 0},
	{.rx = desfire_select_app_ok, .rx_len = sizeof(desfire_select_app_ok), .err = 0},
	{.rx = desfire_app_key_settings_ok, .rx_len = sizeof(desfire_app_key_settings_ok), .err = 0},
	{.rx = desfire_file_ids_ok, .rx_len = sizeof(desfire_file_ids_ok), .err = 0},
	{.rx = desfire_file_settings_ok, .rx_len = sizeof(desfire_file_settings_ok), .err = 0},
	{.rx = desfire_read_data_ok, .rx_len = sizeof(desfire_read_data_ok), .err = 0},
};

#define DESFIRE_DESFIRE_READ_STEP_COUNT ARRAY_SIZE(desfire_Desfire_read_steps)

#endif /* TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_ */
