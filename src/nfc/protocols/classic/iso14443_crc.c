/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/classic/iso14443_crc.h"

#include <string.h>

#define ISO14443_3A_CRC_INIT 0x6363U

uint16_t iso14443_crc_a(const uint8_t *data, size_t len)
{
	uint16_t crc = ISO14443_3A_CRC_INIT;

	for (size_t i = 0U; i < len; i++) {
		uint8_t byte = data[i];

		byte ^= (uint8_t)(crc & 0xFFU);
		byte ^= (uint8_t)(byte << 4);
		crc = (uint16_t)((crc >> 8) ^ (((uint16_t)byte) << 8) ^
				 (((uint16_t)byte) << 3) ^ (byte >> 4));
	}

	return crc;
}

void iso14443_crc_append_a(uint8_t *buf, size_t *len)
{
	uint16_t crc = iso14443_crc_a(buf, *len);

	buf[*len] = (uint8_t)(crc & 0xFFU);
	buf[*len + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);
	*len += ISO14443_CRC_SIZE;
}

bool iso14443_crc_check_a(const uint8_t *data, size_t len)
{
	uint16_t crc_rx;
	uint16_t crc_calc;

	if (len <= ISO14443_CRC_SIZE) {
		return false;
	}

	crc_rx = (uint16_t)data[len - 2U] | ((uint16_t)data[len - 1U] << 8U);
	crc_calc = iso14443_crc_a(data, len - ISO14443_CRC_SIZE);

	return crc_calc == crc_rx;
}

void iso14443_crc_trim_a(uint8_t *buf, size_t *len)
{
	if (*len >= ISO14443_CRC_SIZE) {
		*len -= ISO14443_CRC_SIZE;
		(void)memset(&buf[*len], 0, ISO14443_CRC_SIZE);
	}
}
