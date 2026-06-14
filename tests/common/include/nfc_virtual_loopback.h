/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * Virtual RF loopback harness — load → emulate → poller read → save → compare.
 */

#ifndef TESTS_COMMON_NFC_VIRTUAL_LOOPBACK_H_
#define TESTS_COMMON_NFC_VIRTUAL_LOOPBACK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "reader/nfc_reader_engine.h"
#include "router/service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NFC_VIRTUAL_LOOPBACK_MAX_BLOB 2048U

typedef int (*nfc_virtual_loopback_poller_detect_fn)(nfc_reader_session_t *session);
typedef int (*nfc_virtual_loopback_poller_read_fn)(nfc_reader_session_t *session,
						   void *read_out);
typedef int (*nfc_virtual_loopback_listener_setup_fn)(void *user_ctx);
typedef void (*nfc_virtual_loopback_listener_teardown_fn)(void *user_ctx);
typedef int (*nfc_virtual_loopback_copy_read_fn)(void *save_model, const void *read_out,
						 void *user_ctx);
typedef int (*nfc_virtual_loopback_compare_fn)(const void *expected, const void *actual,
					       void *user_ctx);

typedef struct {
	const nfc_service_t *listener_svc;
	const nfc_service_t *save_svc;
	const uint8_t *golden_blob;
	size_t golden_blob_len;
	const char *golden_slot;
	const char *output_slot;
	nfc_virtual_loopback_poller_detect_fn poller_detect;
	nfc_virtual_loopback_poller_read_fn poller_read;
	void *read_out;
	nfc_virtual_loopback_listener_setup_fn listener_setup;
	nfc_virtual_loopback_listener_teardown_fn listener_teardown;
	void *listener_user_ctx;
	void *save_model;
	nfc_virtual_loopback_copy_read_fn copy_read_to_save;
	void *copy_user_ctx;
	const void *expected;
	nfc_virtual_loopback_compare_fn compare;
	void *compare_user_ctx;
	bool verify_envelope;
} nfc_virtual_loopback_params_t;

/** @brief Init nfc_store with in-memory save/load callbacks (suite setup). */
int nfc_virtual_loopback_init(void);

/** @brief Reset session mock, virtual RF, and staged blob (per-test setup). */
void nfc_virtual_loopback_reset(void);

/**
 * @brief Run full virtual loopback chain.
 *
 * @return 0 on pass; negative errno on failure.
 */
int nfc_virtual_loopback_run(const nfc_virtual_loopback_params_t *params);

/** @brief Last blob written by nfc_store_save during loopback_run. */
int nfc_virtual_loopback_last_saved(const uint8_t **blob, size_t *len);

#ifdef __cplusplus
}
#endif

#endif /* TESTS_COMMON_NFC_VIRTUAL_LOOPBACK_H_ */
