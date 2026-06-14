/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocols/classic/classic_i.h"

#include <string.h>

#include <zephyr/sys/util.h>

uint8_t classic_get_total_sectors(classic_type_t type)
{
	switch (type) {
	case CLASSIC_TYPE_MINI:
		return 5U;
	case CLASSIC_TYPE_1K:
		return 16U;
	case CLASSIC_TYPE_4K:
		return 40U;
	default:
		return 0U;
	}
}

uint16_t classic_get_total_blocks(classic_type_t type)
{
	switch (type) {
	case CLASSIC_TYPE_MINI:
		return 20U;
	case CLASSIC_TYPE_1K:
		return 64U;
	case CLASSIC_TYPE_4K:
		return 256U;
	default:
		return 0U;
	}
}

uint8_t classic_get_sector_by_block(uint8_t block)
{
	if (block < 128U) {
		return (uint8_t)((block | 0x03U) / 4U);
	}

	return (uint8_t)(32U + (((block | 0x0FU) - 32U * 4U) / 16U));
}

bool classic_is_sector_trailer(uint8_t block)
{
	if (block < 128U) {
		return (block & 0x03U) == 0x03U;
	}

	return (block & 0x0FU) == 0x0FU;
}

uint8_t classic_get_first_block_of_sector(uint8_t sector)
{
	if (sector < 32U) {
		return (uint8_t)(sector * 4U);
	}

	return (uint8_t)(32U * 4U + (sector - 32U) * 16U);
}

uint8_t classic_get_blocks_in_sector(uint8_t sector)
{
	return (sector < 32U) ? 4U : 16U;
}

bool classic_is_block_read(const classic_data_t *data, uint8_t block)
{
	if ((data == NULL) || (block >= CLASSIC_BLOCK_COUNT_MAX)) {
		return false;
	}

	return (data->block_read_mask[block / 32U] & (1UL << (block % 32U))) != 0U;
}

void classic_set_block_read(classic_data_t *data, uint8_t block,
			    const uint8_t block_data[CLASSIC_BLOCK_SIZE])
{
	if ((data == NULL) || (block_data == NULL) || (block >= CLASSIC_BLOCK_COUNT_MAX)) {
		return;
	}

	(void)memcpy(data->blocks[block], block_data, CLASSIC_BLOCK_SIZE);
	data->block_read_mask[block / 32U] |= (1UL << (block % 32U));
}

bool classic_is_key_found(const classic_data_t *data, uint8_t sector, bool key_b)
{
	uint64_t mask;

	if ((data == NULL) || (sector >= CLASSIC_SECTOR_COUNT_MAX)) {
		return false;
	}

	mask = key_b ? data->key_b_mask : data->key_a_mask;

	return (mask & (1ULL << sector)) != 0ULL;
}

void classic_set_key_found(classic_data_t *data, uint8_t sector, bool key_b, uint64_t key)
{
	ARG_UNUSED(key);

	if ((data == NULL) || (sector >= CLASSIC_SECTOR_COUNT_MAX)) {
		return;
	}

	if (key_b) {
		data->key_b_mask |= (1ULL << sector);
	} else {
		data->key_a_mask |= (1ULL << sector);
	}
}

classic_type_t classic_type_from_name(const char *name)
{
	if (name == NULL) {
		return CLASSIC_TYPE_UNKNOWN;
	}

	if (strcmp(name, "MINI") == 0) {
		return CLASSIC_TYPE_MINI;
	}
	if (strcmp(name, "1K") == 0) {
		return CLASSIC_TYPE_1K;
	}
	if (strcmp(name, "4K") == 0) {
		return CLASSIC_TYPE_4K;
	}

	return CLASSIC_TYPE_UNKNOWN;
}
