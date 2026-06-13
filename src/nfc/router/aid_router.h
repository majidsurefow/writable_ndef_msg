/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * AID router — ISO-DEP listen dispatch (Gate 3 minimal).
 */

#ifndef NFC_ROUTER_AID_ROUTER_H_
#define NFC_ROUTER_AID_ROUTER_H_

#include <stddef.h>
#include <stdint.h>

#include "framing/apdu_types.h"
#include "service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t max_aids;
} aid_router_config_t;

typedef struct {
	uint32_t error_count;
	int32_t last_error_code;
	uint32_t select_matched_count;
	uint32_t select_unmatched_count;
	uint32_t apdu_routed_count;
	uint32_t apdu_no_service_count;
	uint32_t field_off_count;
	uint32_t register_table_full_count;
} aid_router_stats_t;

typedef enum {
	AID_ROUTER_STATE_UNINITIALIZED = 0,
	AID_ROUTER_STATE_INITIALIZED,
} aid_router_state_t;

int aid_router_init(void);
int aid_router_shutdown(void);

int aid_router_register(const uint8_t *aid, size_t aid_len, const nfc_service_t *svc);
void aid_router_clear(void);
void aid_router_dispatch(const nfc_apdu_t *apdu);
void aid_router_field_off(void);

const aid_router_config_t *aid_router_get_config(void);
int aid_router_get_stats(aid_router_stats_t *out);
aid_router_state_t aid_router_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_ROUTER_AID_ROUTER_H_ */
