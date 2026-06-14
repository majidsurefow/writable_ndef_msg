/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Human-readable names for nfc_service persist_id values.
 */

#ifndef NFC_STORE_NFC_PERSIST_NAME_H_
#define NFC_STORE_NFC_PERSIST_NAME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @return protocol label, or "unknown" for unrecognized ids. */
const char *nfc_persist_id_name(uint8_t persist_id);

#ifdef __cplusplus
}
#endif

#endif /* NFC_STORE_NFC_PERSIST_NAME_H_ */
