/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/iso15693_3/iso15693_3_poller_i.h"

#include <errno.h>
#include <string.h>

#define ISO15693_CRC_INIT  0xFFFFU
#define ISO15693_CRC_POLY  0x8408U

uint16_t iso15693_3_crc16(const uint8_t *data, size_t len)
{
	uint16_t crc = ISO15693_CRC_INIT;

	if (data == NULL) {
		return crc;
	}

	for (size_t i = 0U; i < len; i++) {
		crc ^= (uint16_t)data[i];
		for (size_t j = 0U; j < 8U; j++) {
			if ((crc & 1U) != 0U) {
				crc = (uint16_t)((crc >> 1U) ^ ISO15693_CRC_POLY);
			} else {
				crc >>= 1U;
			}
		}
	}

	return (uint16_t)(~crc);
}

size_t iso15693_3_crc_append(uint8_t *buf, size_t buf_len, size_t payload_len)
{
	uint16_t crc;

	if ((buf == NULL) || (payload_len > buf_len) ||
	    ((payload_len + ISO15693_CRC_SIZE) > buf_len)) {
		return 0U;
	}

	crc = iso15693_3_crc16(buf, payload_len);
	buf[payload_len] = (uint8_t)(crc & 0xFFU);
	buf[payload_len + 1U] = (uint8_t)((crc >> 8U) & 0xFFU);

	return payload_len + ISO15693_CRC_SIZE;
}

bool iso15693_3_crc_check(const uint8_t *buf, size_t len)
{
	uint16_t crc_rx;
	uint16_t crc_calc;

	if ((buf == NULL) || (len <= ISO15693_CRC_SIZE)) {
		return false;
	}

	crc_rx = (uint16_t)buf[len - 2U] | ((uint16_t)buf[len - 1U] << 8U);
	crc_calc = iso15693_3_crc16(buf, len - ISO15693_CRC_SIZE);

	return crc_calc == crc_rx;
}

size_t iso15693_3_crc_trim(uint8_t *buf, size_t len)
{
	if ((buf == NULL) || (len <= ISO15693_CRC_SIZE)) {
		return 0U;
	}

	return len - ISO15693_CRC_SIZE;
}

void iso15693_3_append_uid_reversed(const uint8_t uid[ISO15693_UID_SIZE], uint8_t *out,
				    size_t *out_len)
{
	if ((uid == NULL) || (out == NULL) || (out_len == NULL)) {
		return;
	}

	for (size_t i = 0U; i < ISO15693_UID_SIZE; i++) {
		out[i] = uid[ISO15693_UID_SIZE - 1U - i];
	}

	*out_len = ISO15693_UID_SIZE;
}

static bool iso15693_3_error_response_parse(int *err_out, const uint8_t *rx, size_t rx_len)
{
	if ((err_out == NULL) || (rx == NULL) || (rx_len == 0U)) {
		return false;
	}

	if ((rx[0] & ISO15693_RESP_FLAG_ERROR) == 0U) {
		return false;
	}

	if (rx_len < 2U) {
		*err_out = -EIO;
		return true;
	}

	switch (rx[1]) {
	case 0x01U:
	case 0x02U:
		*err_out = -ENOTSUP;
		break;
	default:
		*err_out = -EIO;
		break;
	}

	return true;
}

int iso15693_3_inventory_response_parse(const uint8_t *rx, size_t rx_len, uint8_t uid[ISO15693_UID_SIZE],
					uint8_t *dsfid)
{
	int err = 0;

	if ((rx == NULL) || (uid == NULL) || (dsfid == NULL)) {
		return -EINVAL;
	}

	if (iso15693_3_error_response_parse(&err, rx, rx_len)) {
		return err;
	}

	if (rx_len != (2U + ISO15693_UID_SIZE)) {
		return -EIO;
	}

	*dsfid = rx[1];
	for (size_t i = 0U; i < ISO15693_UID_SIZE; i++) {
		uid[i] = rx[(ISO15693_UID_SIZE + 1U) - i];
	}

	return 0;
}

int iso15693_3_system_info_response_parse(const uint8_t *rx, size_t rx_len, iso15693_3_data_t *data)
{
	size_t offset;
	uint8_t info_flags;
	int err = 0;

	if ((rx == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	if (iso15693_3_error_response_parse(&err, rx, rx_len)) {
		return err;
	}

	if (rx_len < 2U) {
		return -EIO;
	}

	info_flags = rx[1];
	offset = 2U;

	if ((info_flags & ISO15693_SYSINFO_FLAG_DSFID) != 0U) {
		if (offset >= rx_len) {
			return -EIO;
		}
		data->dsfid = rx[offset++];
	}

	if ((info_flags & ISO15693_SYSINFO_FLAG_AFI) != 0U) {
		if (offset >= rx_len) {
			return -EIO;
		}
		data->afi = rx[offset++];
	}

	if ((info_flags & ISO15693_SYSINFO_FLAG_MEMORY) != 0U) {
		if ((offset + 1U) >= rx_len) {
			return -EIO;
		}
		data->block_count = (uint16_t)rx[offset++] + 1U;
		data->block_size = (uint8_t)((rx[offset++] & 0x1FU) + 1U);
	}

	if ((info_flags & ISO15693_SYSINFO_FLAG_IC_REF) != 0U) {
		if (offset >= rx_len) {
			return -EIO;
		}
		data->ic_ref = rx[offset++];
	}

	if (offset != rx_len) {
		return -EIO;
	}

	if ((data->block_count == 0U) || (data->block_count > ISO15693_BLOCKS_MAX) ||
	    (data->block_size == 0U) || (data->block_size > ISO15693_BLOCK_SIZE_MAX)) {
		return -EIO;
	}

	return 0;
}

int iso15693_3_read_block_response_parse(const uint8_t *rx, size_t rx_len, uint8_t block_size,
					 uint8_t *out)
{
	size_t payload_len;
	int err = 0;

	if ((rx == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	if (iso15693_3_error_response_parse(&err, rx, rx_len)) {
		return err;
	}

	if (rx_len <= 1U) {
		return -EIO;
	}

	payload_len = rx_len - 1U;
	if (payload_len != block_size) {
		return -EIO;
	}

	(void)memcpy(out, &rx[1], block_size);
	return 0;
}

int iso15693_3_get_block_security_response_parse(const uint8_t *rx, size_t rx_len, uint8_t *out,
						 uint16_t block_count)
{
	size_t payload_len;
	int err = 0;

	if ((rx == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	if (iso15693_3_error_response_parse(&err, rx, rx_len)) {
		return err;
	}

	if (rx_len <= 1U) {
		return -EIO;
	}

	payload_len = rx_len - 1U;
	if (payload_len != block_count) {
		return -EIO;
	}

	(void)memcpy(out, &rx[1], block_count);
	return 0;
}

bool iso15693_3_optional_step_ok(int ret)
{
	return (ret == 0) || (ret == -ENOTSUP) || (ret == -ETIMEDOUT);
}
