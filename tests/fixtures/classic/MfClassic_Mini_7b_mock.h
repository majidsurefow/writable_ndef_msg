/* Auto-generated from Flipper MfClassic_Mini_7b.nfc — do not edit. */
#ifndef CLASSIC_FIXTURE_MFCLASSIC_MINI_7B_H_
#define CLASSIC_FIXTURE_MFCLASSIC_MINI_7B_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

static const uint8_t classic_MfClassic_Mini_7b_model[] = {
0x01U, 0x00U, 0x07U, 0x04U, 0xDEU, 0xADU, 0xCAU, 0xFEU,
0xEDU, 0xFAU, 0x09U, 0x44U, 0x00U, 0xFFU, 0xFFU, 0x0FU,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x1FU, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x88U, 0x04U, 0xDEU,
0xADU, 0xCAU, 0xBDU, 0x09U, 0x44U, 0x00U, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFEU, 0xEDU, 0xFAU,
0xE9U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_MODEL_LEN sizeof(classic_MfClassic_Mini_7b_model)

static const uint8_t classic_MfClassic_Mini_7b_step0_rx[] = {
};

static const uint8_t classic_MfClassic_Mini_7b_step1_rx[] = {
};

static const uint8_t classic_MfClassic_Mini_7b_step2_rx[] = {
0x10U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t classic_MfClassic_Mini_7b_step3_rx[] = {
0x9AU, 0xC9U, 0x81U, 0x14U,
};

static const uint8_t classic_MfClassic_Mini_7b_step4_rx[] = {
0x78U, 0xF7U, 0x3FU, 0xDEU, 0x6AU, 0x1FU, 0xA6U, 0x70U,
0x84U, 0x12U, 0x58U, 0xD0U, 0x5AU, 0x75U, 0x17U, 0xA8U,
0x52U, 0x6CU,
};

static const uint8_t classic_MfClassic_Mini_7b_step5_rx[] = {
0x6AU, 0x05U, 0xD0U, 0xCFU, 0x21U, 0x9DU, 0x6AU, 0x2AU,
0x38U, 0x26U, 0xAEU, 0xCCU, 0x6AU, 0xABU, 0xB9U, 0x59U,
0xC4U, 0xB2U,
};

static const uint8_t classic_MfClassic_Mini_7b_step6_rx[] = {
0xD1U, 0x3FU, 0x43U, 0x63U, 0x78U, 0x4CU, 0x73U, 0xE2U,
0x6CU, 0xFAU, 0x09U, 0xC6U, 0x15U, 0x7AU, 0xCCU, 0x85U,
0x7FU, 0x49U,
};

static const uint8_t classic_MfClassic_Mini_7b_step7_rx[] = {
0xA8U, 0x98U, 0x9BU, 0xF7U, 0x19U, 0x68U, 0xCBU, 0xB3U,
0xF0U, 0x5CU, 0xDEU, 0x77U, 0x49U, 0x54U, 0x7AU, 0x19U,
0x5BU, 0xCBU,
};

static const uint8_t classic_MfClassic_Mini_7b_step8_rx[] = {
0x10U, 0x00U, 0x00U, 0x01U,
};

static const uint8_t classic_MfClassic_Mini_7b_step9_rx[] = {
0x08U, 0x76U, 0x12U, 0x20U,
};

static const uint8_t classic_MfClassic_Mini_7b_step10_rx[] = {
0xB0U, 0xC3U, 0x38U, 0xBDU, 0x15U, 0x06U, 0x14U, 0x9BU,
0x24U, 0x80U, 0xFCU, 0x7EU, 0x27U, 0x7EU, 0x62U, 0x5DU,
0x6EU, 0xA6U,
};

static const uint8_t classic_MfClassic_Mini_7b_step11_rx[] = {
0xD8U, 0x42U, 0xFBU, 0x9AU, 0x83U, 0xD3U, 0xD7U, 0xA5U,
0xBFU, 0x2CU, 0x23U, 0xB7U, 0x96U, 0x3EU, 0x8DU, 0xA4U,
0xB8U, 0x36U,
};

static const uint8_t classic_MfClassic_Mini_7b_step12_rx[] = {
0x95U, 0x9EU, 0xC6U, 0x97U, 0xF6U, 0x25U, 0x96U, 0xCBU,
0xB3U, 0x46U, 0xC3U, 0x08U, 0x30U, 0x05U, 0xAFU, 0x2CU,
0x36U, 0x76U,
};

static const uint8_t classic_MfClassic_Mini_7b_step13_rx[] = {
0x1BU, 0x97U, 0x05U, 0xE0U, 0x52U, 0xB4U, 0xC0U, 0xB6U,
0xCBU, 0x84U, 0x5EU, 0xA1U, 0x72U, 0xAFU, 0x32U, 0x0CU,
0x9BU, 0x15U,
};

static const uint8_t classic_MfClassic_Mini_7b_step14_rx[] = {
0x10U, 0x00U, 0x00U, 0x02U,
};

static const uint8_t classic_MfClassic_Mini_7b_step15_rx[] = {
0x20U, 0x7CU, 0x07U, 0x45U,
};

static const uint8_t classic_MfClassic_Mini_7b_step16_rx[] = {
0x33U, 0x18U, 0x56U, 0xBFU, 0x90U, 0x80U, 0x20U, 0xE1U,
0x87U, 0x46U, 0x03U, 0xE6U, 0x58U, 0x2DU, 0xCDU, 0x0AU,
0xE9U, 0x8DU,
};

static const uint8_t classic_MfClassic_Mini_7b_step17_rx[] = {
0x57U, 0xCCU, 0xFAU, 0xA4U, 0x17U, 0xE3U, 0x8FU, 0x49U,
0x1EU, 0x02U, 0x47U, 0x1CU, 0x11U, 0x60U, 0xF8U, 0xF1U,
0x54U, 0x77U,
};

static const uint8_t classic_MfClassic_Mini_7b_step18_rx[] = {
0x21U, 0x72U, 0x89U, 0xFCU, 0x9BU, 0x9EU, 0xB6U, 0x5CU,
0x96U, 0xE2U, 0xAEU, 0xD2U, 0xA2U, 0x86U, 0x2AU, 0x97U,
0xD2U, 0x37U,
};

static const uint8_t classic_MfClassic_Mini_7b_step19_rx[] = {
0x94U, 0x45U, 0xB9U, 0x3EU, 0x9BU, 0x61U, 0x70U, 0x11U,
0x5AU, 0xB5U, 0x22U, 0x20U, 0xF8U, 0x3FU, 0xC6U, 0xC9U,
0xE1U, 0xADU,
};

static const uint8_t classic_MfClassic_Mini_7b_step20_rx[] = {
0x10U, 0x00U, 0x00U, 0x03U,
};

static const uint8_t classic_MfClassic_Mini_7b_step21_rx[] = {
0xC8U, 0xCBU, 0x6CU, 0xFAU,
};

static const uint8_t classic_MfClassic_Mini_7b_step22_rx[] = {
0x56U, 0x49U, 0x0DU, 0x57U, 0x93U, 0x1EU, 0x4FU, 0x29U,
0xFDU, 0xD1U, 0x60U, 0x48U, 0xD6U, 0x50U, 0xAEU, 0x7FU,
0xA6U, 0xDFU,
};

static const uint8_t classic_MfClassic_Mini_7b_step23_rx[] = {
0xECU, 0x47U, 0x23U, 0xDDU, 0xEAU, 0x9BU, 0x6BU, 0xFEU,
0x37U, 0x08U, 0x1AU, 0x1DU, 0x00U, 0xCCU, 0x94U, 0x65U,
0x69U, 0x98U,
};

static const uint8_t classic_MfClassic_Mini_7b_step24_rx[] = {
0xE3U, 0xC9U, 0xBAU, 0xA9U, 0x35U, 0xBAU, 0x9FU, 0x17U,
0xCEU, 0x3EU, 0xCAU, 0xC6U, 0xEFU, 0x05U, 0x6AU, 0x62U,
0xA6U, 0x18U,
};

static const uint8_t classic_MfClassic_Mini_7b_step25_rx[] = {
0xCDU, 0xD1U, 0x9AU, 0x68U, 0x93U, 0x42U, 0x55U, 0xE2U,
0xD8U, 0x50U, 0xB6U, 0xE0U, 0xFCU, 0x8BU, 0x87U, 0x8AU,
0x1AU, 0xC3U,
};

static const uint8_t classic_MfClassic_Mini_7b_step26_rx[] = {
0x10U, 0x00U, 0x00U, 0x04U,
};

static const uint8_t classic_MfClassic_Mini_7b_step27_rx[] = {
0x96U, 0x4BU, 0x14U, 0x4BU,
};

static const uint8_t classic_MfClassic_Mini_7b_step28_rx[] = {
0x93U, 0xFDU, 0x70U, 0xEDU, 0x22U, 0x23U, 0x7AU, 0x61U,
0x1DU, 0x08U, 0x09U, 0xE2U, 0x80U, 0xD8U, 0x72U, 0x03U,
0x48U, 0x38U,
};

static const uint8_t classic_MfClassic_Mini_7b_step29_rx[] = {
0x85U, 0x8FU, 0x0FU, 0xA4U, 0xBDU, 0xD5U, 0xD5U, 0x9FU,
0x63U, 0x88U, 0x3FU, 0x4FU, 0x66U, 0x3FU, 0x68U, 0x7EU,
0x86U, 0x38U,
};

static const uint8_t classic_MfClassic_Mini_7b_step30_rx[] = {
0x9DU, 0x6CU, 0xA1U, 0xE0U, 0x8BU, 0x9FU, 0xC9U, 0x55U,
0x54U, 0x19U, 0x32U, 0x71U, 0x56U, 0xFFU, 0x2AU, 0x38U,
0xC7U, 0xABU,
};

static const uint8_t classic_MfClassic_Mini_7b_step31_rx[] = {
0x3EU, 0xB3U, 0xB0U, 0xDDU, 0x5BU, 0x4BU, 0x6CU, 0xA0U,
0xD9U, 0x91U, 0xECU, 0xB8U, 0xB4U, 0xBBU, 0x41U, 0x7EU,
0x34U, 0xD9U,
};

static const nfc_session_mock_step_t classic_MfClassic_Mini_7b_read_steps[] = {
	{ .rx = classic_MfClassic_Mini_7b_step0_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step0_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step1_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step1_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step2_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step2_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step3_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step3_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step4_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step4_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step5_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step5_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step6_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step6_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step7_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step7_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step8_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step8_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step9_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step9_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step10_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step10_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step11_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step11_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step12_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step12_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step13_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step13_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step14_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step14_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step15_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step15_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step16_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step16_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step17_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step17_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step18_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step18_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step19_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step19_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step20_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step20_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step21_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step21_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step22_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step22_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step23_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step23_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step24_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step24_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step25_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step25_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step26_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step26_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step27_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step27_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step28_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step28_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step29_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step29_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step30_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step30_rx), .err = 0 },
	{ .rx = classic_MfClassic_Mini_7b_step31_rx, .rx_len = sizeof(classic_MfClassic_Mini_7b_step31_rx), .err = 0 },
};

#define CLASSIC_MFCLASSIC_MINI_7B_READ_STEP_COUNT ARRAY_SIZE(classic_MfClassic_Mini_7b_read_steps)

#define CLASSIC_MFCLASSIC_MINI_7B_TX_STEP_COUNT 32U

static const uint8_t classic_MfClassic_Mini_7b_step0_tx[] = {
0x60U, 0xFEU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP0_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step1_tx[] = {
0x60U, 0x3EU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP1_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step2_tx[] = {
0x60U, 0x00U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP2_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step3_tx[] = {
0x5CU, 0x7BU, 0xBBU, 0x38U, 0x9AU, 0xC9U, 0x81U, 0x14U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP3_TX_LEN 8U
static const uint8_t classic_MfClassic_Mini_7b_step4_tx[] = {
0x0EU, 0x55U, 0x22U, 0xEEU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP4_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step5_tx[] = {
0x41U, 0xF4U, 0x72U, 0x9AU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP5_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step6_tx[] = {
0x85U, 0xC6U, 0x31U, 0x6EU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP6_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step7_tx[] = {
0x92U, 0x46U, 0x47U, 0xF5U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP7_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step8_tx[] = {
0x60U, 0x04U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP8_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step9_tx[] = {
0x15U, 0x95U, 0x7EU, 0x10U, 0xBFU, 0xB4U, 0x4BU, 0x64U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP9_TX_LEN 8U
static const uint8_t classic_MfClassic_Mini_7b_step10_tx[] = {
0x21U, 0xD6U, 0x2BU, 0xD6U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP10_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step11_tx[] = {
0xC5U, 0xD8U, 0x00U, 0x80U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP11_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step12_tx[] = {
0x16U, 0x82U, 0xBFU, 0x2BU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP12_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step13_tx[] = {
0x5CU, 0xDBU, 0xB0U, 0xCEU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP13_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step14_tx[] = {
0x60U, 0x08U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP14_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step15_tx[] = {
0xDCU, 0xCAU, 0xD7U, 0x79U, 0x4EU, 0xF9U, 0xB4U, 0xCDU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP15_TX_LEN 8U
static const uint8_t classic_MfClassic_Mini_7b_step16_tx[] = {
0x1EU, 0x3AU, 0x87U, 0xE9U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP16_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step17_tx[] = {
0x75U, 0xCCU, 0xB6U, 0x33U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP17_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step18_tx[] = {
0xD0U, 0x11U, 0x92U, 0xC6U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP18_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step19_tx[] = {
0x20U, 0xD2U, 0x0EU, 0x03U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP19_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step20_tx[] = {
0x60U, 0x0CU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP20_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step21_tx[] = {
0x34U, 0xC0U, 0xF8U, 0x92U, 0x11U, 0x8CU, 0x86U, 0x36U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP21_TX_LEN 8U
static const uint8_t classic_MfClassic_Mini_7b_step22_tx[] = {
0x2FU, 0xB2U, 0xD6U, 0x15U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP22_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step23_tx[] = {
0xB3U, 0xE0U, 0x0DU, 0x13U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP23_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step24_tx[] = {
0x6EU, 0x0DU, 0xA1U, 0x87U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP24_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step25_tx[] = {
0xB5U, 0xC4U, 0xEFU, 0x3DU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP25_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step26_tx[] = {
0x60U, 0x10U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP26_TX_LEN 2U
static const uint8_t classic_MfClassic_Mini_7b_step27_tx[] = {
0xDCU, 0xE1U, 0x57U, 0x7AU, 0x4BU, 0x41U, 0x73U, 0x5AU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP27_TX_LEN 8U
static const uint8_t classic_MfClassic_Mini_7b_step28_tx[] = {
0x1CU, 0x7BU, 0x38U, 0x6BU,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP28_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step29_tx[] = {
0x70U, 0x72U, 0x6BU, 0x66U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP29_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step30_tx[] = {
0x49U, 0xA7U, 0x5DU, 0x91U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP30_TX_LEN 4U
static const uint8_t classic_MfClassic_Mini_7b_step31_tx[] = {
0x4AU, 0xFCU, 0xC4U, 0xB2U,
};

#define CLASSIC_MFCLASSIC_MINI_7B_STEP31_TX_LEN 4U
static const uint8_t *const classic_MfClassic_Mini_7b_tx_steps[CLASSIC_MFCLASSIC_MINI_7B_TX_STEP_COUNT] = {
	classic_MfClassic_Mini_7b_step0_tx,
	classic_MfClassic_Mini_7b_step1_tx,
	classic_MfClassic_Mini_7b_step2_tx,
	classic_MfClassic_Mini_7b_step3_tx,
	classic_MfClassic_Mini_7b_step4_tx,
	classic_MfClassic_Mini_7b_step5_tx,
	classic_MfClassic_Mini_7b_step6_tx,
	classic_MfClassic_Mini_7b_step7_tx,
	classic_MfClassic_Mini_7b_step8_tx,
	classic_MfClassic_Mini_7b_step9_tx,
	classic_MfClassic_Mini_7b_step10_tx,
	classic_MfClassic_Mini_7b_step11_tx,
	classic_MfClassic_Mini_7b_step12_tx,
	classic_MfClassic_Mini_7b_step13_tx,
	classic_MfClassic_Mini_7b_step14_tx,
	classic_MfClassic_Mini_7b_step15_tx,
	classic_MfClassic_Mini_7b_step16_tx,
	classic_MfClassic_Mini_7b_step17_tx,
	classic_MfClassic_Mini_7b_step18_tx,
	classic_MfClassic_Mini_7b_step19_tx,
	classic_MfClassic_Mini_7b_step20_tx,
	classic_MfClassic_Mini_7b_step21_tx,
	classic_MfClassic_Mini_7b_step22_tx,
	classic_MfClassic_Mini_7b_step23_tx,
	classic_MfClassic_Mini_7b_step24_tx,
	classic_MfClassic_Mini_7b_step25_tx,
	classic_MfClassic_Mini_7b_step26_tx,
	classic_MfClassic_Mini_7b_step27_tx,
	classic_MfClassic_Mini_7b_step28_tx,
	classic_MfClassic_Mini_7b_step29_tx,
	classic_MfClassic_Mini_7b_step30_tx,
	classic_MfClassic_Mini_7b_step31_tx
};

static const size_t classic_MfClassic_Mini_7b_tx_lens[CLASSIC_MFCLASSIC_MINI_7B_TX_STEP_COUNT] = {
	CLASSIC_MFCLASSIC_MINI_7B_STEP0_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP1_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP2_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP3_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP4_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP5_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP6_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP7_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP8_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP9_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP10_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP11_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP12_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP13_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP14_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP15_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP16_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP17_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP18_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP19_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP20_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP21_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP22_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP23_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP24_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP25_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP26_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP27_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP28_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP29_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP30_TX_LEN,
	CLASSIC_MFCLASSIC_MINI_7B_STEP31_TX_LEN
};

#endif
