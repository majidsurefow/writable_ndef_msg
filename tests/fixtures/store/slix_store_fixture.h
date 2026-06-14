/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — SLIX .card.bin layouts (clone-only).
 */

#ifndef TESTS_FIXTURES_STORE_SLIX_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_SLIX_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_slix_cap_default_card[] = {
#include "Slix_cap_default_card.inc"
};

#define STORE_FIXTURE_SLIX_CAP_DEFAULT_CARD_LEN sizeof(store_fixture_slix_cap_default_card)

static const uint8_t store_fixture_slix_cap_missed_card[] = {
#include "Slix_cap_missed_card.inc"
};

#define STORE_FIXTURE_SLIX_CAP_MISSED_CARD_LEN sizeof(store_fixture_slix_cap_missed_card)

static const uint8_t store_fixture_slix_cap_accept_all_pass_card[] = {
#include "Slix_cap_accept_all_pass_card.inc"
};

#define STORE_FIXTURE_SLIX_CAP_ACCEPT_ALL_PASS_CARD_LEN \
	sizeof(store_fixture_slix_cap_accept_all_pass_card)

#endif /* TESTS_FIXTURES_STORE_SLIX_STORE_FIXTURE_H_ */
