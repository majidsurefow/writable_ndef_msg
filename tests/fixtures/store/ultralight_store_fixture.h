/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — ultralight .card.bin layouts.
 */

#ifndef TESTS_FIXTURES_STORE_ULTRALIGHT_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_ULTRALIGHT_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_ultralight_11_card[] = {
#include "Ultralight_11_card.inc"
};

#define STORE_FIXTURE_ULTRALIGHT_11_CARD_LEN sizeof(store_fixture_ultralight_11_card)

static const uint8_t store_fixture_ultralight_21_card[] = {
#include "Ultralight_21_card.inc"
};

#define STORE_FIXTURE_ULTRALIGHT_21_CARD_LEN sizeof(store_fixture_ultralight_21_card)

static const uint8_t store_fixture_ultralight_c_card[] = {
#include "Ultralight_C_card.inc"
};

#define STORE_FIXTURE_ULTRALIGHT_C_CARD_LEN sizeof(store_fixture_ultralight_c_card)

static const uint8_t store_fixture_ntag213_locked_card[] = {
#include "Ntag213_locked_card.inc"
};

#define STORE_FIXTURE_NTAG213_LOCKED_CARD_LEN sizeof(store_fixture_ntag213_locked_card)

static const uint8_t store_fixture_ntag215_card[] = {
#include "Ntag215_card.inc"
};

#define STORE_FIXTURE_NTAG215_CARD_LEN sizeof(store_fixture_ntag215_card)

static const uint8_t store_fixture_ntag216_card[] = {
#include "Ntag216_card.inc"
};

#define STORE_FIXTURE_NTAG216_CARD_LEN sizeof(store_fixture_ntag216_card)

#endif /* TESTS_FIXTURES_STORE_ULTRALIGHT_STORE_FIXTURE_H_ */
