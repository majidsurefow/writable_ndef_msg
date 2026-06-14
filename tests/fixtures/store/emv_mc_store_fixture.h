/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — EMV Mastercard synthetic .card.bin layout.
 */

#ifndef TESTS_FIXTURES_STORE_EMV_MC_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_EMV_MC_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_emv_mc_card[] = {
#include "Emv_mc_card.inc"
};

#define STORE_FIXTURE_EMV_MC_CARD_LEN sizeof(store_fixture_emv_mc_card)

#endif /* TESTS_FIXTURES_STORE_EMV_MC_STORE_FIXTURE_H_ */
