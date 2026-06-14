/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP SLIX data model — child of ISO15693-3 (cookbook §5.9).
 */

#ifndef NFC_PROTOCOLS_SLIX_H_
#define NFC_PROTOCOLS_SLIX_H_

#include <stddef.h>
#include <stdint.h>

#include "protocols/iso15693_3/iso15693_3.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLIX_EXT_MAGIC_0       'S'
#define SLIX_EXT_MAGIC_1       'L'
#define SLIX_EXT_VERSION       0x01U
#define SLIX_PASSWORD_COUNT    5U
#define SLIX_PASSWORD_SIZE     4U
#define SLIX_SIGNATURE_SIZE    32U
#define SLIX_NXP_MFG_CODE      0x04U

typedef enum {
	SLIX_CAP_DEFAULT = 0U,
	SLIX_CAP_ACCEPT_ALL,
	SLIX_CAP_MISSED,
} slix_capabilities_t;

typedef enum {
	SLIX_TYPE_UNKNOWN = 0xFFU,
	SLIX_TYPE_SLIX = 0U,
	SLIX_TYPE_SLIX_S,
	SLIX_TYPE_SLIX_L,
	SLIX_TYPE_SLIX2,
} slix_type_t;

typedef struct {
	iso15693_3_data_t parent;
	slix_capabilities_t capabilities;
	slix_type_t type;
	uint8_t passwords[SLIX_PASSWORD_COUNT][SLIX_PASSWORD_SIZE];
	uint8_t signature[SLIX_SIGNATURE_SIZE];
	uint8_t privacy_mode;
	uint8_t protection_pointer;
	uint8_t protection_condition;
	uint8_t lock_eas;
	uint8_t lock_ppl;
} slix_data_t;

void slix_data_reset(slix_data_t *data);
uint8_t slix_persist_id(void);
slix_type_t slix_type_from_uid(const uint8_t uid[ISO15693_UID_SIZE]);

int slix_serialize(const slix_data_t *data, uint8_t *out, size_t out_max, size_t *out_len);
int slix_deserialize(slix_data_t *data, const uint8_t *in, size_t in_len);

const nfc_service_t *slix_service_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_SLIX_H_ */
