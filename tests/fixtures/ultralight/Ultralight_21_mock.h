/* Auto-generated from Flipper Ultralight_21.nfc — do not edit. */
#ifndef ULTRALIGHT_FIXTURE_ULTRALIGHT_21_H_
#define ULTRALIGHT_FIXTURE_ULTRALIGHT_21_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

static const uint8_t ultralight_Ultralight_21_model[] = {
0x01U, 0x02U, 0x29U, 0x00U, 0x34U, 0xBFU, 0xABU, 0xA8U,
0xB1U, 0xAEU, 0x73U, 0xD6U, 0xBAU, 0x00U, 0x70U, 0x08U,
0xFFU, 0xFFU, 0xFFU, 0xFCU, 0x45U, 0xD9U, 0xBBU, 0xA0U,
0x5DU, 0x9DU, 0xFAU, 0x00U, 0x80U, 0x70U, 0x38U, 0x40U,
0x12U, 0x30U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0xACU, 0xA1U, 0x0DU, 0xE4U,
0x80U, 0x70U, 0x38U, 0x40U, 0x00U, 0x57U, 0xA0U, 0x01U,
0x00U, 0x08U, 0xC1U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U,
0xACU, 0xA1U, 0x0DU, 0xE4U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xBDU,
0x00U, 0x00U, 0x00U, 0xFFU, 0x00U, 0x05U, 0x00U, 0x00U,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U,
0x01U, 0x00U, 0x34U, 0x21U, 0x01U, 0x01U, 0x00U, 0x0EU,
0x03U, 0x00U, 0x03U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x03U, 0x00U, 0x00U, 0x00U,
};

#define ULTRALIGHT_ULTRALIGHT_21_MODEL_LEN sizeof(ultralight_Ultralight_21_model)

static const uint8_t ultralight_Ultralight_21_step0_rx[] = {
0x00U, 0x34U, 0x21U, 0x01U, 0x01U, 0x00U, 0x0EU, 0x03U,
};

static const uint8_t ultralight_Ultralight_21_step0_tx[] = {
0x60U,
};

static const uint8_t ultralight_Ultralight_21_step1_rx[] = {
0x34U, 0xBFU, 0xABU, 0xA8U, 0xB1U, 0xAEU, 0x73U, 0xD6U,
0xBAU, 0x00U, 0x70U, 0x08U, 0xFFU, 0xFFU, 0xFFU, 0xFCU,
};

static const uint8_t ultralight_Ultralight_21_step1_tx[] = {
0x30U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step2_rx[] = {
0x45U, 0xD9U, 0xBBU, 0xA0U, 0x5DU, 0x9DU, 0xFAU, 0x00U,
0x80U, 0x70U, 0x38U, 0x40U, 0x12U, 0x30U, 0x02U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step2_tx[] = {
0x30U, 0x04U,
};

static const uint8_t ultralight_Ultralight_21_step3_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0xACU, 0xA1U, 0x0DU, 0xE4U, 0x80U, 0x70U, 0x38U, 0x40U,
};

static const uint8_t ultralight_Ultralight_21_step3_tx[] = {
0x30U, 0x08U,
};

static const uint8_t ultralight_Ultralight_21_step4_rx[] = {
0x00U, 0x57U, 0xA0U, 0x01U, 0x00U, 0x08U, 0xC1U, 0x40U,
0x00U, 0x00U, 0x00U, 0x00U, 0xACU, 0xA1U, 0x0DU, 0xE4U,
};

static const uint8_t ultralight_Ultralight_21_step4_tx[] = {
0x30U, 0x0CU,
};

static const uint8_t ultralight_Ultralight_21_step5_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step5_tx[] = {
0x30U, 0x10U,
};

static const uint8_t ultralight_Ultralight_21_step6_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step6_tx[] = {
0x30U, 0x14U,
};

static const uint8_t ultralight_Ultralight_21_step7_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step7_tx[] = {
0x30U, 0x18U,
};

static const uint8_t ultralight_Ultralight_21_step8_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step8_tx[] = {
0x30U, 0x1CU,
};

static const uint8_t ultralight_Ultralight_21_step9_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step9_tx[] = {
0x30U, 0x20U,
};

static const uint8_t ultralight_Ultralight_21_step10_rx[] = {
0x00U, 0x00U, 0x00U, 0xBDU, 0x00U, 0x00U, 0x00U, 0xFFU,
0x00U, 0x05U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
};

static const uint8_t ultralight_Ultralight_21_step10_tx[] = {
0x30U, 0x24U,
};

static const uint8_t ultralight_Ultralight_21_step11_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step11_tx[] = {
0x30U, 0x28U,
};

static const uint8_t ultralight_Ultralight_21_step12_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step12_tx[] = {
0x3CU, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step13_rx[] = {
0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step13_tx[] = {
0x39U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step14_rx[] = {
0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step14_tx[] = {
0x39U, 0x01U,
};

static const uint8_t ultralight_Ultralight_21_step15_rx[] = {
0x00U, 0x00U, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step15_tx[] = {
0x39U, 0x02U,
};

static const uint8_t ultralight_Ultralight_21_step16_rx[] = {
0x00U,
};

static const uint8_t ultralight_Ultralight_21_step16_tx[] = {
0x3EU, 0x00U,
};

static const uint8_t ultralight_Ultralight_21_step17_rx[] = {
0x00U,
};

static const uint8_t ultralight_Ultralight_21_step17_tx[] = {
0x3EU, 0x01U,
};

static const uint8_t ultralight_Ultralight_21_step18_rx[] = {
0x00U,
};

static const uint8_t ultralight_Ultralight_21_step18_tx[] = {
0x3EU, 0x02U,
};

static const nfc_session_mock_step_t ultralight_Ultralight_21_read_steps[] = {
	{ .rx = ultralight_Ultralight_21_step0_rx, .rx_len = sizeof(ultralight_Ultralight_21_step0_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step1_rx, .rx_len = sizeof(ultralight_Ultralight_21_step1_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step2_rx, .rx_len = sizeof(ultralight_Ultralight_21_step2_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step3_rx, .rx_len = sizeof(ultralight_Ultralight_21_step3_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step4_rx, .rx_len = sizeof(ultralight_Ultralight_21_step4_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step5_rx, .rx_len = sizeof(ultralight_Ultralight_21_step5_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step6_rx, .rx_len = sizeof(ultralight_Ultralight_21_step6_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step7_rx, .rx_len = sizeof(ultralight_Ultralight_21_step7_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step8_rx, .rx_len = sizeof(ultralight_Ultralight_21_step8_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step9_rx, .rx_len = sizeof(ultralight_Ultralight_21_step9_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step10_rx, .rx_len = sizeof(ultralight_Ultralight_21_step10_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step11_rx, .rx_len = sizeof(ultralight_Ultralight_21_step11_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step12_rx, .rx_len = sizeof(ultralight_Ultralight_21_step12_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step13_rx, .rx_len = sizeof(ultralight_Ultralight_21_step13_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step14_rx, .rx_len = sizeof(ultralight_Ultralight_21_step14_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step15_rx, .rx_len = sizeof(ultralight_Ultralight_21_step15_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step16_rx, .rx_len = sizeof(ultralight_Ultralight_21_step16_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step17_rx, .rx_len = sizeof(ultralight_Ultralight_21_step17_rx), .err = 0 },
	{ .rx = ultralight_Ultralight_21_step18_rx, .rx_len = sizeof(ultralight_Ultralight_21_step18_rx), .err = 0 },
};

#define ULTRALIGHT_ULTRALIGHT_21_READ_STEP_COUNT ARRAY_SIZE(ultralight_Ultralight_21_read_steps)

static const uint8_t *const ultralight_Ultralight_21_read_tx_steps[] = {
	ultralight_Ultralight_21_step0_tx,
	ultralight_Ultralight_21_step1_tx,
	ultralight_Ultralight_21_step2_tx,
	ultralight_Ultralight_21_step3_tx,
	ultralight_Ultralight_21_step4_tx,
	ultralight_Ultralight_21_step5_tx,
	ultralight_Ultralight_21_step6_tx,
	ultralight_Ultralight_21_step7_tx,
	ultralight_Ultralight_21_step8_tx,
	ultralight_Ultralight_21_step9_tx,
	ultralight_Ultralight_21_step10_tx,
	ultralight_Ultralight_21_step11_tx,
	ultralight_Ultralight_21_step12_tx,
	ultralight_Ultralight_21_step13_tx,
	ultralight_Ultralight_21_step14_tx,
	ultralight_Ultralight_21_step15_tx,
	ultralight_Ultralight_21_step16_tx,
	ultralight_Ultralight_21_step17_tx,
	ultralight_Ultralight_21_step18_tx,
};

static const size_t ultralight_Ultralight_21_read_tx_lens[] = {
	sizeof(ultralight_Ultralight_21_step0_tx),
	sizeof(ultralight_Ultralight_21_step1_tx),
	sizeof(ultralight_Ultralight_21_step2_tx),
	sizeof(ultralight_Ultralight_21_step3_tx),
	sizeof(ultralight_Ultralight_21_step4_tx),
	sizeof(ultralight_Ultralight_21_step5_tx),
	sizeof(ultralight_Ultralight_21_step6_tx),
	sizeof(ultralight_Ultralight_21_step7_tx),
	sizeof(ultralight_Ultralight_21_step8_tx),
	sizeof(ultralight_Ultralight_21_step9_tx),
	sizeof(ultralight_Ultralight_21_step10_tx),
	sizeof(ultralight_Ultralight_21_step11_tx),
	sizeof(ultralight_Ultralight_21_step12_tx),
	sizeof(ultralight_Ultralight_21_step13_tx),
	sizeof(ultralight_Ultralight_21_step14_tx),
	sizeof(ultralight_Ultralight_21_step15_tx),
	sizeof(ultralight_Ultralight_21_step16_tx),
	sizeof(ultralight_Ultralight_21_step17_tx),
	sizeof(ultralight_Ultralight_21_step18_tx),
};

#define ULTRALIGHT_ULTRALIGHT_21_READ_TX_STEP_COUNT ARRAY_SIZE(ultralight_Ultralight_21_read_tx_steps)

#endif
