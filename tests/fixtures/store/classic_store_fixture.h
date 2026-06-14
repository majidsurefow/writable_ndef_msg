/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — classic .card.bin layouts (clone-only).
 */

#ifndef TESTS_FIXTURES_STORE_CLASSIC_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_CLASSIC_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_mfclassic_1k_4b_card[] = {
#include "MfClassic_1K_4b_card.inc"
};

#define STORE_FIXTURE_MFCLASSIC_1K_4B_CARD_LEN sizeof(store_fixture_mfclassic_1k_4b_card)

#endif /* TESTS_FIXTURES_STORE_CLASSIC_STORE_FIXTURE_H_ */
