/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal/nfc_apdu_asm.h"

bool nfc_apdu_frag_fits(size_t cur_len, size_t frag_len, size_t buf_cap)
{
	return (cur_len + frag_len) <= buf_cap;
}

nfc_apdu_asm_drop_t nfc_apdu_asm_on_frag(nfc_apdu_asm_drop_t drop, size_t cur_len,
					 size_t frag_len, size_t buf_cap)
{
	if (drop != NFC_APDU_ASM_OK) {
		return drop;
	}

	if (!nfc_apdu_frag_fits(cur_len, frag_len, buf_cap)) {
		return NFC_APDU_ASM_DROP_OVERSIZE;
	}

	return NFC_APDU_ASM_OK;
}
