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

static const uint8_t store_fixture_mfclassic_1k_7b_card[] = {
#include "MfClassic_1K_7b_card.inc"
};

#define STORE_FIXTURE_MFCLASSIC_1K_7B_CARD_LEN sizeof(store_fixture_mfclassic_1k_7b_card)

static const uint8_t store_fixture_mfclassic_4k_4b_card[] = {
#include "MfClassic_4K_4b_card.inc"
};

#define STORE_FIXTURE_MFCLASSIC_4K_4B_CARD_LEN sizeof(store_fixture_mfclassic_4k_4b_card)

static const uint8_t store_fixture_mfclassic_mini_4b_card[] = {
#include "MfClassic_Mini_4b_card.inc"
};

#define STORE_FIXTURE_MFCLASSIC_MINI_4B_CARD_LEN sizeof(store_fixture_mfclassic_mini_4b_card)

#endif /* TESTS_FIXTURES_STORE_CLASSIC_STORE_FIXTURE_H_ */
