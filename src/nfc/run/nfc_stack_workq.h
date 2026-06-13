/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared NFC stack work queue — poll blocking, listen fifo drain, framing.
 */

#ifndef NFC_STACK_WORKQ_H_
#define NFC_STACK_WORKQ_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the NFC stack work queue (lazy-started on first access).
 *
 * Used by HAL poll/listen paths and the reader engine. Priority and stack
 * size come from Kconfig (NFC_STACK_WORKQ_*).
 *
 * @return Pointer to the started work queue; never NULL after first call.
 */
struct k_work_q *nfc_stack_wq_get(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_STACK_WORKQ_H_ */
