/* Auto-generated from Flipper MfClassic_1K_4b.nfc — do not edit. */
#ifndef CLASSIC_FIXTURE_MFCLASSIC_1K_4B_H_
#define CLASSIC_FIXTURE_MFCLASSIC_1K_4B_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

static const uint8_t classic_MfClassic_1K_4b_model[] = {
0x01U, 0x01U, 0x04U, 0x04U, 0xDEU, 0xADU, 0xBEU, 0x08U,
0x00U, 0x04U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x04U, 0xDEU, 0xADU, 0xBEU, 0x00U, 0x08U,
0x00U, 0x04U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0x07U, 0x80U, 0x69U, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU,
};

#define CLASSIC_MFCLASSIC_1K_4B_MODEL_LEN sizeof(classic_MfClassic_1K_4b_model)

static const uint8_t classic_MfClassic_1K_4b_step0_rx[] = {
};

static const uint8_t classic_MfClassic_1K_4b_step1_rx[] = {
0x20U, 0x00U, 0x00U, 0x3EU,
};

static const uint8_t classic_MfClassic_1K_4b_step2_rx[] = {
0x10U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t classic_MfClassic_1K_4b_step3_rx[] = {
0xFFU, 0xF9U, 0xF4U, 0x82U,
};

static const uint8_t classic_MfClassic_1K_4b_step4_rx[] = {
0xDDU, 0xA1U, 0x5CU, 0xD6U, 0xF5U, 0x8BU, 0xA3U, 0x85U,
0xFEU, 0x54U, 0x69U, 0xEBU, 0xD8U, 0x89U, 0x12U, 0x8BU,
0x92U, 0xDBU,
};

static const uint8_t classic_MfClassic_1K_4b_step5_rx[] = {
0x46U, 0x39U, 0x89U, 0xEBU, 0xF9U, 0xA9U, 0xBCU, 0x85U,
0x5BU, 0x95U, 0x94U, 0xCFU, 0x88U, 0xACU, 0xD2U, 0x51U,
0x92U, 0xD3U,
};

static const uint8_t classic_MfClassic_1K_4b_step6_rx[] = {
0x98U, 0x78U, 0xCEU, 0x52U, 0x1EU, 0xE2U, 0xF2U, 0xB2U,
0x76U, 0x5EU, 0xD0U, 0xE1U, 0x4DU, 0x3CU, 0x7FU, 0x3BU,
0x2BU, 0x3AU,
};

static const uint8_t classic_MfClassic_1K_4b_step7_rx[] = {
0x7CU, 0x6AU, 0x24U, 0xDDU, 0x77U, 0x2CU, 0xFBU, 0x23U,
0xA0U, 0x2BU, 0xDBU, 0x54U, 0x18U, 0xC5U, 0x14U, 0x62U,
0xE5U, 0x3BU,
};

static const uint8_t classic_MfClassic_1K_4b_step8_rx[] = {
0x10U, 0x00U, 0x00U, 0x01U,
};

static const uint8_t classic_MfClassic_1K_4b_step9_rx[] = {
0x64U, 0xA4U, 0xC9U, 0x8FU,
};

static const uint8_t classic_MfClassic_1K_4b_step10_rx[] = {
0x50U, 0xCEU, 0x0AU, 0x58U, 0x3BU, 0xADU, 0x9AU, 0xEBU,
0xEAU, 0x7EU, 0x32U, 0xA2U, 0xC6U, 0x44U, 0x5BU, 0x7CU,
0x5AU, 0x1BU,
};

static const uint8_t classic_MfClassic_1K_4b_step11_rx[] = {
0x03U, 0x15U, 0xACU, 0x55U, 0x3FU, 0xFCU, 0x18U, 0xB7U,
0x07U, 0x67U, 0x4DU, 0x89U, 0x07U, 0x5FU, 0xB1U, 0x5FU,
0x0EU, 0x18U,
};

static const uint8_t classic_MfClassic_1K_4b_step12_rx[] = {
0x4FU, 0x5FU, 0x77U, 0x0CU, 0xD7U, 0x76U, 0xEFU, 0xDEU,
0xAFU, 0x79U, 0x8FU, 0xCBU, 0xE5U, 0xF9U, 0x44U, 0x60U,
0x4EU, 0x8AU,
};

static const uint8_t classic_MfClassic_1K_4b_step13_rx[] = {
0xFDU, 0x84U, 0x41U, 0x66U, 0xFAU, 0x23U, 0x85U, 0x6AU,
0xD7U, 0x40U, 0xD6U, 0x16U, 0x62U, 0x4EU, 0xA8U, 0x00U,
0xB8U, 0x19U,
};

static const uint8_t classic_MfClassic_1K_4b_step14_rx[] = {
0x10U, 0x00U, 0x00U, 0x02U,
};

static const uint8_t classic_MfClassic_1K_4b_step15_rx[] = {
0x60U, 0xD3U, 0x4EU, 0x57U,
};

static const uint8_t classic_MfClassic_1K_4b_step16_rx[] = {
0x61U, 0x84U, 0x57U, 0xFEU, 0x6DU, 0x80U, 0x5AU, 0x52U,
0x21U, 0xBDU, 0x7BU, 0x38U, 0x15U, 0x87U, 0xD8U, 0x2DU,
0x4EU, 0x09U,
};

static const uint8_t classic_MfClassic_1K_4b_step17_rx[] = {
0x32U, 0x0EU, 0xE9U, 0x4AU, 0x0DU, 0x83U, 0x92U, 0x4BU,
0xFAU, 0x3FU, 0x7BU, 0x53U, 0xDAU, 0xFFU, 0x31U, 0x07U,
0xA2U, 0x13U,
};

static const uint8_t classic_MfClassic_1K_4b_step18_rx[] = {
0xBDU, 0x2DU, 0x5DU, 0xE8U, 0xA4U, 0x3AU, 0xF4U, 0x31U,
0x80U, 0x57U, 0x75U, 0xD8U, 0xF6U, 0x5CU, 0x45U, 0x54U,
0xA4U, 0x93U,
};

static const uint8_t classic_MfClassic_1K_4b_step19_rx[] = {
0xFDU, 0x02U, 0x49U, 0x45U, 0x79U, 0x0DU, 0x3DU, 0xC3U,
0xDDU, 0x9AU, 0x71U, 0xC8U, 0xB6U, 0xB4U, 0x50U, 0xA6U,
0xFDU, 0x47U,
};

static const uint8_t classic_MfClassic_1K_4b_step20_rx[] = {
0x10U, 0x00U, 0x00U, 0x03U,
};

static const uint8_t classic_MfClassic_1K_4b_step21_rx[] = {
0xB8U, 0x66U, 0x0DU, 0xFBU,
};

static const uint8_t classic_MfClassic_1K_4b_step22_rx[] = {
0x0EU, 0x18U, 0x63U, 0x24U, 0x1FU, 0x43U, 0xA5U, 0x11U,
0x98U, 0xD8U, 0x47U, 0x20U, 0x99U, 0x5FU, 0x64U, 0xF0U,
0x85U, 0x58U,
};

static const uint8_t classic_MfClassic_1K_4b_step23_rx[] = {
0x82U, 0xCAU, 0xACU, 0x9DU, 0x7BU, 0xB8U, 0x81U, 0x10U,
0x6CU, 0x15U, 0xA1U, 0xA1U, 0x84U, 0x66U, 0x12U, 0x84U,
0xFBU, 0x1AU,
};

static const uint8_t classic_MfClassic_1K_4b_step24_rx[] = {
0xABU, 0x24U, 0xA3U, 0xAEU, 0x50U, 0x33U, 0xDBU, 0xF1U,
0xF2U, 0xCDU, 0x13U, 0xCDU, 0x85U, 0x75U, 0xF2U, 0x12U,
0x80U, 0x74U,
};

static const uint8_t classic_MfClassic_1K_4b_step25_rx[] = {
0xC3U, 0xC8U, 0xC8U, 0x6BU, 0x60U, 0x6CU, 0x57U, 0x12U,
0x58U, 0x64U, 0x56U, 0xFEU, 0xB7U, 0xC2U, 0x98U, 0xD7U,
0xD8U, 0x87U,
};

static const uint8_t classic_MfClassic_1K_4b_step26_rx[] = {
0x10U, 0x00U, 0x00U, 0x04U,
};

static const uint8_t classic_MfClassic_1K_4b_step27_rx[] = {
0xCCU, 0x6EU, 0xD3U, 0x6BU,
};

static const uint8_t classic_MfClassic_1K_4b_step28_rx[] = {
0x50U, 0xDCU, 0xBAU, 0xFDU, 0xD0U, 0x91U, 0x16U, 0x1CU,
0x08U, 0x47U, 0x2CU, 0x49U, 0xD7U, 0x31U, 0x6CU, 0xF1U,
0xE3U, 0xFFU,
};

static const uint8_t classic_MfClassic_1K_4b_step29_rx[] = {
0x00U, 0xBAU, 0x8AU, 0x94U, 0xD8U, 0x65U, 0x69U, 0x45U,
0x5BU, 0xB8U, 0x0BU, 0x8AU, 0xFCU, 0xCAU, 0xC7U, 0x6CU,
0xA1U, 0x72U,
};

static const uint8_t classic_MfClassic_1K_4b_step30_rx[] = {
0xA5U, 0xE0U, 0xB5U, 0xE9U, 0xB3U, 0x27U, 0xECU, 0x20U,
0x21U, 0x2DU, 0x0AU, 0xD7U, 0x1CU, 0x2AU, 0x0CU, 0x88U,
0x6EU, 0x95U,
};

static const uint8_t classic_MfClassic_1K_4b_step31_rx[] = {
0x16U, 0x76U, 0x5CU, 0x9EU, 0x72U, 0x27U, 0xFCU, 0x45U,
0xD8U, 0x2FU, 0x00U, 0xB9U, 0xA8U, 0x00U, 0xECU, 0x2FU,
0xFBU, 0xCFU,
};

static const uint8_t classic_MfClassic_1K_4b_step32_rx[] = {
0x10U, 0x00U, 0x00U, 0x05U,
};

static const uint8_t classic_MfClassic_1K_4b_step33_rx[] = {
0x46U, 0x26U, 0x1BU, 0xCAU,
};

static const uint8_t classic_MfClassic_1K_4b_step34_rx[] = {
0x5FU, 0xD3U, 0xF5U, 0xCCU, 0x99U, 0xB0U, 0x08U, 0x39U,
0x78U, 0x2CU, 0xBFU, 0xCEU, 0x6FU, 0xACU, 0x1CU, 0x34U,
0xADU, 0x5BU,
};

static const uint8_t classic_MfClassic_1K_4b_step35_rx[] = {
0x57U, 0x96U, 0x9EU, 0x3BU, 0x78U, 0x8DU, 0x32U, 0x45U,
0xDCU, 0xFFU, 0xA1U, 0xADU, 0xFCU, 0xFBU, 0xE0U, 0x46U,
0xD3U, 0x53U,
};

static const uint8_t classic_MfClassic_1K_4b_step36_rx[] = {
0xABU, 0xC4U, 0x16U, 0xFBU, 0x97U, 0x56U, 0xB7U, 0xAFU,
0x1CU, 0x10U, 0x75U, 0xE7U, 0x59U, 0x94U, 0x9AU, 0x69U,
0x81U, 0xD3U,
};

static const uint8_t classic_MfClassic_1K_4b_step37_rx[] = {
0xE9U, 0x46U, 0x2DU, 0x4BU, 0x63U, 0x2AU, 0x75U, 0xC8U,
0x0AU, 0x33U, 0xE3U, 0xF5U, 0x83U, 0xFFU, 0xD7U, 0x09U,
0xEAU, 0x02U,
};

static const uint8_t classic_MfClassic_1K_4b_step38_rx[] = {
0x10U, 0x00U, 0x00U, 0x06U,
};

static const uint8_t classic_MfClassic_1K_4b_step39_rx[] = {
0xCAU, 0xD4U, 0xB3U, 0xE1U,
};

static const uint8_t classic_MfClassic_1K_4b_step40_rx[] = {
0xEEU, 0x92U, 0xD7U, 0x16U, 0xB5U, 0x94U, 0x3DU, 0x8DU,
0xDCU, 0x69U, 0xCDU, 0x95U, 0x65U, 0xEDU, 0x81U, 0xE2U,
0x07U, 0xDFU,
};

static const uint8_t classic_MfClassic_1K_4b_step41_rx[] = {
0x84U, 0x65U, 0xA1U, 0x4CU, 0x6FU, 0x50U, 0xA0U, 0x4BU,
0x68U, 0xC8U, 0x7CU, 0x61U, 0xA4U, 0x18U, 0x9EU, 0x2FU,
0x04U, 0x4EU,
};

static const uint8_t classic_MfClassic_1K_4b_step42_rx[] = {
0x4CU, 0x04U, 0xA4U, 0x8EU, 0x1DU, 0xA1U, 0x84U, 0x4AU,
0x2BU, 0x8CU, 0x03U, 0xFFU, 0x2EU, 0x23U, 0xBFU, 0xE1U,
0x30U, 0xD2U,
};

static const uint8_t classic_MfClassic_1K_4b_step43_rx[] = {
0xB3U, 0x98U, 0x5AU, 0x18U, 0x28U, 0xD4U, 0x69U, 0xC2U,
0xA2U, 0xF2U, 0x87U, 0x9EU, 0xCFU, 0x71U, 0xDDU, 0x4DU,
0xCDU, 0xEDU,
};

static const uint8_t classic_MfClassic_1K_4b_step44_rx[] = {
0x10U, 0x00U, 0x00U, 0x07U,
};

static const uint8_t classic_MfClassic_1K_4b_step45_rx[] = {
0xB6U, 0x7AU, 0x19U, 0x01U,
};

static const uint8_t classic_MfClassic_1K_4b_step46_rx[] = {
0xD1U, 0xBEU, 0x81U, 0xB4U, 0x22U, 0x52U, 0x79U, 0x32U,
0x06U, 0x1CU, 0x8EU, 0x30U, 0x8DU, 0x43U, 0xE5U, 0xCAU,
0x42U, 0x75U,
};

static const uint8_t classic_MfClassic_1K_4b_step47_rx[] = {
0xA4U, 0x1FU, 0x8EU, 0xBAU, 0x30U, 0x0EU, 0x32U, 0x21U,
0xCCU, 0x50U, 0xE9U, 0x19U, 0xFBU, 0xACU, 0xA1U, 0x9FU,
0x23U, 0x1FU,
};

static const uint8_t classic_MfClassic_1K_4b_step48_rx[] = {
0xC8U, 0x26U, 0xBBU, 0x48U, 0x8DU, 0x2CU, 0x75U, 0xD9U,
0xACU, 0xE5U, 0x81U, 0x93U, 0x81U, 0x33U, 0x4BU, 0x89U,
0x28U, 0xE3U,
};

static const uint8_t classic_MfClassic_1K_4b_step49_rx[] = {
0x13U, 0x6AU, 0x0FU, 0x05U, 0xAFU, 0xE9U, 0x8AU, 0x06U,
0x59U, 0x26U, 0xEFU, 0x07U, 0x41U, 0x21U, 0x10U, 0xC5U,
0x0EU, 0x7AU,
};

static const uint8_t classic_MfClassic_1K_4b_step50_rx[] = {
0x10U, 0x00U, 0x00U, 0x08U,
};

static const uint8_t classic_MfClassic_1K_4b_step51_rx[] = {
0x11U, 0xF3U, 0x8CU, 0xD3U,
};

static const uint8_t classic_MfClassic_1K_4b_step52_rx[] = {
0x55U, 0x34U, 0x4EU, 0x1CU, 0x26U, 0x7DU, 0x8BU, 0x6CU,
0x83U, 0xD3U, 0xE1U, 0xFDU, 0x28U, 0xBDU, 0xD1U, 0xB7U,
0x8CU, 0xEFU,
};

static const uint8_t classic_MfClassic_1K_4b_step53_rx[] = {
0x1FU, 0x70U, 0x63U, 0x7BU, 0xD0U, 0x6AU, 0x6FU, 0x89U,
0x04U, 0xB1U, 0x15U, 0xE2U, 0x3EU, 0x98U, 0xF0U, 0x67U,
0x46U, 0xDBU,
};

static const uint8_t classic_MfClassic_1K_4b_step54_rx[] = {
0x78U, 0x39U, 0x7DU, 0xA2U, 0xDDU, 0xF8U, 0x24U, 0x4FU,
0xC9U, 0x80U, 0x6DU, 0xA5U, 0x7FU, 0x22U, 0xE7U, 0xFEU,
0x92U, 0x2FU,
};

static const uint8_t classic_MfClassic_1K_4b_step55_rx[] = {
0x83U, 0x76U, 0x6CU, 0xEFU, 0x26U, 0x6BU, 0x37U, 0x42U,
0xD3U, 0xD1U, 0x65U, 0x03U, 0x15U, 0xA4U, 0x85U, 0xA3U,
0xE2U, 0xCBU,
};

static const uint8_t classic_MfClassic_1K_4b_step56_rx[] = {
0x10U, 0x00U, 0x00U, 0x09U,
};

static const uint8_t classic_MfClassic_1K_4b_step57_rx[] = {
0x71U, 0x0FU, 0xC3U, 0x2FU,
};

static const uint8_t classic_MfClassic_1K_4b_step58_rx[] = {
0x18U, 0xA3U, 0xB8U, 0x91U, 0xCBU, 0xA2U, 0x97U, 0xAAU,
0x8AU, 0x05U, 0xDBU, 0xD1U, 0x35U, 0x50U, 0xF2U, 0xA4U,
0xE5U, 0xDEU,
};

static const uint8_t classic_MfClassic_1K_4b_step59_rx[] = {
0x0CU, 0x1AU, 0xF9U, 0x8CU, 0x5FU, 0x3AU, 0x47U, 0xC7U,
0x7BU, 0x68U, 0xDEU, 0x93U, 0x9BU, 0x70U, 0x10U, 0xA8U,
0xAAU, 0x13U,
};

static const uint8_t classic_MfClassic_1K_4b_step60_rx[] = {
0x7DU, 0xBFU, 0x8CU, 0x67U, 0xCFU, 0x6AU, 0x7BU, 0x36U,
0x3AU, 0x12U, 0xA6U, 0xA6U, 0x33U, 0xE4U, 0x98U, 0x00U,
0xDFU, 0xA1U,
};

static const uint8_t classic_MfClassic_1K_4b_step61_rx[] = {
0xF4U, 0x4AU, 0x96U, 0x25U, 0x69U, 0xAFU, 0xEFU, 0xC9U,
0xDFU, 0xC2U, 0x46U, 0x47U, 0x73U, 0x32U, 0xB8U, 0xAEU,
0xEEU, 0x9CU,
};

static const uint8_t classic_MfClassic_1K_4b_step62_rx[] = {
0x10U, 0x00U, 0x00U, 0x0AU,
};

static const uint8_t classic_MfClassic_1K_4b_step63_rx[] = {
0xD4U, 0x19U, 0xCBU, 0x5EU,
};

static const uint8_t classic_MfClassic_1K_4b_step64_rx[] = {
0x34U, 0xA3U, 0x9FU, 0x97U, 0xAFU, 0x09U, 0xB9U, 0x4BU,
0x05U, 0x59U, 0x99U, 0xEBU, 0x18U, 0x4DU, 0x58U, 0x92U,
0x44U, 0x2CU,
};

static const uint8_t classic_MfClassic_1K_4b_step65_rx[] = {
0xF2U, 0x01U, 0xACU, 0x35U, 0x62U, 0xB9U, 0x2CU, 0xC0U,
0x74U, 0x14U, 0xA5U, 0xD0U, 0xC4U, 0xBBU, 0xC4U, 0xCDU,
0xC9U, 0x87U,
};

static const uint8_t classic_MfClassic_1K_4b_step66_rx[] = {
0xD9U, 0x3CU, 0xCBU, 0x97U, 0xF5U, 0xADU, 0x2EU, 0x99U,
0xFAU, 0xF3U, 0xF1U, 0x26U, 0x71U, 0xB2U, 0xDEU, 0xA5U,
0xE8U, 0x37U,
};

static const uint8_t classic_MfClassic_1K_4b_step67_rx[] = {
0xA5U, 0x2FU, 0x39U, 0xBDU, 0x75U, 0xADU, 0x86U, 0xE9U,
0x80U, 0x15U, 0x70U, 0x65U, 0xFFU, 0x80U, 0xACU, 0x21U,
0x58U, 0x5FU,
};

static const uint8_t classic_MfClassic_1K_4b_step68_rx[] = {
0x10U, 0x00U, 0x00U, 0x0BU,
};

static const uint8_t classic_MfClassic_1K_4b_step69_rx[] = {
0x5EU, 0x73U, 0xF9U, 0x36U,
};

static const uint8_t classic_MfClassic_1K_4b_step70_rx[] = {
0xECU, 0x13U, 0xCCU, 0x6EU, 0xD7U, 0xA1U, 0x40U, 0x2DU,
0xBDU, 0x34U, 0xE6U, 0xD9U, 0x69U, 0xB0U, 0x4BU, 0x71U,
0x8DU, 0x18U,
};

static const uint8_t classic_MfClassic_1K_4b_step71_rx[] = {
0xCAU, 0x4BU, 0x4BU, 0x0DU, 0xA2U, 0x2BU, 0x3EU, 0x32U,
0x24U, 0x8EU, 0x24U, 0xB5U, 0x41U, 0xFEU, 0xADU, 0x44U,
0x9FU, 0x67U,
};

static const uint8_t classic_MfClassic_1K_4b_step72_rx[] = {
0x8EU, 0xD8U, 0xF5U, 0x08U, 0xAFU, 0x9DU, 0x11U, 0x1FU,
0xBDU, 0x8EU, 0x2AU, 0x98U, 0x83U, 0x50U, 0xA6U, 0x71U,
0xD2U, 0xD9U,
};

static const uint8_t classic_MfClassic_1K_4b_step73_rx[] = {
0x22U, 0x87U, 0x4DU, 0xBDU, 0x7BU, 0xD3U, 0xC6U, 0x53U,
0xF0U, 0xFBU, 0x3AU, 0x9EU, 0xFFU, 0xA3U, 0x21U, 0x96U,
0xA9U, 0x52U,
};

static const uint8_t classic_MfClassic_1K_4b_step74_rx[] = {
0x10U, 0x00U, 0x00U, 0x0CU,
};

static const uint8_t classic_MfClassic_1K_4b_step75_rx[] = {
0xA8U, 0xF9U, 0xE2U, 0x6BU,
};

static const uint8_t classic_MfClassic_1K_4b_step76_rx[] = {
0xB8U, 0xA9U, 0xE8U, 0xBFU, 0x7CU, 0x66U, 0x79U, 0x7CU,
0x54U, 0x1FU, 0x99U, 0x90U, 0x75U, 0x73U, 0x80U, 0x1BU,
0x37U, 0x13U,
};

static const uint8_t classic_MfClassic_1K_4b_step77_rx[] = {
0x1BU, 0xEBU, 0xECU, 0x51U, 0x6CU, 0xFAU, 0xA0U, 0x84U,
0x27U, 0xAAU, 0x85U, 0x8FU, 0xD9U, 0xC3U, 0xB6U, 0x6BU,
0xD0U, 0x19U,
};

static const uint8_t classic_MfClassic_1K_4b_step78_rx[] = {
0x91U, 0xF9U, 0x4AU, 0xF6U, 0x70U, 0x37U, 0xC8U, 0xA5U,
0xDBU, 0x63U, 0x45U, 0x20U, 0xA1U, 0xEFU, 0xABU, 0x29U,
0xF3U, 0x50U,
};

static const uint8_t classic_MfClassic_1K_4b_step79_rx[] = {
0x83U, 0x06U, 0xE6U, 0xE7U, 0x35U, 0x67U, 0xEFU, 0x29U,
0x50U, 0x37U, 0x45U, 0xB7U, 0xF8U, 0xF9U, 0x58U, 0xF8U,
0x92U, 0xBBU,
};

static const uint8_t classic_MfClassic_1K_4b_step80_rx[] = {
0x10U, 0x00U, 0x00U, 0x0DU,
};

static const uint8_t classic_MfClassic_1K_4b_step81_rx[] = {
0xE0U, 0x11U, 0x98U, 0x5EU,
};

static const uint8_t classic_MfClassic_1K_4b_step82_rx[] = {
0x9CU, 0xEBU, 0xCEU, 0x5EU, 0x16U, 0x97U, 0x55U, 0x95U,
0xFEU, 0x8FU, 0x85U, 0x16U, 0xA3U, 0x6DU, 0xE1U, 0xB5U,
0x0CU, 0x42U,
};

static const uint8_t classic_MfClassic_1K_4b_step83_rx[] = {
0x49U, 0xE0U, 0xDAU, 0xFCU, 0x5AU, 0x25U, 0x05U, 0x4DU,
0xABU, 0x2FU, 0xCDU, 0x2AU, 0x51U, 0x9FU, 0x14U, 0xB2U,
0x58U, 0x06U,
};

static const uint8_t classic_MfClassic_1K_4b_step84_rx[] = {
0x77U, 0xD4U, 0xEBU, 0xAFU, 0x0FU, 0x5CU, 0x09U, 0x07U,
0x6FU, 0x71U, 0x43U, 0xD9U, 0x3CU, 0xD2U, 0xE9U, 0x5EU,
0x93U, 0x07U,
};

static const uint8_t classic_MfClassic_1K_4b_step85_rx[] = {
0xDAU, 0xD5U, 0x22U, 0xF8U, 0x49U, 0xABU, 0xF4U, 0xBBU,
0xB0U, 0x43U, 0x5EU, 0x7EU, 0x97U, 0x5AU, 0x42U, 0x3FU,
0xBCU, 0x29U,
};

static const uint8_t classic_MfClassic_1K_4b_step86_rx[] = {
0x10U, 0x00U, 0x00U, 0x0EU,
};

static const uint8_t classic_MfClassic_1K_4b_step87_rx[] = {
0x47U, 0xC3U, 0x05U, 0xF3U,
};

static const uint8_t classic_MfClassic_1K_4b_step88_rx[] = {
0xA2U, 0x41U, 0x22U, 0x2FU, 0x1AU, 0xC5U, 0x47U, 0x13U,
0xE5U, 0x08U, 0x5AU, 0x4CU, 0x85U, 0xD7U, 0x5EU, 0x0BU,
0xEEU, 0xDAU,
};

static const uint8_t classic_MfClassic_1K_4b_step89_rx[] = {
0x84U, 0xA7U, 0x76U, 0x47U, 0xC6U, 0x1DU, 0x0DU, 0xACU,
0x51U, 0x50U, 0xF0U, 0x44U, 0x3AU, 0xE6U, 0xECU, 0x14U,
0x2BU, 0x5FU,
};

static const uint8_t classic_MfClassic_1K_4b_step90_rx[] = {
0xE5U, 0xE5U, 0x83U, 0x26U, 0x34U, 0xA5U, 0x70U, 0x36U,
0x3FU, 0x1DU, 0x31U, 0x77U, 0x8AU, 0xA1U, 0x4AU, 0x1EU,
0x17U, 0xB9U,
};

static const uint8_t classic_MfClassic_1K_4b_step91_rx[] = {
0xE0U, 0x8AU, 0xD4U, 0x02U, 0xEEU, 0x65U, 0x06U, 0xE5U,
0x84U, 0xDFU, 0x46U, 0xB5U, 0x37U, 0xF3U, 0xD6U, 0x66U,
0xCDU, 0xDBU,
};

static const uint8_t classic_MfClassic_1K_4b_step92_rx[] = {
0x10U, 0x00U, 0x00U, 0x0FU,
};

static const uint8_t classic_MfClassic_1K_4b_step93_rx[] = {
0x29U, 0x4BU, 0x79U, 0xEDU,
};

static const uint8_t classic_MfClassic_1K_4b_step94_rx[] = {
0x00U, 0x9FU, 0xA0U, 0x96U, 0x4BU, 0x91U, 0xB7U, 0xC9U,
0xBFU, 0xCFU, 0xB4U, 0xC7U, 0xB8U, 0xFEU, 0x1DU, 0xF7U,
0xA7U, 0xDEU,
};

static const uint8_t classic_MfClassic_1K_4b_step95_rx[] = {
0x7BU, 0xECU, 0x5AU, 0x4DU, 0xB8U, 0xBEU, 0xFDU, 0xB9U,
0xBCU, 0x7EU, 0xFEU, 0x3CU, 0x99U, 0xC9U, 0x3FU, 0x43U,
0x66U, 0x62U,
};

static const uint8_t classic_MfClassic_1K_4b_step96_rx[] = {
0x56U, 0x9AU, 0x84U, 0x9EU, 0xBAU, 0xCFU, 0xA4U, 0x9CU,
0x45U, 0xEFU, 0x27U, 0xFDU, 0xADU, 0x77U, 0xB6U, 0x40U,
0xE4U, 0xB7U,
};

static const uint8_t classic_MfClassic_1K_4b_step97_rx[] = {
0x8BU, 0x85U, 0x01U, 0xF5U, 0xE8U, 0x68U, 0x3BU, 0x5CU,
0xD4U, 0x31U, 0x96U, 0xAAU, 0x86U, 0x83U, 0x42U, 0x1EU,
0xE0U, 0x6BU,
};

static const nfc_session_mock_step_t classic_MfClassic_1K_4b_read_steps[] = {
	{ .rx = classic_MfClassic_1K_4b_step0_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step0_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step1_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step1_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step2_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step2_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step3_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step3_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step4_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step4_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step5_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step5_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step6_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step6_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step7_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step7_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step8_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step8_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step9_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step9_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step10_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step10_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step11_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step11_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step12_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step12_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step13_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step13_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step14_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step14_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step15_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step15_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step16_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step16_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step17_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step17_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step18_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step18_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step19_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step19_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step20_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step20_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step21_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step21_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step22_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step22_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step23_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step23_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step24_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step24_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step25_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step25_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step26_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step26_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step27_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step27_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step28_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step28_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step29_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step29_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step30_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step30_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step31_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step31_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step32_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step32_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step33_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step33_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step34_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step34_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step35_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step35_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step36_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step36_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step37_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step37_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step38_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step38_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step39_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step39_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step40_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step40_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step41_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step41_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step42_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step42_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step43_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step43_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step44_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step44_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step45_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step45_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step46_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step46_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step47_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step47_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step48_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step48_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step49_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step49_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step50_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step50_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step51_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step51_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step52_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step52_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step53_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step53_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step54_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step54_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step55_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step55_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step56_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step56_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step57_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step57_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step58_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step58_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step59_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step59_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step60_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step60_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step61_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step61_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step62_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step62_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step63_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step63_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step64_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step64_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step65_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step65_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step66_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step66_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step67_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step67_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step68_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step68_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step69_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step69_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step70_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step70_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step71_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step71_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step72_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step72_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step73_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step73_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step74_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step74_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step75_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step75_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step76_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step76_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step77_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step77_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step78_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step78_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step79_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step79_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step80_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step80_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step81_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step81_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step82_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step82_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step83_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step83_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step84_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step84_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step85_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step85_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step86_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step86_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step87_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step87_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step88_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step88_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step89_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step89_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step90_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step90_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step91_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step91_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step92_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step92_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step93_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step93_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step94_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step94_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step95_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step95_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step96_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step96_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_4b_step97_rx, .rx_len = sizeof(classic_MfClassic_1K_4b_step97_rx), .err = 0 },
};

#define CLASSIC_MFCLASSIC_1K_4B_READ_STEP_COUNT ARRAY_SIZE(classic_MfClassic_1K_4b_read_steps)

#define CLASSIC_MFCLASSIC_1K_4B_TX_STEP_COUNT 98U

static const uint8_t classic_MfClassic_1K_4b_step0_tx[] = {
0x60U, 0xFEU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP0_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step1_tx[] = {
0x60U, 0x3EU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP1_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step2_tx[] = {
0x60U, 0x00U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP2_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step3_tx[] = {
0x91U, 0x07U, 0xBDU, 0x3AU, 0xFFU, 0xF9U, 0xF4U, 0x82U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP3_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step4_tx[] = {
0x2CU, 0x7AU, 0xFBU, 0xB1U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP4_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step5_tx[] = {
0x7BU, 0xECU, 0x75U, 0x17U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP5_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step6_tx[] = {
0x60U, 0x62U, 0x69U, 0x45U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP6_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step7_tx[] = {
0xEFU, 0x3FU, 0xCBU, 0x41U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP7_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step8_tx[] = {
0x60U, 0x04U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP8_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step9_tx[] = {
0x11U, 0x3FU, 0xD3U, 0x33U, 0xD3U, 0x66U, 0x90U, 0xCBU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP9_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step10_tx[] = {
0x9BU, 0xA7U, 0xC8U, 0x14U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP10_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step11_tx[] = {
0x4BU, 0x3FU, 0x06U, 0xBFU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP11_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step12_tx[] = {
0x21U, 0x6DU, 0xC6U, 0x3AU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP12_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step13_tx[] = {
0xF8U, 0x2BU, 0xBAU, 0x48U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP13_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step14_tx[] = {
0x60U, 0x08U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP14_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step15_tx[] = {
0x91U, 0xC8U, 0x59U, 0x9EU, 0x0EU, 0x56U, 0xFDU, 0xDFU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP15_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step16_tx[] = {
0xDFU, 0x7DU, 0x4BU, 0xE2U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP16_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step17_tx[] = {
0xD7U, 0x3CU, 0xC7U, 0xFEU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP17_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step18_tx[] = {
0xF2U, 0x16U, 0x75U, 0x57U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP18_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step19_tx[] = {
0xA6U, 0x7AU, 0x8DU, 0xBCU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP19_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step20_tx[] = {
0x60U, 0x0CU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP20_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step21_tx[] = {
0x59U, 0xDAU, 0xDFU, 0xFEU, 0x61U, 0x21U, 0xE7U, 0x37U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP21_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step22_tx[] = {
0x83U, 0x14U, 0x57U, 0x88U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP22_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step23_tx[] = {
0x80U, 0xB7U, 0x94U, 0x8FU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP23_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step24_tx[] = {
0xAEU, 0xF5U, 0xE4U, 0x48U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP24_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step25_tx[] = {
0x2EU, 0x21U, 0x98U, 0xDEU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP25_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step26_tx[] = {
0x60U, 0x10U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP26_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step27_tx[] = {
0xB3U, 0x88U, 0xF7U, 0xE4U, 0x11U, 0x64U, 0xB4U, 0x7AU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP27_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step28_tx[] = {
0x57U, 0xA2U, 0x8EU, 0x48U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP28_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step29_tx[] = {
0x1BU, 0x53U, 0x8AU, 0xABU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP29_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step30_tx[] = {
0x1FU, 0x93U, 0xD5U, 0x9EU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP30_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step31_tx[] = {
0x42U, 0x70U, 0xF1U, 0x51U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP31_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step32_tx[] = {
0x60U, 0x14U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP32_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step33_tx[] = {
0x31U, 0xC3U, 0xFFU, 0x30U, 0x2CU, 0xEEU, 0x25U, 0x9FU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP33_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step34_tx[] = {
0x67U, 0x75U, 0x77U, 0x25U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP34_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step35_tx[] = {
0x0EU, 0xCDU, 0x38U, 0x41U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP35_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step36_tx[] = {
0xCEU, 0xBCU, 0x69U, 0x1BU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP36_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step37_tx[] = {
0x2EU, 0x9FU, 0xB7U, 0x0AU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP37_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step38_tx[] = {
0x60U, 0x18U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP38_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step39_tx[] = {
0x33U, 0x93U, 0x39U, 0x3DU, 0x79U, 0x5BU, 0x67U, 0x78U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP39_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step40_tx[] = {
0x75U, 0xB2U, 0x15U, 0xC1U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP40_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step41_tx[] = {
0x57U, 0xDDU, 0xC9U, 0x40U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP41_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step42_tx[] = {
0x81U, 0x9CU, 0x97U, 0xB7U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP42_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step43_tx[] = {
0x91U, 0x98U, 0x64U, 0x93U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP43_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step44_tx[] = {
0x60U, 0x1CU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP44_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step45_tx[] = {
0xB3U, 0xBDU, 0xCDU, 0xA2U, 0xB2U, 0x37U, 0x94U, 0xDCU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP45_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step46_tx[] = {
0x10U, 0xC8U, 0x1BU, 0x81U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP46_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step47_tx[] = {
0x4EU, 0x7FU, 0xAAU, 0x84U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP47_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step48_tx[] = {
0xF5U, 0xDBU, 0xC0U, 0x86U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP48_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step49_tx[] = {
0x3FU, 0xC7U, 0x8CU, 0xD7U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP49_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step50_tx[] = {
0x60U, 0x20U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP50_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step51_tx[] = {
0x81U, 0xFAU, 0xBEU, 0xB0U, 0xABU, 0xE6U, 0x42U, 0xF1U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP51_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step52_tx[] = {
0xCDU, 0xE0U, 0x32U, 0xD4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP52_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step53_tx[] = {
0xD4U, 0x8CU, 0x2BU, 0x88U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP53_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step54_tx[] = {
0x84U, 0x10U, 0x92U, 0xD5U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP54_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step55_tx[] = {
0xECU, 0xF2U, 0x58U, 0x4FU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP55_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step56_tx[] = {
0x60U, 0x24U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP56_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step57_tx[] = {
0x81U, 0x8CU, 0x48U, 0xB2U, 0x7CU, 0xD8U, 0x54U, 0x49U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP57_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step58_tx[] = {
0x29U, 0x44U, 0xA7U, 0x55U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP58_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step59_tx[] = {
0x1BU, 0xAAU, 0x3FU, 0xA4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP59_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step60_tx[] = {
0x70U, 0x37U, 0xC4U, 0x81U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP60_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step61_tx[] = {
0x34U, 0x39U, 0xABU, 0x16U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP61_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step62_tx[] = {
0x60U, 0x28U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP62_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step63_tx[] = {
0x91U, 0x8EU, 0xEFU, 0xD2U, 0x00U, 0x89U, 0xB6U, 0xF4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP63_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step64_tx[] = {
0x2FU, 0xA6U, 0x06U, 0xE4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP64_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step65_tx[] = {
0x0CU, 0x41U, 0xF6U, 0x80U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP65_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step66_tx[] = {
0xE1U, 0x9EU, 0x30U, 0xBFU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP66_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step67_tx[] = {
0x11U, 0xD5U, 0x98U, 0xFDU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP67_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step68_tx[] = {
0x60U, 0x2CU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP68_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step69_tx[] = {
0x19U, 0x56U, 0x12U, 0x29U, 0x3DU, 0x21U, 0xDDU, 0xD8U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP69_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step70_tx[] = {
0xE5U, 0xD5U, 0x2FU, 0xDBU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP70_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step71_tx[] = {
0x65U, 0x63U, 0x76U, 0xC4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP71_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step72_tx[] = {
0x60U, 0x95U, 0x81U, 0x4EU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP72_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step73_tx[] = {
0x11U, 0x76U, 0xB5U, 0xB4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP73_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step74_tx[] = {
0x60U, 0x30U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP74_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step75_tx[] = {
0xA3U, 0x19U, 0xA5U, 0x05U, 0xCFU, 0xE6U, 0x4BU, 0x58U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP75_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step76_tx[] = {
0xBCU, 0x77U, 0x32U, 0x05U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP76_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step77_tx[] = {
0x3AU, 0x23U, 0x86U, 0x9DU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP77_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step78_tx[] = {
0xE7U, 0x87U, 0xD8U, 0xBBU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP78_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step79_tx[] = {
0x53U, 0x45U, 0x3BU, 0x9CU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP79_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step80_tx[] = {
0x60U, 0x34U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP80_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step81_tx[] = {
0x23U, 0x97U, 0xBFU, 0x1CU, 0x30U, 0xCCU, 0x68U, 0x29U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP81_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step82_tx[] = {
0xCEU, 0xC3U, 0x2DU, 0x53U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP82_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step83_tx[] = {
0xCEU, 0x3EU, 0x6DU, 0xBBU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP83_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step84_tx[] = {
0xDCU, 0x82U, 0x12U, 0x93U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP84_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step85_tx[] = {
0x78U, 0x2DU, 0x37U, 0xFCU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP85_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step86_tx[] = {
0x60U, 0x38U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP86_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step87_tx[] = {
0x31U, 0x46U, 0xE3U, 0x15U, 0x4EU, 0x59U, 0x1FU, 0x48U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP87_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step88_tx[] = {
0x51U, 0x1EU, 0x51U, 0xB4U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP88_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step89_tx[] = {
0x33U, 0xF6U, 0xABU, 0xEEU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP89_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step90_tx[] = {
0x2CU, 0xABU, 0x96U, 0xABU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP90_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step91_tx[] = {
0x05U, 0x4DU, 0x01U, 0x9EU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP91_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step92_tx[] = {
0x60U, 0x3CU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP92_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_4b_step93_tx[] = {
0xB3U, 0xC9U, 0x76U, 0xA9U, 0x97U, 0x13U, 0x3AU, 0x12U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP93_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_4b_step94_tx[] = {
0x7AU, 0x05U, 0x41U, 0xF3U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP94_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step95_tx[] = {
0xF9U, 0x15U, 0xF9U, 0xABU,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP95_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step96_tx[] = {
0xD2U, 0x89U, 0xEDU, 0xE1U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP96_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_4b_step97_tx[] = {
0xF3U, 0x49U, 0x46U, 0xC8U,
};

#define CLASSIC_MFCLASSIC_1K_4B_STEP97_TX_LEN 4U
static const uint8_t *const classic_MfClassic_1K_4b_tx_steps[CLASSIC_MFCLASSIC_1K_4B_TX_STEP_COUNT] = {
	classic_MfClassic_1K_4b_step0_tx,
	classic_MfClassic_1K_4b_step1_tx,
	classic_MfClassic_1K_4b_step2_tx,
	classic_MfClassic_1K_4b_step3_tx,
	classic_MfClassic_1K_4b_step4_tx,
	classic_MfClassic_1K_4b_step5_tx,
	classic_MfClassic_1K_4b_step6_tx,
	classic_MfClassic_1K_4b_step7_tx,
	classic_MfClassic_1K_4b_step8_tx,
	classic_MfClassic_1K_4b_step9_tx,
	classic_MfClassic_1K_4b_step10_tx,
	classic_MfClassic_1K_4b_step11_tx,
	classic_MfClassic_1K_4b_step12_tx,
	classic_MfClassic_1K_4b_step13_tx,
	classic_MfClassic_1K_4b_step14_tx,
	classic_MfClassic_1K_4b_step15_tx,
	classic_MfClassic_1K_4b_step16_tx,
	classic_MfClassic_1K_4b_step17_tx,
	classic_MfClassic_1K_4b_step18_tx,
	classic_MfClassic_1K_4b_step19_tx,
	classic_MfClassic_1K_4b_step20_tx,
	classic_MfClassic_1K_4b_step21_tx,
	classic_MfClassic_1K_4b_step22_tx,
	classic_MfClassic_1K_4b_step23_tx,
	classic_MfClassic_1K_4b_step24_tx,
	classic_MfClassic_1K_4b_step25_tx,
	classic_MfClassic_1K_4b_step26_tx,
	classic_MfClassic_1K_4b_step27_tx,
	classic_MfClassic_1K_4b_step28_tx,
	classic_MfClassic_1K_4b_step29_tx,
	classic_MfClassic_1K_4b_step30_tx,
	classic_MfClassic_1K_4b_step31_tx,
	classic_MfClassic_1K_4b_step32_tx,
	classic_MfClassic_1K_4b_step33_tx,
	classic_MfClassic_1K_4b_step34_tx,
	classic_MfClassic_1K_4b_step35_tx,
	classic_MfClassic_1K_4b_step36_tx,
	classic_MfClassic_1K_4b_step37_tx,
	classic_MfClassic_1K_4b_step38_tx,
	classic_MfClassic_1K_4b_step39_tx,
	classic_MfClassic_1K_4b_step40_tx,
	classic_MfClassic_1K_4b_step41_tx,
	classic_MfClassic_1K_4b_step42_tx,
	classic_MfClassic_1K_4b_step43_tx,
	classic_MfClassic_1K_4b_step44_tx,
	classic_MfClassic_1K_4b_step45_tx,
	classic_MfClassic_1K_4b_step46_tx,
	classic_MfClassic_1K_4b_step47_tx,
	classic_MfClassic_1K_4b_step48_tx,
	classic_MfClassic_1K_4b_step49_tx,
	classic_MfClassic_1K_4b_step50_tx,
	classic_MfClassic_1K_4b_step51_tx,
	classic_MfClassic_1K_4b_step52_tx,
	classic_MfClassic_1K_4b_step53_tx,
	classic_MfClassic_1K_4b_step54_tx,
	classic_MfClassic_1K_4b_step55_tx,
	classic_MfClassic_1K_4b_step56_tx,
	classic_MfClassic_1K_4b_step57_tx,
	classic_MfClassic_1K_4b_step58_tx,
	classic_MfClassic_1K_4b_step59_tx,
	classic_MfClassic_1K_4b_step60_tx,
	classic_MfClassic_1K_4b_step61_tx,
	classic_MfClassic_1K_4b_step62_tx,
	classic_MfClassic_1K_4b_step63_tx,
	classic_MfClassic_1K_4b_step64_tx,
	classic_MfClassic_1K_4b_step65_tx,
	classic_MfClassic_1K_4b_step66_tx,
	classic_MfClassic_1K_4b_step67_tx,
	classic_MfClassic_1K_4b_step68_tx,
	classic_MfClassic_1K_4b_step69_tx,
	classic_MfClassic_1K_4b_step70_tx,
	classic_MfClassic_1K_4b_step71_tx,
	classic_MfClassic_1K_4b_step72_tx,
	classic_MfClassic_1K_4b_step73_tx,
	classic_MfClassic_1K_4b_step74_tx,
	classic_MfClassic_1K_4b_step75_tx,
	classic_MfClassic_1K_4b_step76_tx,
	classic_MfClassic_1K_4b_step77_tx,
	classic_MfClassic_1K_4b_step78_tx,
	classic_MfClassic_1K_4b_step79_tx,
	classic_MfClassic_1K_4b_step80_tx,
	classic_MfClassic_1K_4b_step81_tx,
	classic_MfClassic_1K_4b_step82_tx,
	classic_MfClassic_1K_4b_step83_tx,
	classic_MfClassic_1K_4b_step84_tx,
	classic_MfClassic_1K_4b_step85_tx,
	classic_MfClassic_1K_4b_step86_tx,
	classic_MfClassic_1K_4b_step87_tx,
	classic_MfClassic_1K_4b_step88_tx,
	classic_MfClassic_1K_4b_step89_tx,
	classic_MfClassic_1K_4b_step90_tx,
	classic_MfClassic_1K_4b_step91_tx,
	classic_MfClassic_1K_4b_step92_tx,
	classic_MfClassic_1K_4b_step93_tx,
	classic_MfClassic_1K_4b_step94_tx,
	classic_MfClassic_1K_4b_step95_tx,
	classic_MfClassic_1K_4b_step96_tx,
	classic_MfClassic_1K_4b_step97_tx
};

static const size_t classic_MfClassic_1K_4b_tx_lens[CLASSIC_MFCLASSIC_1K_4B_TX_STEP_COUNT] = {
	CLASSIC_MFCLASSIC_1K_4B_STEP0_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP1_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP2_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP3_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP4_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP5_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP6_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP7_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP8_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP9_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP10_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP11_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP12_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP13_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP14_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP15_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP16_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP17_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP18_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP19_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP20_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP21_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP22_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP23_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP24_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP25_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP26_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP27_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP28_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP29_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP30_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP31_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP32_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP33_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP34_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP35_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP36_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP37_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP38_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP39_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP40_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP41_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP42_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP43_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP44_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP45_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP46_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP47_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP48_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP49_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP50_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP51_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP52_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP53_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP54_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP55_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP56_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP57_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP58_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP59_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP60_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP61_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP62_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP63_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP64_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP65_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP66_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP67_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP68_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP69_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP70_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP71_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP72_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP73_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP74_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP75_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP76_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP77_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP78_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP79_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP80_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP81_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP82_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP83_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP84_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP85_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP86_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP87_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP88_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP89_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP90_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP91_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP92_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP93_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP94_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP95_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP96_TX_LEN,
	CLASSIC_MFCLASSIC_1K_4B_STEP97_TX_LEN
};

#endif
