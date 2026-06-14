/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIFARE Classic data model — clone-only (cookbook §5.3 / F2).
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_classic/mf_classic.h
 */

#ifndef NFC_PROTOCOLS_CLASSIC_H_
#define NFC_PROTOCOLS_CLASSIC_H_

#include <stddef.h>
#include <stdint.h>

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLASSIC_FORMAT_VERSION        0x01U
#define CLASSIC_BLOCK_SIZE            16U
#define CLASSIC_BLOCK_COUNT_MAX       256U
#define CLASSIC_SECTOR_COUNT_MAX      40U
#define CLASSIC_KEY_SIZE              6U
#define CLASSIC_BLOCK_MASK_WORDS      8U

typedef enum {
	CLASSIC_TYPE_MINI = 0U,
	CLASSIC_TYPE_1K,
	CLASSIC_TYPE_4K,
	CLASSIC_TYPE_UNKNOWN = 0xFFU,
} classic_type_t;

typedef struct {
	classic_type_t type;
	uint8_t uid[10];
	uint8_t uid_len;
	uint8_t sak;
	uint8_t atqa[2];
	uint32_t block_read_mask[CLASSIC_BLOCK_MASK_WORDS];
	uint64_t key_a_mask;
	uint64_t key_b_mask;
	uint8_t blocks[CLASSIC_BLOCK_COUNT_MAX][CLASSIC_BLOCK_SIZE];
} classic_data_t;

void classic_data_reset(classic_data_t *data);
uint8_t classic_persist_id(void);

int classic_serialize(const classic_data_t *data, uint8_t *out, size_t out_max,
		      size_t *out_len);
int classic_deserialize(classic_data_t *data, const uint8_t *in, size_t in_len);

const nfc_service_t *classic_service_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_H_ */
