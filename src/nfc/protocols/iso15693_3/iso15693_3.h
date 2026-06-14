/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * ISO15693-3 data model — parent of SLIX (cookbook §5.8).
 */

#ifndef NFC_PROTOCOLS_ISO15693_3_H_
#define NFC_PROTOCOLS_ISO15693_3_H_

#include <stddef.h>
#include <stdint.h>

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISO15693_FORMAT_VERSION  0x01U
#define ISO15693_UID_SIZE        8U
#define ISO15693_BLOCKS_MAX      128U
#define ISO15693_BLOCK_SIZE_MAX  4U

typedef struct {
	uint8_t uid[ISO15693_UID_SIZE];
	uint8_t dsfid;
	uint8_t afi;
	uint8_t ic_ref;
	uint8_t lock_dsfid;
	uint8_t lock_afi;
	uint16_t block_count;
	uint8_t block_size;
	uint8_t block_data[ISO15693_BLOCKS_MAX * ISO15693_BLOCK_SIZE_MAX];
	uint8_t block_security[ISO15693_BLOCKS_MAX];
} iso15693_3_data_t;

void iso15693_3_data_reset(iso15693_3_data_t *data);
uint8_t iso15693_3_persist_id(void);

int iso15693_3_serialize(const iso15693_3_data_t *data, uint8_t *out, size_t out_max,
			 size_t *out_len);
int iso15693_3_deserialize(iso15693_3_data_t *data, const uint8_t *in, size_t in_len);
int iso15693_3_compare(const iso15693_3_data_t *expected, const iso15693_3_data_t *actual);

const nfc_service_t *iso15693_3_service_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ISO15693_3_H_ */
