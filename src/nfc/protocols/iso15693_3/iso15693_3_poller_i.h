/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Private ISO15693-3 poller helpers — CRC and response parsers.
 * Behavioral reference: flipperzero/lib/nfc/protocols/iso15693_3/iso15693_3_i.c
 */

#ifndef NFC_PROTOCOLS_ISO15693_3_POLLER_I_H_
#define NFC_PROTOCOLS_ISO15693_3_POLLER_I_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "protocols/iso15693_3/iso15693_3.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISO15693_CRC_SIZE              2U
#define ISO15693_BLOCKS_PER_QUERY      32U

#define ISO15693_SYSINFO_FLAG_DSFID    0x01U
#define ISO15693_SYSINFO_FLAG_AFI      0x10U
#define ISO15693_SYSINFO_FLAG_MEMORY   0x40U
#define ISO15693_SYSINFO_FLAG_IC_REF   0x80U

#define ISO15693_RESP_FLAG_ERROR       0x01U

uint16_t iso15693_3_crc16(const uint8_t *data, size_t len);
size_t iso15693_3_crc_append(uint8_t *buf, size_t buf_len, size_t payload_len);
bool iso15693_3_crc_check(const uint8_t *buf, size_t len);
size_t iso15693_3_crc_trim(uint8_t *buf, size_t len);

void iso15693_3_append_uid_reversed(const uint8_t uid[ISO15693_UID_SIZE], uint8_t *out,
				    size_t *out_len);

int iso15693_3_inventory_response_parse(const uint8_t *rx, size_t rx_len, uint8_t uid[ISO15693_UID_SIZE],
					uint8_t *dsfid);
int iso15693_3_system_info_response_parse(const uint8_t *rx, size_t rx_len, iso15693_3_data_t *data);
int iso15693_3_read_block_response_parse(const uint8_t *rx, size_t rx_len, uint8_t block_size,
					 uint8_t *out);
int iso15693_3_get_block_security_response_parse(const uint8_t *rx, size_t rx_len,
						 uint8_t *out, uint16_t block_count);

bool iso15693_3_optional_step_ok(int ret);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ISO15693_3_POLLER_I_H_ */
