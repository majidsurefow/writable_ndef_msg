/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIFARE Ultralight / NTAG data model — poller + T4 adapter (cookbook §5.2).
 * Behavioral reference: flipperzero/lib/nfc/protocols/mf_ultralight/mf_ultralight.h
 */

#ifndef NFC_PROTOCOLS_ULTRALIGHT_H_
#define NFC_PROTOCOLS_ULTRALIGHT_H_

#include <stddef.h>
#include <stdint.h>

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NFC_ULTRALIGHT_MAX_PAGES
#define CONFIG_NFC_ULTRALIGHT_MAX_PAGES 256
#endif

#define ULTRALIGHT_FORMAT_VERSION       0x01U
#define ULTRALIGHT_PAGE_SIZE            4U
#define ULTRALIGHT_CC_PAGE              3U
#define ULTRALIGHT_DATA_START_PAGE      4U
#define ULTRALIGHT_VERSION_SIZE         8U
#define ULTRALIGHT_SIGNATURE_SIZE       32U
#define ULTRALIGHT_COUNTER_NUM          3U
#define ULTRALIGHT_TEARING_FLAG_NUM     3U

#define ULTRALIGHT_TLV_NULL               0x00U
#define ULTRALIGHT_TLV_NDEF               0x03U
#define ULTRALIGHT_TLV_TERMINATOR         0xFEU
#define ULTRALIGHT_TLV_LONG_FORM          0xFFU

#define ULTRALIGHT_CMD_READ               0x30U
#define ULTRALIGHT_CMD_GET_VERSION        0x60U
#define ULTRALIGHT_CMD_READ_SIG           0x3CU
#define ULTRALIGHT_CMD_READ_CNT           0x39U
#define ULTRALIGHT_CMD_CHECK_TEARING      0x3EU
#define ULTRALIGHT_CMD_AUTH               0x1AU
#define ULTRALIGHT_CMD_AUTH_CONT          0xAFU
#define ULTRALIGHT_CMD_PWD_AUTH           0x1BU

#define ULTRALIGHT_READ_RESP_LEN          16U
#define ULTRALIGHT_PWD_AUTH_RESP_LEN      2U
#define ULTRALIGHT_AUTH0_NONE             0xFFU
#define ULTRALIGHT_CONFIG_BLOCK_PAGES     5U

/** Serialized blob max — 256 pages + optional EV1 fields. */
#define ULTRALIGHT_MAX_SERIALIZED \
	(4U + (CONFIG_NFC_ULTRALIGHT_MAX_PAGES * ULTRALIGHT_PAGE_SIZE) + \
	 1U + ULTRALIGHT_VERSION_SIZE + 1U + ULTRALIGHT_SIGNATURE_SIZE + \
	 1U + (ULTRALIGHT_COUNTER_NUM * 3U) + 1U + ULTRALIGHT_TEARING_FLAG_NUM)

typedef enum {
	UL_VARIANT_ORIGIN = 0U,
	UL_VARIANT_UL11 = 1U,
	UL_VARIANT_UL21 = 2U,
	UL_VARIANT_UNKNOWN = 0xFFU,
} ultralight_variant_t;

typedef enum {
	UL_TYPE_ORIGIN = 0,
	UL_TYPE_UL11,
	UL_TYPE_UL21,
	UL_TYPE_NTAG203,
	UL_TYPE_NTAG213,
	UL_TYPE_NTAG215,
	UL_TYPE_NTAG216,
	UL_TYPE_MFUL_C,
	UL_TYPE_UNKNOWN,
} ultralight_type_t;

typedef struct {
	uint8_t pages[CONFIG_NFC_ULTRALIGHT_MAX_PAGES][ULTRALIGHT_PAGE_SIZE];
	uint16_t pages_total;
	uint16_t pages_read;
	ultralight_type_t type;
	ultralight_variant_t variant;
	uint8_t version[ULTRALIGHT_VERSION_SIZE];
	bool has_version;
	uint8_t signature[ULTRALIGHT_SIGNATURE_SIZE];
	bool has_signature;
	uint8_t counters[ULTRALIGHT_COUNTER_NUM][3];
	uint8_t counter_count;
	uint8_t tearing_flags[ULTRALIGHT_TEARING_FLAG_NUM];
	uint8_t tearing_flag_count;
} ultralight_data_t;

/** NTAG21x config block — last five pages (lock, AUTH0, ACCESS, PWD, PACK). */
typedef struct {
	uint8_t auth0;
	bool prot;
	uint8_t authlim;
	uint8_t pwd[ULTRALIGHT_PAGE_SIZE];
	uint8_t pack[ULTRALIGHT_PWD_AUTH_RESP_LEN];
	bool valid;
} ultralight_config_t;

void ultralight_data_reset(ultralight_data_t *data);
uint8_t ultralight_persist_id(void);
uint16_t ultralight_pages_total_for_type(ultralight_type_t type);

int ultralight_serialize(const ultralight_data_t *data, uint8_t *out, size_t out_max,
			 size_t *out_len);
int ultralight_deserialize(ultralight_data_t *data, const uint8_t *in, size_t in_len);

int ultralight_scan_ndef_tlv(const uint8_t *data, size_t data_len, const uint8_t **msg_out,
			     size_t *msg_len_out);
int ultralight_extract_ndef(const ultralight_data_t *data, const uint8_t **msg, size_t *msg_len);

bool ultralight_type_has_password_config(ultralight_type_t type);
void ultralight_config_page_indices(uint16_t pages_total, uint16_t *auth0_page,
				    uint16_t *access_page, uint16_t *pwd_page,
				    uint16_t *pack_page);
bool ultralight_parse_config(const ultralight_data_t *data, ultralight_config_t *cfg);
bool ultralight_read_protection_enabled(const ultralight_config_t *cfg);
bool ultralight_page_needs_auth(const ultralight_config_t *cfg, uint16_t page);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_ULTRALIGHT_H_ */
