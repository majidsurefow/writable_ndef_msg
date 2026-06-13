/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tier E store envelope goldens — nfc_store_save layout.
 */

#ifndef TESTS_FIXTURES_STORE_STORE_FIXTURE_H_
#define TESTS_FIXTURES_STORE_STORE_FIXTURE_H_

#include <stddef.h>
#include <stdint.h>

static const uint8_t store_fixture_ndef_empty_card[] = {
#include "ndef_empty_card.inc"
};

#define STORE_FIXTURE_NDEF_EMPTY_CARD_LEN sizeof(store_fixture_ndef_empty_card)

static const uint8_t store_fixture_ndef_uri_5byte_card[] = {
#include "ndef_uri_5byte_card.inc"
};

#define STORE_FIXTURE_NDEF_URI_5BYTE_CARD_LEN sizeof(store_fixture_ndef_uri_5byte_card)

static const uint8_t store_fixture_ndef_chunk_255_card[] = {
#include "ndef_chunk_255_card.inc"
};

#define STORE_FIXTURE_NDEF_CHUNK_255_CARD_LEN sizeof(store_fixture_ndef_chunk_255_card)

#endif /* TESTS_FIXTURES_STORE_STORE_FIXTURE_H_ */
