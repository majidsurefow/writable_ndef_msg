/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/helpers/felica_crc.c
 */

#ifndef NFC_HELPERS_FELICA_CRC_H_
#define NFC_HELPERS_FELICA_CRC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FELICA_CRC_SIZE sizeof(uint16_t)

uint16_t felica_crc_calculate(const uint8_t *data, size_t length);

void felica_crc_append(uint8_t *buf, size_t *len);

bool felica_crc_check(const uint8_t *data, size_t len);

void felica_crc_trim(uint8_t *buf, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_HELPERS_FELICA_CRC_H_ */
