/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC applet service — Layer-1 headless API (LOCKED 2026-06-14, Phase A).
 *
 * This is the canonical headless entry point for the product applets
 * (scan / read / emulate / loop / check). It contains NO `struct shell`:
 * every function returns errno and/or fills a result struct, and long-running
 * orchestration emits progress through an optional caller-supplied log
 * callback. The shell adapter (`nfc_applet_shell_cmds.c`, Layer-2) is the only
 * place allowed to render these results to a `struct shell *`.
 *
 * Layer map (LOCKED):
 *   L0 engines  : reader engine, nfc_stack, store, HAL   (no shell)
 *   L1 applets  : this header + nfc_applet_*.c            (no shell)  ◀── here
 *   L2 shell    : *_shell_cmds.c                          (struct shell* only)
 *   L3 app/SMF  : future consumer of L1
 */

#ifndef NFC_APPLETS_NFC_APPLET_SERVICE_H_
#define NFC_APPLETS_NFC_APPLET_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "hal/nfc_transport.h"
#include "protocols/ndef/ndef.h"

/* nfc_profile_t is needed by the emulate API in all builds; nfc_applet_emulate()
 * is always defined (returns -ENOTSUP when CONFIG_NFC_LISTEN_STACK is off).
 */
#include "nfc_stack/nfc_stack.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Scan applet — continuous discovery + per-tag callback              */
/* ------------------------------------------------------------------ */

/**
 * @brief Per-tag discovery callback (Layer-1, headless).
 *
 * Invoked on the nfc_stack work queue for every tag the continuous discovery
 * loop activates. @p info is owned by the engine and valid only for the
 * duration of the call; copy out anything you need to retain.
 */
typedef void (*nfc_applet_tag_cb_t)(const nfc_transport_tag_info_t *info, void *ctx);

/** Result of a single discovery (headless mirror of the active session). */
typedef struct {
	nfc_uid_t uid;
	uint8_t protocol;
	uint8_t interface;
	uint8_t mode_tech;
	bool valid;
} nfc_applet_scan_result_t;

/**
 * @brief Start continuous tag discovery; invoke @p cb on each detected tag.
 *
 * Non-blocking. Runs a self-rescheduling work item on nfc_stack_wq that
 * discovers, fires @p cb, releases the session, and repeats until
 * nfc_applet_discover_stop(). Replaces the deprecated one-shot blocking scan.
 *
 * @param cb  Per-tag callback (NULL = discover silently).
 * @param ctx Opaque context passed back to @p cb.
 * @return 0 on success, -EBUSY if discovery is already active.
 */
int nfc_applet_discover_start(nfc_applet_tag_cb_t cb, void *ctx);

/** @brief Request the continuous discovery loop to stop and release session. */
int nfc_applet_discover_stop(void);

/** @brief True while continuous discovery is running. */
bool nfc_applet_discover_active(void);

/** @brief True while any scan/discovery (one-shot DK or continuous) is busy. */
bool nfc_applet_scan_busy(void);

/**
 * @brief Snapshot the active poll session into a result struct (no rendering).
 *
 * Replaces the old shell-typed nfc_applet_scan_print().
 *
 * @return 0 on success, -ENOENT if no session is active, -EINVAL on NULL out.
 */
int nfc_applet_scan_get_result(nfc_applet_scan_result_t *out);

/* ------------------------------------------------------------------ */
/* Read applet — one-shot scan + poller clone → slot                  */
/* ------------------------------------------------------------------ */

int nfc_applet_read_start(const char *slot, k_timeout_t timeout);
bool nfc_applet_read_busy(void);
int nfc_applet_read_wait(k_timeout_t deadline);

/* ------------------------------------------------------------------ */
/* Shared slot metadata (headless)                                    */
/* ------------------------------------------------------------------ */

typedef struct {
	uint8_t persist_id;        /* NFC_PERSIST_ID_*        */
	uint8_t flags;             /* NFC_STORE_ENTRY_FLAG_*  */
	const char *protocol_name; /* nfc_persist_id_name(id) */
	bool present;              /* peek_entry_meta succeeded */
} nfc_applet_card_meta_t;

/**
 * @brief Headless slot metadata getter (peek_entry_meta + persist name).
 *
 * Lets the shell adapter avoid reaching into store naming directly.
 *
 * @return 0 if the slot is present, negative errno otherwise (out->present=false).
 */
int nfc_applet_get_card_meta(const char *slot, nfc_applet_card_meta_t *out);

/* ------------------------------------------------------------------ */
/* Emulate applet                                                     */
/* ------------------------------------------------------------------ */

int nfc_applet_emulate(const char *slot, nfc_profile_t profile);

/* ------------------------------------------------------------------ */
/* Check applet (field diff vs store) — internal to loop + DK shell   */
/* ------------------------------------------------------------------ */

int nfc_applet_verify_start(const char *slot, k_timeout_t timeout);
bool nfc_applet_verify_busy(void);
int nfc_applet_verify_wait(k_timeout_t deadline);
/** @return 0 on PASS, negative errno on FAIL or error. */
int nfc_applet_verify_last_result(void);

/**
 * @brief Compare stored vs polled NDEF models (Tier E + check core).
 *
 * UID pointers may be NULL to skip UID check (store envelope has no UID v1).
 */
int nfc_applet_verify_compare(const ndef_data_t *expected, const ndef_data_t *actual,
			      const nfc_uid_t *expected_uid, const nfc_uid_t *actual_uid);

/* ------------------------------------------------------------------ */
/* Loop applet — read → emulate → check orchestration                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Optional progress callback for long-running orchestration.
 *
 * @param ctx   Caller context.
 * @param stage Stage label ("read"/"emulate"/"verify"); NULL-terminated literal.
 * @param code  0 = entering stage, negative errno = stage failed.
 *
 * NULL callback = silent (used by headless tests and UIs that poll the result).
 */
typedef void (*nfc_applet_log_fn)(void *ctx, const char *stage, int code);

typedef enum {
	NFC_APPLET_LOOP_STAGE_READ = 0,
	NFC_APPLET_LOOP_STAGE_EMULATE,
	NFC_APPLET_LOOP_STAGE_VERIFY,
	NFC_APPLET_LOOP_STAGE_DONE,
} nfc_applet_loop_stage_t;

typedef struct {
	nfc_applet_loop_stage_t failed_stage; /* last stage reached */
	int verify_result;                    /* 0 = PASS           */
} nfc_applet_loop_result_t;

/**
 * @brief Orchestrate read → emulate → check on one slot (headless).
 *
 * Replaces the old nfc_applet_loop(struct shell *, ...). Emits stage progress
 * via @p log (silent when NULL) and fills @p out with the failed stage and the
 * final check result.
 *
 * @return 0 on PASS, negative errno on the first failing stage / FAIL.
 */
int nfc_applet_loop_run(const char *slot, k_timeout_t timeout,
			nfc_applet_log_fn log, void *ctx,
			nfc_applet_loop_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* NFC_APPLETS_NFC_APPLET_SERVICE_H_ */
