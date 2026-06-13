/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Pure helpers for inbound APDU fragment assembly (HAL listen path).
 */

#ifndef NFC_APDU_ASM_H_
#define NFC_APDU_ASM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Assembly drop reason mirrored by listen backends (nrfx ISR path). */
typedef enum {
	NFC_APDU_ASM_OK = 0,
	NFC_APDU_ASM_DROP_NOBUF,
	NFC_APDU_ASM_DROP_OVERSIZE,
} nfc_apdu_asm_drop_t;

/**
 * @brief Return true if @p frag_len bytes fit after @p cur_len in a @p buf_cap buffer.
 *
 * Used by the NFCT ISR assembly path before net_buf_add_mem().
 */
bool nfc_apdu_frag_fits(size_t cur_len, size_t frag_len, size_t buf_cap);

/**
 * @brief Update drop state after a fragment size check (pure, testable).
 *
 * If @p drop is NFC_APDU_ASM_OK and the fragment would overflow @p buf_cap,
 * returns NFC_APDU_ASM_DROP_OVERSIZE; otherwise returns @p drop unchanged.
 */
nfc_apdu_asm_drop_t nfc_apdu_asm_on_frag(nfc_apdu_asm_drop_t drop, size_t cur_len,
					 size_t frag_len, size_t buf_cap);

#ifdef __cplusplus
}
#endif

#endif /* NFC_APDU_ASM_H_ */
