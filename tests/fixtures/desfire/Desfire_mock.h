/* Auto-generated from Flipper MfDesfire_EV1_sample.nfc — do not edit. */
#ifndef TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_
#define TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

static const uint8_t desfire_Desfire_model[] = {
0x01U, 0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U,
0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U, 0x04U,
0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x01U, 0x24U, 0x60U, 0x1DU, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x0DU, 0x01U, 0x01U, 0x02U, 0x03U, 0x0DU, 0x00U,
0x01U, 0x01U, 0x00U, 0x00U, 0x0EU, 0x0EU, 0x0FU, 0x00U,
0x00U, 0x00U, 0x0FU, 0x00U, 0x48U, 0x45U, 0x4CU, 0x4CU,
0x4FU, 0x20U, 0x44U, 0x45U, 0x53U, 0x46U, 0x49U, 0x52U,
0x45U, 0x21U, 0x21U,
};

#define DESFIRE_DESFIRE_MODEL_LEN ((size_t)sizeof(desfire_Desfire_model))

static const uint8_t desfire_Desfire_step0_rx[] = {
0x90U, 0x00U,
};

static const uint8_t desfire_Desfire_step1_rx[] = {
0x00U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step2_rx[] = {
0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U, 0x91U,
0xAFU,
};

static const uint8_t desfire_Desfire_step3_rx[] = {
0x04U, 0x01U, 0x01U, 0x00U, 0x18U, 0x05U, 0x01U, 0x91U,
0xAFU,
};

static const uint8_t desfire_Desfire_step4_rx[] = {
0x04U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x01U, 0x24U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step5_rx[] = {
0x60U, 0x1DU, 0x00U, 0x00U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step6_rx[] = {
0x0DU, 0x01U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step7_rx[] = {
0x01U, 0x02U, 0x03U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step8_rx[] = {
0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step9_rx[] = {
0x0DU, 0x00U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step10_rx[] = {
0x01U, 0x91U, 0x00U,
};

static const uint8_t desfire_Desfire_step11_rx[] = {
0x00U, 0x00U, 0x0EU, 0x0EU, 0x0FU, 0x00U, 0x00U, 0x91U,
0x00U,
};

static const uint8_t desfire_Desfire_step12_rx[] = {
0x48U, 0x45U, 0x4CU, 0x4CU, 0x4FU, 0x20U, 0x44U, 0x45U,
0x53U, 0x46U, 0x49U, 0x52U, 0x45U, 0x21U, 0x21U, 0x91U,
0x00U,
};

static const nfc_session_mock_step_t desfire_Desfire_read_steps[] = {
	{.rx = desfire_Desfire_step0_rx, .rx_len = sizeof(desfire_Desfire_step0_rx), .err = 0},
	{.rx = desfire_Desfire_step1_rx, .rx_len = sizeof(desfire_Desfire_step1_rx), .err = 0},
	{.rx = desfire_Desfire_step2_rx, .rx_len = sizeof(desfire_Desfire_step2_rx), .err = 0},
	{.rx = desfire_Desfire_step3_rx, .rx_len = sizeof(desfire_Desfire_step3_rx), .err = 0},
	{.rx = desfire_Desfire_step4_rx, .rx_len = sizeof(desfire_Desfire_step4_rx), .err = 0},
	{.rx = desfire_Desfire_step5_rx, .rx_len = sizeof(desfire_Desfire_step5_rx), .err = 0},
	{.rx = desfire_Desfire_step6_rx, .rx_len = sizeof(desfire_Desfire_step6_rx), .err = 0},
	{.rx = desfire_Desfire_step7_rx, .rx_len = sizeof(desfire_Desfire_step7_rx), .err = 0},
	{.rx = desfire_Desfire_step8_rx, .rx_len = sizeof(desfire_Desfire_step8_rx), .err = 0},
	{.rx = desfire_Desfire_step9_rx, .rx_len = sizeof(desfire_Desfire_step9_rx), .err = 0},
	{.rx = desfire_Desfire_step10_rx, .rx_len = sizeof(desfire_Desfire_step10_rx), .err = 0},
	{.rx = desfire_Desfire_step11_rx, .rx_len = sizeof(desfire_Desfire_step11_rx), .err = 0},
	{.rx = desfire_Desfire_step12_rx, .rx_len = sizeof(desfire_Desfire_step12_rx), .err = 0},
};

#define DESFIRE_DESFIRE_READ_STEP_COUNT ARRAY_SIZE(desfire_Desfire_read_steps)

#endif
