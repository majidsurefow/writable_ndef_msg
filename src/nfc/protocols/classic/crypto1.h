/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Crypto1 scaffold — port from flipperzero/lib/nfc/helpers/crypto1.h (F2).
 */

#ifndef NFC_PROTOCOLS_CLASSIC_CRYPTO1_H_
#define NFC_PROTOCOLS_CLASSIC_CRYPTO1_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct crypto1 {
	uint32_t odd;
	uint32_t even;
};

#ifdef __cplusplus
}
#endif

#endif /* NFC_PROTOCOLS_CLASSIC_CRYPTO1_H_ */
