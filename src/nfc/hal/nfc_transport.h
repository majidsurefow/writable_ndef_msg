/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Backend-neutral NFC transport API (Gate 1).
 */

#ifndef NFC_TRANSPORT_H_
#define NFC_TRANSPORT_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_buf;

typedef uint8_t nfc_role_t;
#define NFC_ROLE_READER BIT(0)
#define NFC_ROLE_LISTEN BIT(1)

typedef uint32_t nfc_tech_t;
#define NFC_TECH_ISO_DEP_A       BIT(0)
#define NFC_TECH_ISO14443_3A_RAW BIT(1)
#define NFC_TECH_ISO14443_3B_RAW BIT(2)
#define NFC_TECH_TYPE2           BIT(3)
#define NFC_TECH_ISO15693        BIT(5)
#define NFC_TECH_ALL_READER      (NFC_TECH_ISO_DEP_A | NFC_TECH_ISO14443_3A_RAW | \
				  NFC_TECH_ISO14443_3B_RAW | NFC_TECH_TYPE2 | \
				  NFC_TECH_ISO15693)

/** Listen response payload limit (PN7160 TML). NFCT backend BUILD_ASSERTs T4T equivalence. */
#define NFC_TRANSPORT_MAX_RESPONSE_LEN 255U

#define NFC_UID_LEN_SINGLE  4U
#define NFC_UID_LEN_DOUBLE  7U
#define NFC_UID_LEN_TRIPLE 10U

#define NFC_TRANSPORT_DEFAULT_FWI 8U

typedef enum {
	NFC_TRANSPORT_STATE_UNINITIALIZED = 0,
	NFC_TRANSPORT_STATE_INITIALIZED,
	NFC_TRANSPORT_STATE_STARTED,
	NFC_TRANSPORT_STATE_STOPPED,
} nfc_transport_state_t;

typedef enum {
	NFC_TIER_SMART = 1,
} nfc_hal_tier_t;

typedef struct {
	uint8_t roles;
	uint32_t technologies;
	nfc_hal_tier_t tier;
} nfc_transport_caps_t;

typedef struct {
	uint8_t fwi;
} nfc_transport_config_t;

typedef struct {
	uint32_t field_on_count;
	uint32_t field_off_count;
	uint32_t fragment_rx_count;
	uint32_t apdu_assembled_count;
	uint32_t response_tx_count;
	uint32_t uid_rotation_count;
	uint32_t frag_dropped_no_buffer;
	uint32_t apdu_oversized_count;
	uint32_t apdu_dropped_no_consumer;
	uint32_t discover_start_count;
	uint32_t tag_detect_count;
	uint32_t transceive_count;
	uint32_t error_count;
	int32_t last_error_code;
} nfc_transport_stats_t;

typedef struct {
	uint8_t bytes[10];
	uint8_t len;
} nfc_uid_t;

typedef struct {
	nfc_uid_t uid;
	uint8_t protocol;
	uint8_t interface;
	uint8_t mode_tech;
	bool valid;
} nfc_transport_tag_info_t;

typedef struct {
	void (*on_field_on)(void *user_ctx);
	void (*on_field_off)(void *user_ctx);
	/** Complete C-APDU; callee owns net_buf ref. Dispatched on nfc_stack_wq. */
	void (*on_apdu)(struct net_buf *apdu, void *user_ctx);
} nfc_transport_ops_t;

/** @threadsafe */
int nfc_transport_init(void);
/** @caller_sync */
int nfc_transport_shutdown(void);
/** @isr_safe */
nfc_transport_state_t nfc_transport_get_state(void);
/** @isr_safe */
const nfc_transport_caps_t *nfc_transport_get_capabilities(void);
/** @isr_safe — never NULL */
const nfc_transport_config_t *nfc_transport_get_config(void);
/** @isr_safe — copy-out */
int nfc_transport_get_stats(nfc_transport_stats_t *out);

/* --- Poll sub-API (reader/) — blocking on nfc_stack_wq --- */

/** @caller_sync — must run on nfc_stack_wq for blocking discover_wait */
int nfc_transport_discover_start(nfc_tech_t tech_mask);
/** @caller_sync */
int nfc_transport_discover_stop(void);
/** @caller_sync — blocks on nfc_stack_wq thread */
int nfc_transport_discover_wait(nfc_transport_tag_info_t *info, k_timeout_t timeout);
/** @caller_sync */
int nfc_transport_tag_transceive(const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_max,
				 size_t *rx_len, k_timeout_t timeout);

/* --- Listen sub-API (nfc_stack/) — Gate 1 skeleton; full CE at Gate 3 --- */

/**
 * @brief Register upward callbacks for field events and complete APDUs.
 *
 * @caller_sync — register after init(), before start(); NULL ops clears.
 */
int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops, void *user_ctx);

/** @caller_sync — start card-emulation listen discovery */
int nfc_transport_start(const nfc_uid_t *uid);
/** @caller_sync */
int nfc_transport_stop(void);

/**
 * @brief Rotate NFCID1 while listen is active.
 *
 * Gate 1 skeleton: stores UID for next start; live rotation at Gate 3.
 *
 * @caller_sync
 */
int nfc_transport_set_uid(const nfc_uid_t *uid);

/** @caller_sync — synchronous response on nfc_stack_wq */
int nfc_transport_send_response(const uint8_t *buf, size_t len);

/** @caller_sync — defer heavy crypto (Aliro) to nfc_stack_wq */
int nfc_transport_submit_work(struct k_work *work);

#ifdef __cplusplus
}
#endif

#endif /* NFC_TRANSPORT_H_ */
