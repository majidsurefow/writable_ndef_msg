/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC listen orchestrator — Gate 3 minimal NDEF profile.
 */

#ifndef NFC_NFC_STACK_NFC_STACK_H_
#define NFC_NFC_STACK_NFC_STACK_H_

#include <stdint.h>

#include "hal/nfc_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	NFC_PROFILE_NONE = 0,
	NFC_PROFILE_NDEF = 1,
} nfc_profile_t;

typedef struct {
	nfc_profile_t default_profile;
} nfc_stack_config_t;

#define NFC_STACK_CONFIG_DEFAULT \
	((nfc_stack_config_t){ .default_profile = NFC_PROFILE_NDEF })

typedef struct {
	uint32_t error_count;
	int32_t last_error_code;
	uint32_t start_count;
	uint32_t stop_count;
	uint32_t load_count;
} nfc_stack_stats_t;

typedef enum {
	NFC_STACK_STATE_UNINITIALIZED = 0,
	NFC_STACK_STATE_INITIALIZED,
	NFC_STACK_STATE_STARTED,
	NFC_STACK_STATE_STOPPED,
	NFC_STACK_STATE_ERROR,
} nfc_stack_state_t;

int nfc_stack_init(const nfc_stack_config_t *cfg);
int nfc_stack_start(const nfc_uid_t *uid);
int nfc_stack_stop(void);
int nfc_stack_shutdown(void);

int nfc_stack_load(const char *tag);

const nfc_stack_config_t *nfc_stack_get_config(void);
int nfc_stack_get_stats(nfc_stack_stats_t *out);
nfc_stack_state_t nfc_stack_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_NFC_STACK_NFC_STACK_H_ */
