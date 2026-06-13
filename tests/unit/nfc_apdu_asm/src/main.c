/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc_apdu_asm unit tests — frag_fits and oversize drop decision.
 */

#include "hal/nfc_apdu_asm.h"

#include <zephyr/ztest.h>

ZTEST(nfc_apdu_asm, test_fits_empty_and_exact)
{
	zassert_true(nfc_apdu_frag_fits(0U, 512U, 512U));
	zassert_true(nfc_apdu_frag_fits(0U, 1U, 512U));
	zassert_true(nfc_apdu_frag_fits(511U, 1U, 512U));
	zassert_false(nfc_apdu_frag_fits(0U, 513U, 512U));
	zassert_false(nfc_apdu_frag_fits(512U, 1U, 512U));
}

ZTEST(nfc_apdu_asm, test_fits_mid_chain)
{
	zassert_true(nfc_apdu_frag_fits(256U, 256U, 512U));
	zassert_true(nfc_apdu_frag_fits(200U, 312U, 512U));
	zassert_false(nfc_apdu_frag_fits(200U, 313U, 512U));
	zassert_false(nfc_apdu_frag_fits(400U, 113U, 512U));
}

ZTEST(nfc_apdu_asm, test_oversize_drop_decision)
{
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_OK, 511U, 1U, 512U),
		      NFC_APDU_ASM_OK);
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_OK, 511U, 2U, 512U),
		      NFC_APDU_ASM_DROP_OVERSIZE);
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_DROP_NOBUF, 0U, 1U, 512U),
		      NFC_APDU_ASM_DROP_NOBUF);
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_DROP_OVERSIZE, 0U, 1U, 512U),
		      NFC_APDU_ASM_DROP_OVERSIZE);
}

ZTEST(nfc_apdu_asm, test_on_frag_state)
{
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_OK, 200U, 312U, 512U), NFC_APDU_ASM_OK);
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_OK, 200U, 313U, 512U),
		      NFC_APDU_ASM_DROP_OVERSIZE);
	zassert_equal(nfc_apdu_asm_on_frag(NFC_APDU_ASM_DROP_NOBUF, 0U, 1U, 512U),
		      NFC_APDU_ASM_DROP_NOBUF);
}

ZTEST_SUITE(nfc_apdu_asm, NULL, NULL, NULL, NULL, NULL);
