/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * FeliCa data model — clone-only (cookbook §5.7 / F4).
 */

#ifndef NFC_PROTOCOLS_FELICA_H_
#define NFC_PROTOCOLS_FELICA_H_

#include <stddef.h>
#include <stdint.h>

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FELICA_FORMAT_VERSION   0x02U
#define FELICA_IDM_SIZE         8U
#define FELICA_PMM_SIZE         8U
#define FELICA_BLOCK_DATA_SIZE  16U
#define FELICA_BLOCKS_MAX       64U

typedef enum {
	FELICA_WORKFLOW_UNKNOWN = 0,
	FELICA_WORKFLOW_STANDARD,
	FELICA_WORKFLOW_LITE,
} felica_workflow_type_t;

typedef struct {
	uint8_t idm[FELICA_IDM_SIZE];
	uint8_t pmm[FELICA_PMM_SIZE];
	uint16_t blocks_total;
	uint16_t blocks_read;
	struct {
		uint8_t sf1;
		uint8_t sf2;
		uint8_t data[FELICA_BLOCK_DATA_SIZE];
	} blocks[FELICA_BLOCKS_MAX];
} felica_data_t;

void felica_data_reset(felica_data_t *data);
felica_workflow_type_t felica_get_workflow_type(const uint8_t pmm[FELICA_PMM_SIZE]);
uint8_t felica_persist_id(void);

int felica_serialize(const felica_data_t *data, uint8_t *out, size_t out_max, size_t *out_len);
int felica_deserialize(felica_data_t *data, const uint8_t *in, size_t in_len);
int felica_compare(const felica_data_t *expected, const felica_data_t *actual);

const nfc_service_t *felica_service_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_FELICA_H_ */
