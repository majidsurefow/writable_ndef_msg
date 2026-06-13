/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader engine — Gate 0 scan-only.
 */

#ifndef NFC_READER_ENGINE_H_
#define NFC_READER_ENGINE_H_

#include <stdbool.h>

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start an asynchronous tag scan on the reader work queue.
 *
 * Non-blocking. Initializes transport if needed, starts discovery, waits up
 * to @p timeout, logs UID/protocol, then stops discovery.
 *
 * @param timeout Maximum wait for a tag notification.
 * @return 0 if work was queued, -EBUSY if a scan is already running,
 *         or negative errno from transport init.
 */
int nfc_reader_scan_start(k_timeout_t timeout);

/** @brief True while a scan work item is running or queued. */
bool nfc_reader_scan_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_READER_ENGINE_H_ */
