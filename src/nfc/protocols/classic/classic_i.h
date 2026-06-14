/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIFARE Classic sector/block helpers — flipperzero mf_classic.c facts.
 */

#ifndef NFC_PROTOCOLS_CLASSIC_I_H_
#define NFC_PROTOCOLS_CLASSIC_I_H_

#include "protocols/classic/classic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLASSIC_CMD_AUTH_KEY_A     0x60U
#define CLASSIC_CMD_AUTH_KEY_B     0x61U
#define CLASSIC_CMD_READ_BLOCK     0x30U
#define CLASSIC_DEFAULT_KEY        0xFFFFFFFFFFFFULL

uint8_t classic_get_total_sectors(classic_type_t type);
uint16_t classic_get_total_blocks(classic_type_t type);
uint8_t classic_get_sector_by_block(uint8_t block);
bool classic_is_sector_trailer(uint8_t block);
uint8_t classic_get_first_block_of_sector(uint8_t sector);
uint8_t classic_get_blocks_in_sector(uint8_t sector);
bool classic_is_block_read(const classic_data_t *data, uint8_t block);
void classic_set_block_read(classic_data_t *data, uint8_t block,
			    const uint8_t block_data[CLASSIC_BLOCK_SIZE]);
bool classic_is_key_found(const classic_data_t *data, uint8_t sector,
			    bool key_b);
void classic_set_key_found(classic_data_t *data, uint8_t sector, bool key_b,
			   uint64_t key);
classic_type_t classic_type_from_name(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_I_H_ */
