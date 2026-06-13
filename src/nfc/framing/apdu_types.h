/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Parsed C-APDU view and status-word constants (Wave 2 framing).
 */

#ifndef NFC_FRAMING_APDU_TYPES_H_
#define NFC_FRAMING_APDU_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_SW1_OK                    0x90U
#define NFC_SW2_OK                    0x00U
#define NFC_SW_OK                     0x9000U

#define NFC_SW1_WRONG_LENGTH          0x67U
#define NFC_SW2_WRONG_LENGTH          0x00U
#define NFC_SW_WRONG_LENGTH           0x6700U

#define NFC_SW1_CLA_NOT_SUPPORTED     0x6EU
#define NFC_SW2_CLA_NOT_SUPPORTED     0x00U
#define NFC_SW_CLA_NOT_SUPPORTED      0x6E00U

#define NFC_SW1_INS_NOT_SUPPORTED     0x6DU
#define NFC_SW2_INS_NOT_SUPPORTED     0x00U
#define NFC_SW_INS_NOT_SUPPORTED      0x6D00U

#define NFC_SW1_WRONG_P1P2            0x6AU
#define NFC_SW2_WRONG_P1P2            0x86U
#define NFC_SW_WRONG_P1P2             0x6A86U

#define NFC_SW1_FILE_NOT_FOUND        0x6AU
#define NFC_SW2_FILE_NOT_FOUND        0x82U
#define NFC_SW_FILE_NOT_FOUND         0x6A82U

#define NFC_SW1_WRONG_OFFSET          0x6BU
#define NFC_SW2_WRONG_OFFSET          0x00U
#define NFC_SW_WRONG_OFFSET           0x6B00U

#define NFC_SW1_NO_EF                 0x69U
#define NFC_SW2_NO_EF                 0x86U
#define NFC_SW_NO_EF                  0x6986U

#define NFC_SW1_CONDITIONS_NOT_SAT    0x69U
#define NFC_SW2_CONDITIONS_NOT_SAT    0x85U
#define NFC_SW_CONDITIONS_NOT_SAT       0x6985U

#define NFC_CLA_ISO7816               0x00U

#define NFC_INS_SELECT                0xA4U
#define NFC_INS_READ_BINARY           0xB0U
#define NFC_INS_UPDATE_BINARY         0xD6U

#define NFC_SELECT_P1_BY_FILE_ID      0x00U
#define NFC_SELECT_P1_BY_AID          0x04U

#define NFC_AID_MAX_LEN               16U

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

#endif /* NFC_FRAMING_APDU_TYPES_H_ */
