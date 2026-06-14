/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Behavioral reference: flipperzero/lib/nfc/helpers/felica_crc.c
 */

#include "helpers/felica_crc.h"

#include <string.h>

#define FELICA_CRC_POLY 0x1021U
#define FELICA_CRC_INIT 0x0000U

uint16_t felica_crc_calculate(const uint8_t *data, size_t length)
{
	uint16_t crc = FELICA_CRC_INIT;

	if (data == NULL) {
		return 0U;
	}

	for (size_t i = 0U; i < length; i++) {
		crc ^= (uint16_t)((uint16_t)data[i] << 8U);
		for (size_t j = 0U; j < 8U; j++) {
			if ((crc & 0x8000U) != 0U) {
				crc = (uint16_t)((crc << 1U) ^ FELICA_CRC_POLY);
			} else {
				crc <<= 1U;
			}
		}
	}

	return (uint16_t)((crc << 8U) | (crc >> 8U));
}

void felica_crc_append(uint8_t *buf, size_t *len)
{
	uint16_t crc;

	if ((buf == NULL) || (len == NULL)) {
		return;
	}

	crc = felica_crc_calculate(buf, *len);
	buf[*len] = (uint8_t)(crc & 0xFFU);
	buf[*len + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);
	*len += FELICA_CRC_SIZE;
}

bool felica_crc_check(const uint8_t *data, size_t len)
{
	uint16_t crc_rx;
	uint16_t crc_calc;

	if ((data == NULL) || (len <= FELICA_CRC_SIZE)) {
		return false;
	}

	crc_rx = (uint16_t)data[len - 2U] | ((uint16_t)data[len - 1U] << 8U);
	crc_calc = felica_crc_calculate(data, len - FELICA_CRC_SIZE);

	return crc_calc == crc_rx;
}

void felica_crc_trim(uint8_t *buf, size_t *len)
{
	if ((buf == NULL) || (len == NULL) || (*len < FELICA_CRC_SIZE)) {
		return;
	}

	*len -= FELICA_CRC_SIZE;
	(void)memset(&buf[*len], 0, FELICA_CRC_SIZE);
}
