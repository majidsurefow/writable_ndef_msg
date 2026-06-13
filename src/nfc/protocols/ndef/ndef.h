/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NDEF Type-4 data model — shared by poller, listener, store (cookbook §5.1).
 */

#ifndef NFC_PROTOCOLS_NDEF_H_
#define NFC_PROTOCOLS_NDEF_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NFC_NDEF_MAX_SIZE
#define CONFIG_NFC_NDEF_MAX_SIZE 500
#endif

#define NFC_PERSIST_ID_NDEF           0x01U
#define NFC_NDEF_FORMAT_VERSION       0x01U
#define NFC_NDEF_CC_LEN               15U
#define NFC_NDEF_NLEN_FIELD_LEN       2U

typedef struct {
	uint8_t cc[NFC_NDEF_CC_LEN];
	uint8_t ndef_file[NFC_NDEF_NLEN_FIELD_LEN + CONFIG_NFC_NDEF_MAX_SIZE];
	uint16_t cc_len;
	uint16_t ndef_file_len;
} ndef_data_t;

void ndef_data_reset(ndef_data_t *data);
uint8_t ndef_persist_id(void);

int ndef_serialize(const ndef_data_t *data, uint8_t *out, size_t out_max, size_t *out_len);
int ndef_deserialize(ndef_data_t *data, const uint8_t *in, size_t in_len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_NDEF_H_ */
