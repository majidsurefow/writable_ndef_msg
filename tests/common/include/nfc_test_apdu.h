/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Build nfc_apdu_t views from short C-APDU byte arrays (cookbook §14.5).
 */

#ifndef TESTS_COMMON_NFC_TEST_APDU_H_
#define TESTS_COMMON_NFC_TEST_APDU_H_

#include <stddef.h>
#include <stdint.h>

#include "framing/apdu_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse a short (non-extended) C-APDU into @p out.
 *
 * @p storage holds a copy of the command data field; @p out->data points into
 * @p storage when lc > 0. @p storage_cap must be >= apdu_len.
 *
 * @return 0 on success, -EINVAL on malformed input.
 */
int nfc_test_apdu_from_bytes(const uint8_t *apdu, size_t apdu_len, uint8_t *storage,
			     size_t storage_cap, nfc_apdu_t *out);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_TEST_APDU_H_ */
