/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC service vtable — aid_router dispatch target (Wave 3 router).
 */

#ifndef NFC_ROUTER_SERVICE_H_
#define NFC_ROUTER_SERVICE_H_

#include <stddef.h>
#include <stdint.h>

#include "framing/apdu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void (*on_select)(const uint8_t *aid, size_t aid_len, void *user_ctx);
	void (*on_apdu)(const nfc_apdu_t *apdu, void *user_ctx);
	void (*on_deselect)(void *user_ctx);
	void (*on_field_off)(void *user_ctx);
	int (*serialize)(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx);
	int (*deserialize)(const uint8_t *in, size_t in_len, void *user_ctx);
	uint8_t persist_id;
	void *user_ctx;
} nfc_service_t;

#ifdef __cplusplus
}
#endif

#endif /* NFC_ROUTER_SERVICE_H_ */
