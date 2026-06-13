/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader engine — Gate 1 scan + session transceive delegate.
 */

#ifndef NFC_READER_ENGINE_H_
#define NFC_READER_ENGINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "hal/nfc_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Active-tag session for poller transceive (wraps HAL tag_transceive).
 *
 * Populated after a successful discover_wait; cleared on discover_stop.
 * Pollers use nfc_reader_session_transceive() — never nfc_transport_* directly.
 */
typedef struct {
	bool active;
	nfc_transport_tag_info_t tag;
} nfc_reader_session_t;

/** @brief Clear session (no active tag). Does not stop HAL discovery. */
void nfc_reader_session_clear(nfc_reader_session_t *session);

/** @brief Active session after successful scan/discover, or NULL. */
const nfc_reader_session_t *nfc_reader_session_get(void);

/** @brief Stop discovery and clear the active session (after poller work). */
void nfc_reader_session_end(void);

/**
 * @brief Exchange with the active tag via HAL delegate.
 *
 * @caller_sync — run on nfc_stack_wq while poll session is active.
 */
int nfc_reader_session_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				  uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout);

/**
 * @brief Start an asynchronous tag scan on nfc_stack_wq.
 *
 * Non-blocking. Initializes transport if needed, starts discovery, waits up
 * to @p timeout, logs UID/protocol, then leaves the poll session active for
 * nfc_reader_session_transceive() until nfc_reader_session_end().
 *
 * @param timeout Maximum wait for a tag notification.
 * @return 0 if work was queued, -EBUSY if a scan is already running,
 *         or negative errno from transport init.
 */
int nfc_reader_scan_start(k_timeout_t timeout);

/** @brief True while a scan work item is running or queued. */
bool nfc_reader_scan_busy(void);

/**
 * @brief Clone active tag to .card envelope via NDEF poller + nfc_store_save.
 *
 * Requires a prior successful scan (active session). Runs on nfc_stack_wq.
 *
 * @param tag Null-terminated slot name for the saved blob.
 * @return 0 if work was queued, -EBUSY if clone in progress, -ENODEV if no session,
 *         -EINVAL if tag invalid, or negative errno from store init.
 */
int nfc_reader_clone_start(const char *tag);

/** @brief True while a clone work item is running or queued. */
bool nfc_reader_clone_busy(void);

#ifdef __cplusplus
}
#endif

#endif /* NFC_READER_ENGINE_H_ */
