/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Inbound APDU parse + router dispatch (Gate 3 minimal).
 */

#ifndef NFC_FRAMING_APDU_ASSEMBLER_H_
#define NFC_FRAMING_APDU_ASSEMBLER_H_

#include "hal/nfc_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool extended_apdu_support;
} apdu_assembler_config_t;

typedef struct {
	uint32_t error_count;
	int32_t last_error_code;
	uint32_t apdu_rx_count;
	uint32_t apdu_parse_ok_count;
	uint32_t apdu_parse_reject_count;
	uint32_t field_on_count;
	uint32_t field_off_count;
} apdu_assembler_stats_t;

typedef enum {
	APDU_ASSEMBLER_STATE_UNINITIALIZED = 0,
	APDU_ASSEMBLER_STATE_INITIALIZED,
} apdu_assembler_state_t;

int apdu_assembler_init(const apdu_assembler_config_t *cfg);
int apdu_assembler_shutdown(void);

const nfc_transport_ops_t *apdu_assembler_get_ops(void);
const apdu_assembler_config_t *apdu_assembler_get_config(void);
int apdu_assembler_get_stats(apdu_assembler_stats_t *out);
apdu_assembler_state_t apdu_assembler_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_FRAMING_APDU_ASSEMBLER_H_ */
