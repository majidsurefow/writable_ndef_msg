/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Parsed C-APDU view (Wave 2 / tests). Product copy lands in framing/apdu_types.h.
 */

#ifndef TESTS_COMMON_APDU_TYPES_H_
#define TESTS_COMMON_APDU_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_SW1_OK              0x90U
#define NFC_SW2_OK              0x00U
#define NFC_SW_OK               0x9000U

#define NFC_INS_SELECT          0xA4U
#define NFC_INS_READ_BINARY     0xB0U
#define NFC_INS_UPDATE_BINARY   0xD6U

#define NFC_SELECT_P1_BY_FILE_ID 0x00U

typedef struct {
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	const uint8_t *data;
	uint16_t lc;
	uint16_t le;
	bool has_le;
	bool extended;
} nfc_apdu_t;

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_APDU_TYPES_H_ */
