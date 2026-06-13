/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_EMULATION_H_
#define NFC_EMULATION_H_

/**
 * @file nfc_emulation.h
 *
 * ## What must exist before NFC tag emulation works (Nordic nrfxlib T4T)
 *
 * Build-time (Kconfig), typical for this sample:
 * - CONFIG_NFC_T4T_NRFXLIB=y — links the Type 4 Tag emulation library.
 * - CONFIG_NFC_NDEF / NDEF message modules if you use the NDEF file helpers.
 * - Chip must have NFCT; devicetree enables the peripheral for your board.
 *
 * Type 4 Tag, NDEF read/write mode — runtime contract with @ref nfc_t4t_lib:
 *
 * 1. nfc_t4t_setup(callback, context)
 * 2. nfc_t4t_parameter_set(NFC_T4T_PARAM_NFCID1, ...)
 * 3. nfc_t4t_ndef_rwpayload_set(buffer, buffer_length) (or static / raw mode)
 * 4. nfc_t4t_emulation_start()
 *
 * @ref nfc_on performs 1–4. @ref nfc_off calls nfc_t4t_emulation_stop().
 *
 * Teardown: @ref nfc_off; use nfc_t4t_done() before a second @ref nfc_on.
 */

#include <stddef.h>
#include <stdint.h>

#include <nfc_t4t_lib.h>

typedef enum {
	NFC_TAG_TYPE_2 = 2,
	NFC_TAG_TYPE_4 = 4,
} nfc_tag_type_t;

/** NFCID1 passed to @ref nfc_on; bytes are copied into the module. */
typedef struct {
	const uint8_t *bytes;
	size_t len;
} nfc_uid_t;

typedef struct {
	nfc_t4t_callback_t callback;
	void *callback_context;
	uint8_t *ndef_file_buf;
	size_t ndef_file_buf_size;
} nfc_t4_start_args_t;

/**
 * @brief Setup Type 4 NDEF RW emulation and start sensing the field.
 *
 * @retval 0       Success.
 * @retval -EINVAL Invalid arguments.
 * @retval -ENOTSUP type is not @ref NFC_TAG_TYPE_4.
 */
int nfc_on(const nfc_uid_t *uid, nfc_tag_type_t type, const nfc_t4_start_args_t *t4);

/** @brief Stop sensing the NFC field (nfc_t4t_emulation_stop). */
int nfc_off(void);

/**
 * @brief Stop emulation, set a new NFCID1, start emulation again.
 *
 * Call only after a successful @ref nfc_on (does not re-run @c nfc_t4t_setup).
 */
int nfc_apply_uid_and_restart(const nfc_uid_t *uid);

#endif /* NFC_EMULATION_H_ */
