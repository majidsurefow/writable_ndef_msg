/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * MIFARE DESFire data model — cookbook §5.4 / wave5-desfire §1.
 */

#ifndef NFC_PROTOCOLS_DESFIRE_H_
#define NFC_PROTOCOLS_DESFIRE_H_

#include <stddef.h>
#include <stdint.h>

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_NFC_DESFIRE_MAX_APPS
#define CONFIG_NFC_DESFIRE_MAX_APPS 1
#endif
#ifndef CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP
#define CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP 2
#endif
#ifndef CONFIG_NFC_DESFIRE_MAX_FILE_SIZE
#define CONFIG_NFC_DESFIRE_MAX_FILE_SIZE 64
#endif

#define DESFIRE_FORMAT_VERSION       0x01U
#define DESFIRE_CLA_NATIVE           0x90U

#define DESFIRE_CMD_GET_VERSION          0x60U
#define DESFIRE_CMD_GET_FREE_MEMORY      0x6EU
#define DESFIRE_CMD_GET_KEY_SETTINGS     0x45U
#define DESFIRE_CMD_GET_KEY_VERSION      0x64U
#define DESFIRE_CMD_GET_APPLICATION_IDS  0x6AU
#define DESFIRE_CMD_SELECT_APPLICATION   0x5AU
#define DESFIRE_CMD_GET_FILE_IDS         0x6FU
#define DESFIRE_CMD_GET_FILE_SETTINGS    0xF5U
#define DESFIRE_CMD_READ_DATA            0xBDU
#define DESFIRE_CMD_GET_VALUE            0x6CU
#define DESFIRE_CMD_READ_RECORDS         0xBBU
#define DESFIRE_CMD_AUTHENTICATE_AES     0xAAU
#define DESFIRE_CMD_AUTH_EV2_FIRST       0x71U
#define DESFIRE_CMD_ADDITIONAL_FRAME     0xAFU

#define DESFIRE_SW1                       0x91U
#define DESFIRE_STATUS_OK                  0x00U
#define DESFIRE_STATUS_ILLEGAL_CMD         0x1CU
#define DESFIRE_STATUS_LENGTH_ERROR          0x7EU
#define DESFIRE_STATUS_NO_SUCH_KEY         0x40U
#define DESFIRE_STATUS_PERMISSION_DENIED   0x9DU
#define DESFIRE_STATUS_BOUNDARY_ERROR        0xBEU
#define DESFIRE_STATUS_AUTH_ERROR          0xAEU
#define DESFIRE_STATUS_ADDITIONAL_FRAME    0xAFU
#define DESFIRE_STATUS_APP_NOT_FOUND       0xA0U
#define DESFIRE_STATUS_FILE_NOT_FOUND      0xF0U

#define DESFIRE_APP_ID_SIZE      3U
#define DESFIRE_MAX_KEYS        14U
#define DESFIRE_AES_KEY_SIZE    16U
#define DESFIRE_RND_SIZE        16U
#define DESFIRE_UID_SIZE         7U
#define DESFIRE_BATCH_SIZE       5U
#define DESFIRE_VERSION_SIZE     7U
#define DESFIRE_AID_LEN          7U

#define DESFIRE_VERSION_FRAME_COUNT 3U
#define DESFIRE_VERSION_TOTAL_BYTES 28U

typedef enum {
	DESFIRE_FILE_TYPE_STANDARD = 0U,
	DESFIRE_FILE_TYPE_BACKUP = 1U,
	DESFIRE_FILE_TYPE_VALUE = 2U,
	DESFIRE_FILE_TYPE_LINEAR_REC = 3U,
	DESFIRE_FILE_TYPE_CYCLIC_REC = 4U,
} desfire_file_type_t;

typedef enum {
	DESFIRE_COMM_PLAIN = 0U,
	DESFIRE_COMM_MAC = 1U,
	DESFIRE_COMM_ENCRYPTED = 3U,
} desfire_comm_t;

typedef enum {
	DESFIRE_AUTH_STATE_IDLE = 0,
	DESFIRE_AUTH_STATE_STEP1,
	DESFIRE_AUTH_STATE_STEP2,
	DESFIRE_AUTH_STATE_AUTHENTICATED,
} desfire_auth_state_t;

typedef union {
	struct {
		uint32_t size;
	} data;
	struct {
		int32_t lo_limit;
		int32_t hi_limit;
		int32_t limited_credit_value;
		uint8_t limited_credit_enabled;
	} value;
	struct {
		uint32_t record_size;
		uint32_t max_records;
		uint32_t cur_records;
	} record;
} desfire_file_settings_union_t;

typedef struct {
	desfire_file_type_t type;
	desfire_comm_t comm;
	uint16_t access_rights;
	desfire_file_settings_union_t settings;
} desfire_file_settings_t;

typedef struct {
	uint8_t app_id[DESFIRE_APP_ID_SIZE];
	uint8_t key_settings;
	uint8_t key_count;
	uint8_t file_count;
	uint8_t keys[DESFIRE_MAX_KEYS][DESFIRE_AES_KEY_SIZE];
	uint8_t file_ids[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP];
	desfire_file_settings_t file_settings[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP];
	uint16_t file_data_len[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP];
	uint8_t file_data[CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP][CONFIG_NFC_DESFIRE_MAX_FILE_SIZE];
} desfire_application_t;

typedef struct {
	uint8_t hw_version[DESFIRE_VERSION_SIZE];
	uint8_t sw_version[DESFIRE_VERSION_SIZE];
	uint8_t uid[DESFIRE_UID_SIZE];
	uint8_t batch[DESFIRE_BATCH_SIZE];
	uint8_t prod_week;
	uint8_t prod_year;
	uint32_t free_memory;
	uint8_t master_key[DESFIRE_AES_KEY_SIZE];
	uint8_t master_key_settings;
	uint8_t app_count;
	desfire_application_t apps[CONFIG_NFC_DESFIRE_MAX_APPS];
} desfire_data_t;

extern const uint8_t desfire_aid[DESFIRE_AID_LEN];

void desfire_data_reset(desfire_data_t *data);
uint8_t desfire_persist_id(void);

int desfire_serialize(const desfire_data_t *data, uint8_t *out, size_t out_max, size_t *out_len);
int desfire_deserialize(desfire_data_t *data, const uint8_t *in, size_t in_len);

uint16_t desfire_rot_left_16(uint16_t value, uint8_t shift);
void desfire_build_version_bytes(const desfire_data_t *data, uint8_t *out);
bool desfire_access_free_read(uint16_t access_rights);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_DESFIRE_H_ */
