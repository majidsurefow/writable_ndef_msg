/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * In-RAM multi-slot backend for nfc_store save/load callbacks.
 */

#ifndef NFC_STORE_NFC_STORE_RAM_H_
#define NFC_STORE_NFC_STORE_RAM_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

int nfc_store_ram_save_cb(const char *tag, const uint8_t *blob, size_t len, void *user_ctx);
int nfc_store_ram_load_cb(const char *tag, uint8_t *out, size_t max, size_t *out_len,
			  void *user_ctx);

/** Register RAM save/load callbacks with nfc_store (idempotent). */
int nfc_store_ram_init(void);

/** Clear all RAM slots (unit tests). */
void nfc_store_ram_reset(void);

#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)
extern const union shell_cmd_entry nfc_store_ram_cmds;
#endif

#ifdef __cplusplus
}
#endif

#endif /* NFC_STORE_NFC_STORE_RAM_H_ */
