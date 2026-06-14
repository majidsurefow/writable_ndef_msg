/* Auto-generated from Flipper MfClassic_1K_7b.nfc — do not edit. */
#ifndef CLASSIC_FIXTURE_MFCLASSIC_1K_7B_H_
#define CLASSIC_FIXTURE_MFCLASSIC_1K_7B_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

static const uint8_t classic_MfClassic_1K_7b_model[] = {
0x01U, 0x01U, 0x07U, 0x04U, 0xDEU, 0xADU, 0xBEU, 0xEFU,
0xCAU, 0xFEU, 0x08U, 0x44U, 0x00U, 0xFFU, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xFFU, 0xFFU, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x88U, 0x04U, 0xDEU,
0xADU, 0xBEU, 0xC9U, 0x08U, 0x44U, 0x00U, 0xFFU, 0xFFU,
0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xEFU, 0xCAU, 0xFEU,
0xDBU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
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

#define CLASSIC_MFCLASSIC_1K_7B_MODEL_LEN sizeof(classic_MfClassic_1K_7b_model)

static const uint8_t classic_MfClassic_1K_7b_step0_rx[] = {
};

static const uint8_t classic_MfClassic_1K_7b_step1_rx[] = {
0x20U, 0x00U, 0x00U, 0x3EU,
};

static const uint8_t classic_MfClassic_1K_7b_step2_rx[] = {
0x10U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t classic_MfClassic_1K_7b_step3_rx[] = {
0x62U, 0x6CU, 0x35U, 0x2CU,
};

static const uint8_t classic_MfClassic_1K_7b_step4_rx[] = {
0x8FU, 0xB0U, 0x56U, 0x84U, 0xECU, 0xD0U, 0x69U, 0x3EU,
0x38U, 0x19U, 0x40U, 0x43U, 0xC7U, 0xC8U, 0x8DU, 0xA9U,
0x9FU, 0x0EU,
};

static const uint8_t classic_MfClassic_1K_7b_step5_rx[] = {
0xCBU, 0x58U, 0x71U, 0x5EU, 0x6BU, 0xABU, 0x97U, 0xE9U,
0x2FU, 0xBEU, 0x97U, 0xD3U, 0x77U, 0xC4U, 0x66U, 0x01U,
0x33U, 0xBAU,
};

static const uint8_t classic_MfClassic_1K_7b_step6_rx[] = {
0xC0U, 0xF4U, 0x84U, 0x9EU, 0x35U, 0xD6U, 0x0CU, 0x7CU,
0xF1U, 0x65U, 0x60U, 0x87U, 0xD7U, 0x3CU, 0x1FU, 0x0EU,
0x3AU, 0xF3U,
};

static const uint8_t classic_MfClassic_1K_7b_step7_rx[] = {
0x9FU, 0x55U, 0x82U, 0xBBU, 0x04U, 0xCEU, 0xA1U, 0xA3U,
0xA8U, 0x72U, 0xA1U, 0xA1U, 0x05U, 0x4DU, 0x26U, 0xABU,
0x43U, 0x6DU,
};

static const uint8_t classic_MfClassic_1K_7b_step8_rx[] = {
0x10U, 0x00U, 0x00U, 0x01U,
};

static const uint8_t classic_MfClassic_1K_7b_step9_rx[] = {
0xD8U, 0x9FU, 0xA2U, 0x99U,
};

static const uint8_t classic_MfClassic_1K_7b_step10_rx[] = {
0xDEU, 0xC0U, 0x4BU, 0xC6U, 0x1DU, 0xFEU, 0x61U, 0x47U,
0xEEU, 0x78U, 0xA6U, 0x20U, 0x4CU, 0x8EU, 0x0EU, 0xC9U,
0x4EU, 0xEFU,
};

static const uint8_t classic_MfClassic_1K_7b_step11_rx[] = {
0xEDU, 0xD6U, 0xD7U, 0x56U, 0x17U, 0x6EU, 0x14U, 0x72U,
0x6BU, 0x07U, 0xAAU, 0xA1U, 0x20U, 0x1CU, 0x89U, 0xB3U,
0x5CU, 0x64U,
};

static const uint8_t classic_MfClassic_1K_7b_step12_rx[] = {
0x5AU, 0x0CU, 0x1AU, 0x5FU, 0x0EU, 0x67U, 0xE5U, 0xBEU,
0x78U, 0x52U, 0x3CU, 0x7BU, 0x06U, 0xF0U, 0xE8U, 0x04U,
0x86U, 0xB4U,
};

static const uint8_t classic_MfClassic_1K_7b_step13_rx[] = {
0x06U, 0x5DU, 0xBAU, 0xB6U, 0xC3U, 0x14U, 0xA9U, 0xE5U,
0xB6U, 0x5DU, 0x73U, 0xC8U, 0x11U, 0xACU, 0x5DU, 0x93U,
0xE5U, 0xD2U,
};

static const uint8_t classic_MfClassic_1K_7b_step14_rx[] = {
0x10U, 0x00U, 0x00U, 0x02U,
};

static const uint8_t classic_MfClassic_1K_7b_step15_rx[] = {
0xE5U, 0x3EU, 0x38U, 0x47U,
};

static const uint8_t classic_MfClassic_1K_7b_step16_rx[] = {
0x7AU, 0x8BU, 0x83U, 0x9AU, 0x2FU, 0x45U, 0x83U, 0x3FU,
0x7FU, 0x82U, 0x89U, 0xFCU, 0xD6U, 0x7EU, 0x9DU, 0xDEU,
0x69U, 0x59U,
};

static const uint8_t classic_MfClassic_1K_7b_step17_rx[] = {
0x6DU, 0xBBU, 0x00U, 0x35U, 0x83U, 0x0FU, 0x1AU, 0x86U,
0xF3U, 0x2DU, 0x57U, 0x59U, 0x1AU, 0x1FU, 0x2DU, 0x92U,
0x3BU, 0x86U,
};

static const uint8_t classic_MfClassic_1K_7b_step18_rx[] = {
0x39U, 0x95U, 0xC7U, 0xFAU, 0x06U, 0xA4U, 0xF0U, 0xB2U,
0x99U, 0xEDU, 0x55U, 0xF8U, 0xB4U, 0x23U, 0xC8U, 0x9DU,
0x8FU, 0xE3U,
};

static const uint8_t classic_MfClassic_1K_7b_step19_rx[] = {
0xE1U, 0x6BU, 0x62U, 0xBDU, 0x2DU, 0xC5U, 0xBFU, 0x07U,
0x46U, 0x42U, 0x45U, 0x14U, 0xB0U, 0xDCU, 0xA1U, 0xACU,
0xD4U, 0xA8U,
};

static const uint8_t classic_MfClassic_1K_7b_step20_rx[] = {
0x10U, 0x00U, 0x00U, 0x03U,
};

static const uint8_t classic_MfClassic_1K_7b_step21_rx[] = {
0xD8U, 0x2FU, 0xBEU, 0x04U,
};

static const uint8_t classic_MfClassic_1K_7b_step22_rx[] = {
0x08U, 0xF7U, 0x18U, 0x28U, 0x3DU, 0xC0U, 0x03U, 0xE9U,
0x9FU, 0x6EU, 0xD1U, 0xD9U, 0x48U, 0xE9U, 0x9CU, 0x0FU,
0xE7U, 0xC3U,
};

static const uint8_t classic_MfClassic_1K_7b_step23_rx[] = {
0x4CU, 0xCEU, 0x93U, 0xA3U, 0x4BU, 0xEEU, 0x32U, 0xB0U,
0x36U, 0xBEU, 0x21U, 0x3EU, 0xBEU, 0x01U, 0x5FU, 0xBBU,
0xA3U, 0x74U,
};

static const uint8_t classic_MfClassic_1K_7b_step24_rx[] = {
0x8DU, 0x8EU, 0xB4U, 0xECU, 0x10U, 0x21U, 0x73U, 0xD0U,
0x23U, 0x84U, 0x6EU, 0x59U, 0xA4U, 0xEEU, 0xBBU, 0xDCU,
0x99U, 0x9CU,
};

static const uint8_t classic_MfClassic_1K_7b_step25_rx[] = {
0xD6U, 0xC5U, 0x3FU, 0xC2U, 0x3CU, 0xAAU, 0x20U, 0x16U,
0xBEU, 0x96U, 0x2CU, 0x99U, 0x09U, 0x16U, 0x5DU, 0x85U,
0xD0U, 0x91U,
};

static const uint8_t classic_MfClassic_1K_7b_step26_rx[] = {
0x10U, 0x00U, 0x00U, 0x04U,
};

static const uint8_t classic_MfClassic_1K_7b_step27_rx[] = {
0x1DU, 0x06U, 0x8AU, 0xDEU,
};

static const uint8_t classic_MfClassic_1K_7b_step28_rx[] = {
0xBFU, 0xD5U, 0xC7U, 0x40U, 0x40U, 0x2DU, 0x4EU, 0x04U,
0x27U, 0xA9U, 0xEBU, 0xC4U, 0x00U, 0x85U, 0x21U, 0x00U,
0xAFU, 0x59U,
};

static const uint8_t classic_MfClassic_1K_7b_step29_rx[] = {
0xB6U, 0xA4U, 0x4DU, 0x14U, 0x36U, 0xD5U, 0x5FU, 0x52U,
0x90U, 0xF6U, 0x6BU, 0xE1U, 0x61U, 0x17U, 0xBDU, 0xD1U,
0xD8U, 0x35U,
};

static const uint8_t classic_MfClassic_1K_7b_step30_rx[] = {
0xC2U, 0x1DU, 0x11U, 0xE2U, 0x3AU, 0xABU, 0x83U, 0x5BU,
0xD6U, 0xA7U, 0xEFU, 0x47U, 0x6BU, 0xE4U, 0x76U, 0x5DU,
0x0AU, 0xABU,
};

static const uint8_t classic_MfClassic_1K_7b_step31_rx[] = {
0x7BU, 0x5BU, 0xC8U, 0x7BU, 0x3CU, 0x02U, 0x66U, 0x8AU,
0x30U, 0x13U, 0x95U, 0xCFU, 0xD4U, 0x61U, 0x47U, 0x45U,
0xEDU, 0x76U,
};

static const uint8_t classic_MfClassic_1K_7b_step32_rx[] = {
0x10U, 0x00U, 0x00U, 0x05U,
};

static const uint8_t classic_MfClassic_1K_7b_step33_rx[] = {
0x9EU, 0x15U, 0xB6U, 0x02U,
};

static const uint8_t classic_MfClassic_1K_7b_step34_rx[] = {
0x53U, 0xF1U, 0xB9U, 0xC8U, 0x73U, 0x09U, 0x14U, 0x29U,
0x8EU, 0x99U, 0xD3U, 0x2DU, 0xB9U, 0x41U, 0x74U, 0x75U,
0x66U, 0x1EU,
};

static const uint8_t classic_MfClassic_1K_7b_step35_rx[] = {
0x38U, 0x00U, 0xEDU, 0xF5U, 0x0BU, 0x07U, 0x35U, 0x9BU,
0xBAU, 0xBCU, 0x50U, 0x96U, 0x18U, 0x2CU, 0xD1U, 0x84U,
0x93U, 0x5BU,
};

static const uint8_t classic_MfClassic_1K_7b_step36_rx[] = {
0x81U, 0xBAU, 0xAFU, 0xB9U, 0x35U, 0xBBU, 0xFEU, 0xCFU,
0x8BU, 0xC2U, 0xF9U, 0x96U, 0xCAU, 0x18U, 0xE2U, 0xDAU,
0x4AU, 0x74U,
};

static const uint8_t classic_MfClassic_1K_7b_step37_rx[] = {
0x9AU, 0xA8U, 0x24U, 0x09U, 0x23U, 0x52U, 0x01U, 0x4BU,
0x9FU, 0x94U, 0xBFU, 0xE0U, 0xA9U, 0x8DU, 0xD4U, 0x11U,
0xA7U, 0x86U,
};

static const uint8_t classic_MfClassic_1K_7b_step38_rx[] = {
0x10U, 0x00U, 0x00U, 0x06U,
};

static const uint8_t classic_MfClassic_1K_7b_step39_rx[] = {
0x6EU, 0x67U, 0xC2U, 0x6AU,
};

static const uint8_t classic_MfClassic_1K_7b_step40_rx[] = {
0x2CU, 0xB1U, 0x9FU, 0xA5U, 0x23U, 0xBCU, 0xBDU, 0x76U,
0x3FU, 0xB4U, 0x8FU, 0xC9U, 0x01U, 0xAFU, 0x47U, 0x41U,
0x7EU, 0x58U,
};

static const uint8_t classic_MfClassic_1K_7b_step41_rx[] = {
0x4CU, 0x17U, 0x1AU, 0x5EU, 0x57U, 0x7FU, 0x48U, 0x1CU,
0x5DU, 0x6FU, 0x05U, 0x99U, 0x61U, 0x5EU, 0x85U, 0xB2U,
0x78U, 0xE6U,
};

static const uint8_t classic_MfClassic_1K_7b_step42_rx[] = {
0xCFU, 0xECU, 0x6FU, 0x5DU, 0x39U, 0x29U, 0x76U, 0x33U,
0x2FU, 0x53U, 0xF0U, 0xFCU, 0xD7U, 0xA2U, 0xA1U, 0x5FU,
0x4FU, 0xC4U,
};

static const uint8_t classic_MfClassic_1K_7b_step43_rx[] = {
0xA6U, 0xFEU, 0x01U, 0xE1U, 0xFDU, 0xD8U, 0xF1U, 0x79U,
0x0EU, 0x69U, 0xCFU, 0x01U, 0x95U, 0xC0U, 0xBDU, 0x59U,
0xCDU, 0x39U,
};

static const uint8_t classic_MfClassic_1K_7b_step44_rx[] = {
0x10U, 0x00U, 0x00U, 0x07U,
};

static const uint8_t classic_MfClassic_1K_7b_step45_rx[] = {
0xF0U, 0xB0U, 0x81U, 0xCBU,
};

static const uint8_t classic_MfClassic_1K_7b_step46_rx[] = {
0xF1U, 0x79U, 0x5DU, 0x37U, 0x04U, 0x16U, 0x6EU, 0xE5U,
0x47U, 0x09U, 0xECU, 0x8FU, 0x9BU, 0x0BU, 0xFDU, 0xD8U,
0xDCU, 0xB1U,
};

static const uint8_t classic_MfClassic_1K_7b_step47_rx[] = {
0xACU, 0x5DU, 0x56U, 0x80U, 0xF8U, 0x6AU, 0xF3U, 0xD7U,
0x15U, 0x76U, 0xFEU, 0xD4U, 0xD6U, 0xEEU, 0x79U, 0xFAU,
0x0BU, 0x54U,
};

static const uint8_t classic_MfClassic_1K_7b_step48_rx[] = {
0xDFU, 0xBDU, 0xF3U, 0x0BU, 0xD0U, 0x45U, 0x8BU, 0x9CU,
0x87U, 0xA2U, 0x90U, 0xE2U, 0xC8U, 0x11U, 0x77U, 0xD8U,
0x6EU, 0x1DU,
};

static const uint8_t classic_MfClassic_1K_7b_step49_rx[] = {
0x57U, 0xB8U, 0x84U, 0x95U, 0x92U, 0x21U, 0xA0U, 0x38U,
0x8CU, 0x3AU, 0xA3U, 0x0BU, 0x59U, 0xEDU, 0x44U, 0x3BU,
0x85U, 0xBDU,
};

static const uint8_t classic_MfClassic_1K_7b_step50_rx[] = {
0x10U, 0x00U, 0x00U, 0x08U,
};

static const uint8_t classic_MfClassic_1K_7b_step51_rx[] = {
0x82U, 0x05U, 0x09U, 0x0FU,
};

static const uint8_t classic_MfClassic_1K_7b_step52_rx[] = {
0x5DU, 0x6CU, 0xE7U, 0x0EU, 0xA7U, 0xE2U, 0xE0U, 0x0FU,
0x77U, 0x7DU, 0x19U, 0x4DU, 0xF9U, 0x2FU, 0xD1U, 0x7CU,
0x1DU, 0x29U,
};

static const uint8_t classic_MfClassic_1K_7b_step53_rx[] = {
0x43U, 0xC0U, 0x69U, 0x1AU, 0x11U, 0x47U, 0xBEU, 0x5EU,
0x12U, 0xD0U, 0x5BU, 0x74U, 0x56U, 0xEEU, 0x8FU, 0x55U,
0x0CU, 0x36U,
};

static const uint8_t classic_MfClassic_1K_7b_step54_rx[] = {
0x8BU, 0x62U, 0x57U, 0xCDU, 0xFAU, 0x16U, 0xD3U, 0xFAU,
0xE1U, 0x17U, 0x45U, 0x79U, 0x79U, 0x4BU, 0x19U, 0x6FU,
0x4CU, 0xA3U,
};

static const uint8_t classic_MfClassic_1K_7b_step55_rx[] = {
0xEBU, 0xCFU, 0x32U, 0x71U, 0x19U, 0x30U, 0x28U, 0x15U,
0x27U, 0xFEU, 0xCFU, 0x18U, 0xFAU, 0x4CU, 0x3BU, 0x8AU,
0xF9U, 0xCFU,
};

static const uint8_t classic_MfClassic_1K_7b_step56_rx[] = {
0x10U, 0x00U, 0x00U, 0x09U,
};

static const uint8_t classic_MfClassic_1K_7b_step57_rx[] = {
0xC0U, 0xB1U, 0xBCU, 0x91U,
};

static const uint8_t classic_MfClassic_1K_7b_step58_rx[] = {
0x7EU, 0xBFU, 0x14U, 0x26U, 0xBFU, 0xC5U, 0xB0U, 0x27U,
0x0EU, 0x31U, 0x44U, 0x3DU, 0x29U, 0x6DU, 0xD7U, 0x23U,
0x5AU, 0x9AU,
};

static const uint8_t classic_MfClassic_1K_7b_step59_rx[] = {
0xF2U, 0x0CU, 0x1DU, 0xA0U, 0x7FU, 0x69U, 0x9DU, 0x3BU,
0xFDU, 0x24U, 0x7EU, 0xACU, 0x5AU, 0x53U, 0xAFU, 0xD8U,
0x29U, 0xE7U,
};

static const uint8_t classic_MfClassic_1K_7b_step60_rx[] = {
0xC5U, 0x43U, 0x1BU, 0xF2U, 0x9EU, 0x54U, 0x3AU, 0x51U,
0xB2U, 0x99U, 0xE5U, 0x9EU, 0xBEU, 0x3BU, 0xCCU, 0xECU,
0x4EU, 0x07U,
};

static const uint8_t classic_MfClassic_1K_7b_step61_rx[] = {
0x21U, 0xA0U, 0x17U, 0xB0U, 0x2BU, 0xAEU, 0x9BU, 0x15U,
0xB9U, 0x53U, 0x26U, 0x94U, 0xF1U, 0xB7U, 0x91U, 0xF1U,
0x44U, 0xD9U,
};

static const uint8_t classic_MfClassic_1K_7b_step62_rx[] = {
0x10U, 0x00U, 0x00U, 0x0AU,
};

static const uint8_t classic_MfClassic_1K_7b_step63_rx[] = {
0xA2U, 0x68U, 0x7EU, 0x76U,
};

static const uint8_t classic_MfClassic_1K_7b_step64_rx[] = {
0x8DU, 0xAFU, 0x7AU, 0xD2U, 0x23U, 0xF4U, 0xE4U, 0x77U,
0xD6U, 0x61U, 0x16U, 0xD5U, 0x49U, 0xA5U, 0xDCU, 0x85U,
0x54U, 0xAFU,
};

static const uint8_t classic_MfClassic_1K_7b_step65_rx[] = {
0x88U, 0x0EU, 0xE9U, 0xC1U, 0xAAU, 0x6CU, 0x97U, 0x66U,
0x4CU, 0xACU, 0xB7U, 0x4EU, 0x01U, 0x9AU, 0xECU, 0xA2U,
0xA8U, 0xF1U,
};

static const uint8_t classic_MfClassic_1K_7b_step66_rx[] = {
0x5FU, 0x55U, 0x4EU, 0x48U, 0xF1U, 0xB6U, 0x22U, 0x16U,
0xC0U, 0x4BU, 0x0DU, 0x4AU, 0xF3U, 0x54U, 0x4CU, 0x70U,
0x77U, 0x25U,
};

static const uint8_t classic_MfClassic_1K_7b_step67_rx[] = {
0x58U, 0xABU, 0xD6U, 0x93U, 0xA9U, 0xD7U, 0xB3U, 0x59U,
0x2EU, 0xE4U, 0x9DU, 0xFFU, 0xFCU, 0xF6U, 0x5FU, 0x58U,
0xF0U, 0x38U,
};

static const uint8_t classic_MfClassic_1K_7b_step68_rx[] = {
0x10U, 0x00U, 0x00U, 0x0BU,
};

static const uint8_t classic_MfClassic_1K_7b_step69_rx[] = {
0x5FU, 0x8BU, 0xE1U, 0xEDU,
};

static const uint8_t classic_MfClassic_1K_7b_step70_rx[] = {
0x3EU, 0x30U, 0x38U, 0x0AU, 0x88U, 0xE1U, 0xDFU, 0xAAU,
0xBCU, 0xADU, 0xAAU, 0xD8U, 0xA8U, 0xA5U, 0x85U, 0x0BU,
0xD3U, 0x37U,
};

static const uint8_t classic_MfClassic_1K_7b_step71_rx[] = {
0x8AU, 0xD2U, 0x9FU, 0xC1U, 0x1BU, 0xA9U, 0xCFU, 0x9EU,
0xB0U, 0x30U, 0xC7U, 0x99U, 0x7FU, 0x05U, 0x89U, 0x3FU,
0x7AU, 0x2BU,
};

static const uint8_t classic_MfClassic_1K_7b_step72_rx[] = {
0xAEU, 0xCEU, 0x79U, 0x03U, 0xFDU, 0x1EU, 0xB7U, 0xB0U,
0x0BU, 0x83U, 0x73U, 0xB9U, 0x6CU, 0x7AU, 0xB0U, 0xFDU,
0x55U, 0x1AU,
};

static const uint8_t classic_MfClassic_1K_7b_step73_rx[] = {
0xC9U, 0x40U, 0x0FU, 0x4BU, 0x8CU, 0xC4U, 0x42U, 0xFEU,
0xF5U, 0x76U, 0x6FU, 0xDCU, 0xF1U, 0x8DU, 0xC5U, 0xA4U,
0xFAU, 0xF2U,
};

static const uint8_t classic_MfClassic_1K_7b_step74_rx[] = {
0x10U, 0x00U, 0x00U, 0x0CU,
};

static const uint8_t classic_MfClassic_1K_7b_step75_rx[] = {
0x27U, 0x33U, 0x17U, 0x8EU,
};

static const uint8_t classic_MfClassic_1K_7b_step76_rx[] = {
0xECU, 0x21U, 0x37U, 0xA7U, 0x89U, 0xBEU, 0x00U, 0xDCU,
0x68U, 0xC0U, 0x40U, 0x48U, 0xA8U, 0x4DU, 0xFDU, 0x0FU,
0x8EU, 0x86U,
};

static const uint8_t classic_MfClassic_1K_7b_step77_rx[] = {
0xEAU, 0xC9U, 0x6FU, 0xE3U, 0xC0U, 0x6EU, 0xECU, 0xD1U,
0x2EU, 0x35U, 0xA5U, 0x3FU, 0x69U, 0x2BU, 0x39U, 0x85U,
0xBCU, 0x72U,
};

static const uint8_t classic_MfClassic_1K_7b_step78_rx[] = {
0xC0U, 0xC1U, 0xF1U, 0xD8U, 0x13U, 0xBBU, 0xF6U, 0x53U,
0xB9U, 0x65U, 0x1BU, 0x72U, 0x21U, 0x0CU, 0xAEU, 0x13U,
0x55U, 0x9DU,
};

static const uint8_t classic_MfClassic_1K_7b_step79_rx[] = {
0x3AU, 0xC0U, 0x0BU, 0x84U, 0xDEU, 0x79U, 0x46U, 0x19U,
0xFAU, 0x59U, 0x0FU, 0xA6U, 0x63U, 0x0FU, 0x05U, 0x01U,
0x83U, 0x84U,
};

static const uint8_t classic_MfClassic_1K_7b_step80_rx[] = {
0x10U, 0x00U, 0x00U, 0x0DU,
};

static const uint8_t classic_MfClassic_1K_7b_step81_rx[] = {
0x9AU, 0xF6U, 0xFFU, 0xCDU,
};

static const uint8_t classic_MfClassic_1K_7b_step82_rx[] = {
0x8AU, 0x05U, 0xCDU, 0xF9U, 0xE1U, 0x84U, 0x36U, 0xF0U,
0x3AU, 0xB6U, 0xFAU, 0xA8U, 0x79U, 0xD9U, 0x19U, 0xB8U,
0xA8U, 0x71U,
};

static const uint8_t classic_MfClassic_1K_7b_step83_rx[] = {
0x73U, 0x7FU, 0x7EU, 0xD3U, 0x7EU, 0xFEU, 0xDAU, 0x23U,
0xE5U, 0x18U, 0x66U, 0x7FU, 0x80U, 0x50U, 0x2AU, 0x85U,
0x3DU, 0xEFU,
};

static const uint8_t classic_MfClassic_1K_7b_step84_rx[] = {
0x4AU, 0x97U, 0x57U, 0x59U, 0x4FU, 0x2FU, 0x4CU, 0xB7U,
0x8FU, 0xC6U, 0xCAU, 0x9FU, 0x3DU, 0xCAU, 0xA1U, 0x64U,
0xB2U, 0x17U,
};

static const uint8_t classic_MfClassic_1K_7b_step85_rx[] = {
0x2BU, 0x64U, 0x28U, 0xBCU, 0x12U, 0xB5U, 0x96U, 0xE5U,
0x09U, 0x15U, 0x6DU, 0x76U, 0xDCU, 0x10U, 0x6EU, 0xC6U,
0xFAU, 0xC6U,
};

static const uint8_t classic_MfClassic_1K_7b_step86_rx[] = {
0x10U, 0x00U, 0x00U, 0x0EU,
};

static const uint8_t classic_MfClassic_1K_7b_step87_rx[] = {
0x90U, 0x30U, 0xCCU, 0x08U,
};

static const uint8_t classic_MfClassic_1K_7b_step88_rx[] = {
0xD3U, 0xF9U, 0x3FU, 0x03U, 0x72U, 0x0BU, 0xBDU, 0xF9U,
0x06U, 0x09U, 0x25U, 0xA6U, 0x38U, 0x04U, 0x56U, 0x0BU,
0x83U, 0xFAU,
};

static const uint8_t classic_MfClassic_1K_7b_step89_rx[] = {
0x80U, 0x40U, 0x32U, 0x1BU, 0x37U, 0x5EU, 0x76U, 0xC8U,
0x5CU, 0x26U, 0xD0U, 0x59U, 0x2BU, 0x26U, 0xC4U, 0xD1U,
0x3EU, 0xE9U,
};

static const uint8_t classic_MfClassic_1K_7b_step90_rx[] = {
0xF7U, 0xE0U, 0x53U, 0xE4U, 0xF2U, 0x78U, 0x86U, 0xE1U,
0x99U, 0xAAU, 0x9EU, 0x31U, 0xF1U, 0xD5U, 0x22U, 0x10U,
0xC8U, 0xCCU,
};

static const uint8_t classic_MfClassic_1K_7b_step91_rx[] = {
0xA4U, 0x16U, 0x9AU, 0xC5U, 0xD3U, 0xF1U, 0xD1U, 0x01U,
0x51U, 0x67U, 0x5CU, 0xEDU, 0x9AU, 0x6CU, 0x18U, 0xF7U,
0x76U, 0xA0U,
};

static const uint8_t classic_MfClassic_1K_7b_step92_rx[] = {
0x10U, 0x00U, 0x00U, 0x0FU,
};

static const uint8_t classic_MfClassic_1K_7b_step93_rx[] = {
0xADU, 0xB5U, 0xFAU, 0x6BU,
};

static const uint8_t classic_MfClassic_1K_7b_step94_rx[] = {
0xB7U, 0x5EU, 0xF0U, 0x3BU, 0x7DU, 0x6EU, 0xEEU, 0x92U,
0x1FU, 0x79U, 0x6CU, 0xAEU, 0xD5U, 0xEDU, 0x59U, 0x03U,
0xB6U, 0x2DU,
};

static const uint8_t classic_MfClassic_1K_7b_step95_rx[] = {
0x51U, 0x7CU, 0x9CU, 0xC4U, 0x2DU, 0xE0U, 0x59U, 0x82U,
0x2CU, 0x29U, 0x87U, 0x54U, 0xB0U, 0x8EU, 0xF0U, 0x4EU,
0x99U, 0xF0U,
};

static const uint8_t classic_MfClassic_1K_7b_step96_rx[] = {
0x16U, 0xD5U, 0x7DU, 0x98U, 0x71U, 0xFDU, 0x59U, 0xF1U,
0x09U, 0xD9U, 0xFDU, 0x9BU, 0xE2U, 0x70U, 0x9BU, 0x5EU,
0xEEU, 0x3CU,
};

static const uint8_t classic_MfClassic_1K_7b_step97_rx[] = {
0x9DU, 0x49U, 0x02U, 0x0EU, 0xB8U, 0x80U, 0x4EU, 0x19U,
0x39U, 0x50U, 0xD3U, 0x35U, 0x24U, 0x33U, 0xD2U, 0xA4U,
0x82U, 0xC8U,
};

static const nfc_session_mock_step_t classic_MfClassic_1K_7b_read_steps[] = {
	{ .rx = classic_MfClassic_1K_7b_step0_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step0_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step1_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step1_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step2_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step2_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step3_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step3_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step4_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step4_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step5_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step5_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step6_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step6_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step7_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step7_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step8_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step8_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step9_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step9_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step10_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step10_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step11_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step11_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step12_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step12_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step13_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step13_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step14_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step14_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step15_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step15_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step16_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step16_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step17_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step17_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step18_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step18_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step19_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step19_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step20_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step20_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step21_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step21_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step22_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step22_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step23_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step23_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step24_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step24_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step25_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step25_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step26_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step26_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step27_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step27_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step28_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step28_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step29_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step29_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step30_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step30_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step31_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step31_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step32_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step32_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step33_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step33_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step34_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step34_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step35_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step35_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step36_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step36_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step37_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step37_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step38_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step38_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step39_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step39_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step40_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step40_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step41_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step41_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step42_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step42_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step43_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step43_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step44_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step44_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step45_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step45_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step46_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step46_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step47_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step47_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step48_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step48_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step49_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step49_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step50_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step50_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step51_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step51_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step52_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step52_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step53_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step53_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step54_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step54_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step55_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step55_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step56_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step56_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step57_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step57_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step58_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step58_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step59_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step59_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step60_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step60_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step61_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step61_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step62_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step62_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step63_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step63_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step64_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step64_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step65_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step65_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step66_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step66_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step67_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step67_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step68_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step68_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step69_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step69_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step70_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step70_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step71_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step71_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step72_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step72_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step73_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step73_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step74_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step74_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step75_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step75_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step76_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step76_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step77_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step77_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step78_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step78_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step79_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step79_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step80_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step80_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step81_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step81_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step82_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step82_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step83_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step83_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step84_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step84_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step85_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step85_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step86_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step86_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step87_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step87_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step88_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step88_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step89_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step89_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step90_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step90_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step91_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step91_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step92_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step92_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step93_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step93_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step94_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step94_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step95_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step95_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step96_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step96_rx), .err = 0 },
	{ .rx = classic_MfClassic_1K_7b_step97_rx, .rx_len = sizeof(classic_MfClassic_1K_7b_step97_rx), .err = 0 },
};

#define CLASSIC_MFCLASSIC_1K_7B_READ_STEP_COUNT ARRAY_SIZE(classic_MfClassic_1K_7b_read_steps)

#define CLASSIC_MFCLASSIC_1K_7B_TX_STEP_COUNT 98U

static const uint8_t classic_MfClassic_1K_7b_step0_tx[] = {
0x60U, 0xFEU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP0_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step1_tx[] = {
0x60U, 0x3EU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP1_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step2_tx[] = {
0x60U, 0x00U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP2_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step3_tx[] = {
0x70U, 0x40U, 0xE5U, 0x47U, 0x62U, 0x6CU, 0x35U, 0x2CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP3_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step4_tx[] = {
0x09U, 0xF7U, 0x14U, 0x52U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP4_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step5_tx[] = {
0xB6U, 0x19U, 0x08U, 0xB2U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP5_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step6_tx[] = {
0x96U, 0xDBU, 0xCBU, 0x6EU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP6_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step7_tx[] = {
0x26U, 0x62U, 0x3EU, 0xD4U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP7_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step8_tx[] = {
0x60U, 0x04U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP8_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step9_tx[] = {
0x72U, 0x32U, 0x87U, 0xE0U, 0x6FU, 0x5DU, 0xFBU, 0xDDU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP9_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step10_tx[] = {
0x85U, 0x6EU, 0x2BU, 0x4AU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP10_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step11_tx[] = {
0xA9U, 0xE6U, 0xE6U, 0xF2U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP11_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step12_tx[] = {
0x34U, 0xBBU, 0xC0U, 0x9DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP12_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step13_tx[] = {
0x96U, 0x78U, 0xCCU, 0xB8U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP13_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step14_tx[] = {
0x60U, 0x08U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP14_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step15_tx[] = {
0xE4U, 0x43U, 0xE4U, 0x26U, 0x8BU, 0xBBU, 0x8BU, 0xCFU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP15_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step16_tx[] = {
0x85U, 0x81U, 0x85U, 0x33U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP16_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step17_tx[] = {
0x97U, 0x78U, 0x94U, 0x74U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP17_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step18_tx[] = {
0xA2U, 0x30U, 0xF1U, 0xA8U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP18_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step19_tx[] = {
0x30U, 0xABU, 0x55U, 0x1CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP19_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step20_tx[] = {
0x60U, 0x0CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP20_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step21_tx[] = {
0x67U, 0x87U, 0x4CU, 0xA0U, 0x01U, 0x68U, 0x54U, 0xC8U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP21_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step22_tx[] = {
0xB9U, 0x92U, 0xDAU, 0x66U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP22_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step23_tx[] = {
0x5CU, 0x34U, 0xA7U, 0xC6U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP23_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step24_tx[] = {
0x14U, 0x99U, 0xB2U, 0x42U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP24_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step25_tx[] = {
0xC8U, 0x6DU, 0x49U, 0x19U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP25_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step26_tx[] = {
0x60U, 0x10U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP26_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step27_tx[] = {
0x71U, 0x27U, 0x61U, 0x23U, 0xC0U, 0x0CU, 0xEDU, 0xCFU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP27_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step28_tx[] = {
0xDCU, 0xB6U, 0xDFU, 0x77U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP28_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step29_tx[] = {
0x56U, 0xC7U, 0x2FU, 0x3DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP29_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step30_tx[] = {
0x1FU, 0xB1U, 0x31U, 0xD4U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP30_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step31_tx[] = {
0xF1U, 0xF1U, 0x58U, 0x89U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP31_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step32_tx[] = {
0x60U, 0x14U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP32_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step33_tx[] = {
0xF2U, 0x65U, 0x25U, 0xCFU, 0xF4U, 0xDDU, 0x88U, 0x57U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP33_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step34_tx[] = {
0x1CU, 0x1DU, 0xF3U, 0x79U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP34_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step35_tx[] = {
0xB7U, 0x24U, 0xDDU, 0xC9U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP35_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step36_tx[] = {
0x93U, 0xA6U, 0x95U, 0x1DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP36_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step37_tx[] = {
0x8AU, 0x02U, 0x1AU, 0x7DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP37_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step38_tx[] = {
0x60U, 0x18U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP38_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step39_tx[] = {
0xECU, 0xA3U, 0xA4U, 0x94U, 0xDDU, 0xE8U, 0x16U, 0xF3U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP39_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step40_tx[] = {
0x38U, 0xDCU, 0x6AU, 0xA6U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP40_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step41_tx[] = {
0x1FU, 0x75U, 0x9FU, 0xB4U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP41_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step42_tx[] = {
0x32U, 0xB9U, 0xDEU, 0x7CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP42_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step43_tx[] = {
0x04U, 0x00U, 0x92U, 0x29U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP43_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step44_tx[] = {
0x60U, 0x1CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP44_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step45_tx[] = {
0xA7U, 0xE0U, 0x00U, 0xECU, 0xF4U, 0xFDU, 0x0CU, 0x16U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP45_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step46_tx[] = {
0x72U, 0xD1U, 0x9EU, 0x3BU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP46_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step47_tx[] = {
0x17U, 0x56U, 0xEDU, 0x45U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP47_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step48_tx[] = {
0xE6U, 0xA3U, 0x8CU, 0xA7U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP48_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step49_tx[] = {
0xC2U, 0xAAU, 0x44U, 0x70U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP49_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step50_tx[] = {
0x60U, 0x20U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP50_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step51_tx[] = {
0x79U, 0xE2U, 0x76U, 0x73U, 0x38U, 0x10U, 0xC7U, 0x2DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP51_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step52_tx[] = {
0x00U, 0xD5U, 0x1AU, 0x9BU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP52_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step53_tx[] = {
0x97U, 0x22U, 0x59U, 0x12U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP53_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step54_tx[] = {
0x7DU, 0x4DU, 0x54U, 0x5FU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP54_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step55_tx[] = {
0x35U, 0x6DU, 0xF7U, 0xB2U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP55_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step56_tx[] = {
0x60U, 0x24U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP56_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step57_tx[] = {
0x73U, 0x86U, 0x24U, 0x13U, 0xCDU, 0x66U, 0x2BU, 0xF7U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP57_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step58_tx[] = {
0x5FU, 0x8FU, 0x82U, 0x0AU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP58_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step59_tx[] = {
0x08U, 0x8DU, 0x16U, 0xC8U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP59_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step60_tx[] = {
0x1DU, 0x3FU, 0xBDU, 0xE7U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP60_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step61_tx[] = {
0xA9U, 0xD7U, 0x18U, 0x96U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP61_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step62_tx[] = {
0x60U, 0x28U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP62_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step63_tx[] = {
0xADU, 0x86U, 0x55U, 0x47U, 0x76U, 0xF8U, 0x03U, 0xDCU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP63_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step64_tx[] = {
0xEDU, 0x75U, 0x1FU, 0x9DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP64_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step65_tx[] = {
0x14U, 0x41U, 0x77U, 0xDAU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP65_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step66_tx[] = {
0x2CU, 0xE4U, 0x73U, 0xA5U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP66_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step67_tx[] = {
0x8DU, 0x5DU, 0x54U, 0x1CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP67_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step68_tx[] = {
0x60U, 0x2CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP68_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step69_tx[] = {
0x66U, 0xB1U, 0xF1U, 0xEEU, 0x3CU, 0xD9U, 0xC5U, 0x03U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP69_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step70_tx[] = {
0x25U, 0x1CU, 0xC1U, 0xFEU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP70_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step71_tx[] = {
0x4AU, 0xC4U, 0x9FU, 0xEDU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP71_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step72_tx[] = {
0x03U, 0xAAU, 0x56U, 0x9CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP72_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step73_tx[] = {
0xEAU, 0x1BU, 0xE8U, 0x46U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP73_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step74_tx[] = {
0x60U, 0x30U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP74_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step75_tx[] = {
0x71U, 0x21U, 0x27U, 0xA6U, 0x40U, 0x2CU, 0xBEU, 0xBDU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP75_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step76_tx[] = {
0x8DU, 0x9CU, 0x91U, 0x2CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP76_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step77_tx[] = {
0xE7U, 0x09U, 0x11U, 0x93U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP77_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step78_tx[] = {
0xACU, 0xF8U, 0x70U, 0xAAU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP78_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step79_tx[] = {
0xEAU, 0x86U, 0xAEU, 0x7DU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP79_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step80_tx[] = {
0x60U, 0x34U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP80_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step81_tx[] = {
0xF3U, 0xB6U, 0x4DU, 0x3FU, 0x4AU, 0x2BU, 0x0FU, 0xBAU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP81_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step82_tx[] = {
0x6AU, 0x75U, 0xE4U, 0x87U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP82_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step83_tx[] = {
0x80U, 0x37U, 0x83U, 0xD1U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP83_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step84_tx[] = {
0x63U, 0x6DU, 0x07U, 0x0BU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP84_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step85_tx[] = {
0xC4U, 0x93U, 0x0BU, 0x56U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP85_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step86_tx[] = {
0x60U, 0x38U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP86_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step87_tx[] = {
0xE4U, 0x84U, 0x98U, 0x22U, 0x99U, 0xAAU, 0xD6U, 0xB3U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP87_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step88_tx[] = {
0xEDU, 0x63U, 0x6EU, 0x9CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP88_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step89_tx[] = {
0x67U, 0x88U, 0x83U, 0xD2U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP89_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step90_tx[] = {
0xA6U, 0xB8U, 0xA8U, 0x19U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP90_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step91_tx[] = {
0xC4U, 0x9AU, 0xFFU, 0xD3U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP91_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step92_tx[] = {
0x60U, 0x3CU,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP92_TX_LEN 2U
static const uint8_t classic_MfClassic_1K_7b_step93_tx[] = {
0xE6U, 0x07U, 0xF8U, 0xECU, 0x13U, 0xEDU, 0xB9U, 0x94U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP93_TX_LEN 8U
static const uint8_t classic_MfClassic_1K_7b_step94_tx[] = {
0x0FU, 0x60U, 0xE7U, 0xE6U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP94_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step95_tx[] = {
0x4EU, 0x2EU, 0x8BU, 0xD7U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP95_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step96_tx[] = {
0x8CU, 0x10U, 0x43U, 0xE5U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP96_TX_LEN 4U
static const uint8_t classic_MfClassic_1K_7b_step97_tx[] = {
0x25U, 0xCFU, 0xC3U, 0xA4U,
};

#define CLASSIC_MFCLASSIC_1K_7B_STEP97_TX_LEN 4U
static const uint8_t *const classic_MfClassic_1K_7b_tx_steps[CLASSIC_MFCLASSIC_1K_7B_TX_STEP_COUNT] = {
	classic_MfClassic_1K_7b_step0_tx,
	classic_MfClassic_1K_7b_step1_tx,
	classic_MfClassic_1K_7b_step2_tx,
	classic_MfClassic_1K_7b_step3_tx,
	classic_MfClassic_1K_7b_step4_tx,
	classic_MfClassic_1K_7b_step5_tx,
	classic_MfClassic_1K_7b_step6_tx,
	classic_MfClassic_1K_7b_step7_tx,
	classic_MfClassic_1K_7b_step8_tx,
	classic_MfClassic_1K_7b_step9_tx,
	classic_MfClassic_1K_7b_step10_tx,
	classic_MfClassic_1K_7b_step11_tx,
	classic_MfClassic_1K_7b_step12_tx,
	classic_MfClassic_1K_7b_step13_tx,
	classic_MfClassic_1K_7b_step14_tx,
	classic_MfClassic_1K_7b_step15_tx,
	classic_MfClassic_1K_7b_step16_tx,
	classic_MfClassic_1K_7b_step17_tx,
	classic_MfClassic_1K_7b_step18_tx,
	classic_MfClassic_1K_7b_step19_tx,
	classic_MfClassic_1K_7b_step20_tx,
	classic_MfClassic_1K_7b_step21_tx,
	classic_MfClassic_1K_7b_step22_tx,
	classic_MfClassic_1K_7b_step23_tx,
	classic_MfClassic_1K_7b_step24_tx,
	classic_MfClassic_1K_7b_step25_tx,
	classic_MfClassic_1K_7b_step26_tx,
	classic_MfClassic_1K_7b_step27_tx,
	classic_MfClassic_1K_7b_step28_tx,
	classic_MfClassic_1K_7b_step29_tx,
	classic_MfClassic_1K_7b_step30_tx,
	classic_MfClassic_1K_7b_step31_tx,
	classic_MfClassic_1K_7b_step32_tx,
	classic_MfClassic_1K_7b_step33_tx,
	classic_MfClassic_1K_7b_step34_tx,
	classic_MfClassic_1K_7b_step35_tx,
	classic_MfClassic_1K_7b_step36_tx,
	classic_MfClassic_1K_7b_step37_tx,
	classic_MfClassic_1K_7b_step38_tx,
	classic_MfClassic_1K_7b_step39_tx,
	classic_MfClassic_1K_7b_step40_tx,
	classic_MfClassic_1K_7b_step41_tx,
	classic_MfClassic_1K_7b_step42_tx,
	classic_MfClassic_1K_7b_step43_tx,
	classic_MfClassic_1K_7b_step44_tx,
	classic_MfClassic_1K_7b_step45_tx,
	classic_MfClassic_1K_7b_step46_tx,
	classic_MfClassic_1K_7b_step47_tx,
	classic_MfClassic_1K_7b_step48_tx,
	classic_MfClassic_1K_7b_step49_tx,
	classic_MfClassic_1K_7b_step50_tx,
	classic_MfClassic_1K_7b_step51_tx,
	classic_MfClassic_1K_7b_step52_tx,
	classic_MfClassic_1K_7b_step53_tx,
	classic_MfClassic_1K_7b_step54_tx,
	classic_MfClassic_1K_7b_step55_tx,
	classic_MfClassic_1K_7b_step56_tx,
	classic_MfClassic_1K_7b_step57_tx,
	classic_MfClassic_1K_7b_step58_tx,
	classic_MfClassic_1K_7b_step59_tx,
	classic_MfClassic_1K_7b_step60_tx,
	classic_MfClassic_1K_7b_step61_tx,
	classic_MfClassic_1K_7b_step62_tx,
	classic_MfClassic_1K_7b_step63_tx,
	classic_MfClassic_1K_7b_step64_tx,
	classic_MfClassic_1K_7b_step65_tx,
	classic_MfClassic_1K_7b_step66_tx,
	classic_MfClassic_1K_7b_step67_tx,
	classic_MfClassic_1K_7b_step68_tx,
	classic_MfClassic_1K_7b_step69_tx,
	classic_MfClassic_1K_7b_step70_tx,
	classic_MfClassic_1K_7b_step71_tx,
	classic_MfClassic_1K_7b_step72_tx,
	classic_MfClassic_1K_7b_step73_tx,
	classic_MfClassic_1K_7b_step74_tx,
	classic_MfClassic_1K_7b_step75_tx,
	classic_MfClassic_1K_7b_step76_tx,
	classic_MfClassic_1K_7b_step77_tx,
	classic_MfClassic_1K_7b_step78_tx,
	classic_MfClassic_1K_7b_step79_tx,
	classic_MfClassic_1K_7b_step80_tx,
	classic_MfClassic_1K_7b_step81_tx,
	classic_MfClassic_1K_7b_step82_tx,
	classic_MfClassic_1K_7b_step83_tx,
	classic_MfClassic_1K_7b_step84_tx,
	classic_MfClassic_1K_7b_step85_tx,
	classic_MfClassic_1K_7b_step86_tx,
	classic_MfClassic_1K_7b_step87_tx,
	classic_MfClassic_1K_7b_step88_tx,
	classic_MfClassic_1K_7b_step89_tx,
	classic_MfClassic_1K_7b_step90_tx,
	classic_MfClassic_1K_7b_step91_tx,
	classic_MfClassic_1K_7b_step92_tx,
	classic_MfClassic_1K_7b_step93_tx,
	classic_MfClassic_1K_7b_step94_tx,
	classic_MfClassic_1K_7b_step95_tx,
	classic_MfClassic_1K_7b_step96_tx,
	classic_MfClassic_1K_7b_step97_tx
};

static const size_t classic_MfClassic_1K_7b_tx_lens[CLASSIC_MFCLASSIC_1K_7B_TX_STEP_COUNT] = {
	CLASSIC_MFCLASSIC_1K_7B_STEP0_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP1_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP2_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP3_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP4_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP5_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP6_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP7_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP8_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP9_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP10_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP11_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP12_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP13_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP14_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP15_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP16_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP17_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP18_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP19_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP20_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP21_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP22_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP23_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP24_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP25_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP26_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP27_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP28_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP29_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP30_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP31_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP32_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP33_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP34_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP35_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP36_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP37_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP38_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP39_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP40_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP41_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP42_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP43_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP44_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP45_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP46_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP47_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP48_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP49_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP50_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP51_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP52_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP53_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP54_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP55_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP56_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP57_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP58_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP59_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP60_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP61_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP62_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP63_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP64_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP65_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP66_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP67_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP68_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP69_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP70_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP71_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP72_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP73_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP74_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP75_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP76_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP77_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP78_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP79_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP80_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP81_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP82_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP83_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP84_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP85_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP86_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP87_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP88_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP89_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP90_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP91_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP92_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP93_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP94_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP95_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP96_TX_LEN,
	CLASSIC_MFCLASSIC_1K_7B_STEP97_TX_LEN
};

#endif
