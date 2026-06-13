/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC card envelope — minimal Gate 2 save/load (wave6 subset).
 */

#ifndef NFC_STORE_NFC_STORE_H_
#define NFC_STORE_NFC_STORE_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_STORE_BLOB_MAGIC_0              0x4EU
#define NFC_STORE_BLOB_MAGIC_1              0x46U
#define NFC_STORE_BLOB_VERSION_V1           0x01U
#define NFC_STORE_BLOB_VERSION              0x02U
#define NFC_STORE_BLOB_HDR_SIZE             6U
#define NFC_STORE_BLOB_CRC_SIZE             2U
#define NFC_STORE_ENVELOPE_OVERHEAD         (NFC_STORE_BLOB_HDR_SIZE + NFC_STORE_BLOB_CRC_SIZE)
#define NFC_STORE_ENTRY_OVERHEAD_V1         3U
#define NFC_STORE_ENTRY_OVERHEAD            4U

#define NFC_STORE_ENTRY_FLAG_READER_CAPTURED    BIT(0)
#define NFC_STORE_ENTRY_FLAG_HAND_AUTHORED      BIT(1)
#define NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE BIT(2)
#define NFC_STORE_ENTRY_FLAG_READ_ONLY_PARTIAL  BIT(3)

#define NFC_STORE_MAX_TAG_LEN_DEFAULT       16U

typedef struct {
	uint8_t max_tag_len;
} nfc_store_config_t;

#define NFC_STORE_CONFIG_DEFAULT \
	((nfc_store_config_t){ .max_tag_len = 0U })

typedef struct {
	uint32_t error_count;
	int32_t last_error_code;
	uint32_t save_count;
	uint32_t load_count;
	uint32_t serialize_skip_count;
	uint32_t deserialize_skip_count;
	uint32_t corrupt_blob_count;
	uint32_t partial_load_count;
	uint32_t live_commit_count;
} nfc_store_stats_t;

typedef enum {
	NFC_STORE_STATE_UNINITIALIZED = 0,
	NFC_STORE_STATE_INITIALIZED,
} nfc_store_state_t;

typedef int (*nfc_store_save_fn)(const char *tag, const uint8_t *blob, size_t len,
				 void *user_ctx);

typedef int (*nfc_store_load_fn)(const char *tag, uint8_t *out, size_t max, size_t *out_len,
				 void *user_ctx);

typedef int (*nfc_store_commit_fn)(const nfc_service_t *svc, const char *tag, void *user_ctx);

int nfc_store_init(const nfc_store_config_t *cfg);
int nfc_store_shutdown(void);

int nfc_store_register_save_cb(nfc_store_save_fn fn, void *user_ctx);
int nfc_store_register_load_cb(nfc_store_load_fn fn, void *user_ctx);
int nfc_store_register_commit_cb(nfc_store_commit_fn fn, void *user_ctx);

int nfc_store_save(const char *tag, const nfc_service_t *const *svcs, size_t n);
int nfc_store_load(const char *tag, const nfc_service_t *const *svcs, size_t n);
int nfc_store_peek_entry_meta(const char *tag, uint8_t *persist_id, uint8_t *flags);
int nfc_store_on_dirty(const nfc_service_t *svc, const char *tag);

const nfc_store_config_t *nfc_store_get_config(void);
int nfc_store_get_stats(nfc_store_stats_t *out);
nfc_store_state_t nfc_store_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_STORE_NFC_STORE_H_ */
