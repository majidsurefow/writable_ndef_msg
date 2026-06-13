/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Backend-neutral NFC transport API (Gate 0 scaffold).
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

typedef uint8_t nfc_role_t;
#define NFC_ROLE_READER BIT(1)

typedef uint32_t nfc_tech_t;
#define NFC_TECH_ISO_DEP_A       BIT(0)
#define NFC_TECH_ISO14443_3A_RAW BIT(1)
#define NFC_TECH_ISO14443_3B_RAW BIT(2)
#define NFC_TECH_TYPE2           BIT(3)
#define NFC_TECH_ISO15693        BIT(5)
#define NFC_TECH_ALL_READER      (NFC_TECH_ISO_DEP_A | NFC_TECH_ISO14443_3A_RAW | \
				  NFC_TECH_ISO14443_3B_RAW | NFC_TECH_TYPE2 | \
				  NFC_TECH_ISO15693)

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

int nfc_transport_init(void);
int nfc_transport_shutdown(void);
nfc_transport_state_t nfc_transport_get_state(void);
const nfc_transport_caps_t *nfc_transport_get_capabilities(void);

int nfc_transport_discover_start(nfc_tech_t tech_mask);
int nfc_transport_discover_stop(void);
int nfc_transport_discover_wait(nfc_transport_tag_info_t *info, k_timeout_t timeout);

int nfc_transport_tag_transceive(const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_max,
				 size_t *rx_len, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* NFC_TRANSPORT_H_ */
