/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — Aliro .card.bin layout.
 */

#ifndef TESTS_FIXTURES_STORE_ALIRO_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_ALIRO_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_aliro_card[] = {
#include "Aliro_card.inc"
};

#define STORE_FIXTURE_ALIRO_CARD_LEN sizeof(store_fixture_aliro_card)

#endif /* TESTS_FIXTURES_STORE_ALIRO_STORE_FIXTURE_H_ */
