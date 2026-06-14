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
	/** Raw NFC-A Type-2 / Ultralight command bytes (0x30 READ, 0x1A AUTH, …). */
	void (*on_tag_cmd)(const uint8_t *tx, size_t tx_len, void *user_ctx);
	void (*on_deselect)(void *user_ctx);
	void (*on_field_off)(void *user_ctx);
	int (*serialize)(uint8_t *out, size_t out_max, size_t *out_len, void *user_ctx);
	int (*deserialize)(const uint8_t *in, size_t in_len, void *user_ctx);
	uint8_t persist_id;
	void *user_ctx;
} nfc_service_t;

/* Persist ID assignments — stable; do not renumber (wave3-router §1.1, cookbook). */
#define NFC_PERSIST_ID_NDEF        0x01U
#define NFC_PERSIST_ID_DESFIRE     0x02U
#define NFC_PERSIST_ID_ULTRALIGHT  0x03U
#define NFC_PERSIST_ID_EMV         0x04U
#define NFC_PERSIST_ID_ALIRO       0x05U
#define NFC_PERSIST_ID_CLASSIC     0x06U
#define NFC_PERSIST_ID_FELICA      0x07U
#define NFC_PERSIST_ID_ISO15693    0x08U
#define NFC_PERSIST_ID_SLIX        0x09U
#define NFC_PERSIST_ID_ST25TB      0x0AU
#define NFC_PERSIST_ID_MAX         0x0AU

#ifdef __cplusplus
}
#endif

#endif /* NFC_ROUTER_SERVICE_H_ */
