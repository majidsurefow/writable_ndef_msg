/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared inbound APDU net_buf pool (HAL listen path, CONVENTIONS §5).
 */

#ifndef NFC_APDU_POOL_H_
#define NFC_APDU_POOL_H_

#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the FIXED ISR-safe pool for listen-path APDU assembly.
 *
 * Sized by CONFIG_NFC_APDU_POOL_COUNT / CONFIG_NFC_APDU_BUF_SIZE (src/nfc/Kconfig).
 * Allocate with net_buf_alloc(nfc_apdu_pool_get(), K_NO_WAIT) in ISR context.
 */
struct net_buf_pool *nfc_apdu_pool_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_APDU_POOL_H_ */
