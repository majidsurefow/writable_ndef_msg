/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * ISO14443-A CRC — port from flipperzero/lib/nfc/helpers/iso14443_crc.c
 */

#ifndef NFC_PROTOCOLS_CLASSIC_ISO14443_CRC_H_
#define NFC_PROTOCOLS_CLASSIC_ISO14443_CRC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ISO14443_CRC_SIZE 2U

uint16_t iso14443_crc_a(const uint8_t *data, size_t len);
void iso14443_crc_append_a(uint8_t *buf, size_t *len);
bool iso14443_crc_check_a(const uint8_t *data, size_t len);
void iso14443_crc_trim_a(uint8_t *buf, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_ISO14443_CRC_H_ */
