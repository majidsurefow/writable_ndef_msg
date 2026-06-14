/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc loop — headless read → emulate → check orchestration (Gate 4 sign-off).
 *
 * Layer-1: NO struct shell. Progress is emitted through the optional
 * nfc_applet_log_fn callback; the shell adapter (cmd_nfc_loop) supplies one
 * that prints, a UI supplies one that drives a state machine, a test passes
 * NULL and asserts on the result struct.
 */

#include "applets/nfc_applet_service.h"

#include "store/nfc_store.h"

#include <errno.h>

static void nfc_applet_loop_emit(nfc_applet_log_fn log, void *ctx, const char *stage, int code)
{
	if (log != NULL) {
		log(ctx, stage, code);
	}
}

int nfc_applet_loop_run(const char *slot, k_timeout_t timeout,
			nfc_applet_log_fn log, void *ctx,
			nfc_applet_loop_result_t *out)
{
	k_timeout_t wait_deadline = K_MSEC((int32_t)k_ticks_to_ms_floor64(timeout.ticks) + 5000);
	int ret;
	int verify_ret;

	if ((slot == NULL) || (out == NULL)) {
		return -EINVAL;
	}

	out->failed_stage = NFC_APPLET_LOOP_STAGE_READ;
	out->verify_result = -EINPROGRESS;

#if IS_ENABLED(CONFIG_NFC_STORE)
	(void)nfc_store_init(NULL);
#endif

	nfc_applet_loop_emit(log, ctx, "read", 0);
	ret = nfc_applet_read_start(slot, timeout);
	if (ret != 0) {
		nfc_applet_loop_emit(log, ctx, "read", ret);
		return ret;
	}

	ret = nfc_applet_read_wait(wait_deadline);
	if (ret != 0) {
		nfc_applet_loop_emit(log, ctx, "read", ret);
		return ret;
	}

	out->failed_stage = NFC_APPLET_LOOP_STAGE_EMULATE;
	nfc_applet_loop_emit(log, ctx, "emulate", 0);
	ret = nfc_applet_emulate(slot, NFC_PROFILE_NDEF);
	if (ret != 0) {
		nfc_applet_loop_emit(log, ctx, "emulate", ret);
		return ret;
	}

	out->failed_stage = NFC_APPLET_LOOP_STAGE_VERIFY;
	nfc_applet_loop_emit(log, ctx, "verify", 0);
	ret = nfc_applet_verify_start(slot, timeout);
	if (ret != 0) {
		nfc_applet_loop_emit(log, ctx, "verify", ret);
		return ret;
	}

	ret = nfc_applet_verify_wait(wait_deadline);
	if (ret != 0) {
		nfc_applet_loop_emit(log, ctx, "verify", ret);
		return ret;
	}

	verify_ret = nfc_applet_verify_last_result();
	out->verify_result = verify_ret;
	out->failed_stage = NFC_APPLET_LOOP_STAGE_DONE;
	if (verify_ret != 0) {
		nfc_applet_loop_emit(log, ctx, "verify", verify_ret);
	}

	return verify_ret;
}
