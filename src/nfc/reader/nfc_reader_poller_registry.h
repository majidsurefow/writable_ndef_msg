/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reader poller registry — Flipper nfc_pollers_api[] pattern (cookbook §5 / §14.11).
 */

#ifndef NFC_READER_POLLER_REGISTRY_H_
#define NFC_READER_POLLER_REGISTRY_H_

#include <stddef.h>
#include <stdint.h>

#include "reader/nfc_reader_engine.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*nfc_reader_poller_detect_fn)(const nfc_reader_session_t *session);
typedef int (*nfc_reader_poller_read_fn)(const nfc_reader_session_t *session, void *out);
typedef int (*nfc_reader_poller_clone_fn)(const nfc_reader_session_t *session,
					  const char *tag);

/**
 * @brief One reader poller slot (NULL-terminated table).
 *
 * @p detect returns 0 when this poller owns the active tag, -ENOTSUP to try next.
 * @p read fills protocol-specific @p out (caller supplies correct buffer type).
 * @p listener_get supplies the nfc_service_t saved after a successful read (may be NULL).
 * @p clone_fn read + listener bind + nfc_store_save for @p tag (NULL = stub).
 */
typedef struct {
	const char *name;
	uint8_t persist_id;
	uint8_t tech_mask;
	nfc_reader_poller_detect_fn detect;
	nfc_reader_poller_read_fn read;
	const nfc_service_t *(*listener_get)(void);
	nfc_reader_poller_clone_fn clone_fn;
} nfc_reader_poller_entry_t;

/** @brief NULL-terminated poller walk table (NDEF first). */
const nfc_reader_poller_entry_t *nfc_reader_pollers_get(void);

/**
 * @brief Detect + read + listener bind + nfc_store_save for @p tag.
 *
 * Walks @ref nfc_reader_pollers_get until a poller detect succeeds.
 *
 * @return 0 on saved clone; negative errno if no poller matched or I/O failed.
 */
int nfc_reader_pollers_run(const char *tag);

#ifdef __cplusplus
}
#endif

#endif /* NFC_READER_POLLER_REGISTRY_H_ */
