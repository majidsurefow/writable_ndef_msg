/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — FeliCa .card.bin (clone-only).
 */

#ifndef TESTS_FIXTURES_STORE_FELICA_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_FELICA_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_felica_card[] = {
#include "Felica_card.inc"
};

#define STORE_FIXTURE_FELICA_CARD_LEN sizeof(store_fixture_felica_card)

#endif /* TESTS_FIXTURES_STORE_FELICA_STORE_FIXTURE_H_ */
