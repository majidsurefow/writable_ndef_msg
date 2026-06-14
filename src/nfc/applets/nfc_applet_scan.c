/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc scan — continuous, headless tag discovery (Flipper: NFC → Detect Reader).
 *
 * Layer-1: NO struct shell. The continuous discovery loop runs a
 * self-rescheduling work item on nfc_stack_wq, invoking a caller-supplied
 * callback for each detected tag. The shell adapter registers a callback that
 * renders UID/protocol/tech; a UI or test passes its own (or NULL).
 */

#include "applets/nfc_applet_service.h"

#include "reader/nfc_reader_engine.h"
#include "run/nfc_stack_workq.h"

#include <errno.h>

#include <zephyr/kernel.h>

/* Per-iteration discovery window; the loop re-arms until stop is requested. */
#define NFC_APPLET_DISCOVER_SLICE K_MSEC(2000)

static struct k_work s_discover_work;
static atomic_t s_discover_run;
static nfc_applet_tag_cb_t s_tag_cb;
static void *s_tag_ctx;

static void nfc_applet_discover_handler(struct k_work *work)
{
	nfc_transport_tag_info_t info;
	int ret;

	if (atomic_get(&s_discover_run) == 0) {
		return;
	}

	ret = nfc_reader_discover_once(NFC_APPLET_DISCOVER_SLICE, &info);
	if (ret == 0) {
		if (s_tag_cb != NULL) {
			s_tag_cb(&info, s_tag_ctx);
		}
		/* Release the field so the next slice can re-discover. */
		nfc_reader_session_end();
	}

	/* Re-arm unless a stop was requested while we were blocked. */
	if (atomic_get(&s_discover_run) != 0) {
		(void)k_work_submit_to_queue(nfc_stack_wq_get(), work);
	}
}

int nfc_applet_discover_start(nfc_applet_tag_cb_t cb, void *ctx)
{
	if (atomic_get(&s_discover_run) != 0) {
		return -EBUSY;
	}

	if (nfc_reader_scan_busy()) {
		return -EBUSY;
	}

	s_tag_cb = cb;
	s_tag_ctx = ctx;
	atomic_set(&s_discover_run, 1);
	k_work_init(&s_discover_work, nfc_applet_discover_handler);

	int ret = k_work_submit_to_queue(nfc_stack_wq_get(), &s_discover_work);

	if (ret < 0) {
		atomic_clear(&s_discover_run);
		return ret;
	}

	return 0;
}

int nfc_applet_discover_stop(void)
{
	if (atomic_get(&s_discover_run) == 0) {
		return -EALREADY;
	}

	atomic_clear(&s_discover_run);
	/* Best-effort: drop any session the loop left active between slices. */
	nfc_reader_session_end();
	return 0;
}

bool nfc_applet_discover_active(void)
{
	return atomic_get(&s_discover_run) != 0;
}

bool nfc_applet_scan_busy(void)
{
	return nfc_reader_scan_busy() || nfc_applet_discover_active();
}

int nfc_applet_scan_get_result(nfc_applet_scan_result_t *out)
{
	const nfc_reader_session_t *session = nfc_reader_session_get();

	if (out == NULL) {
		return -EINVAL;
	}

	if (session == NULL) {
		return -ENOENT;
	}

	out->uid = session->tag.uid;
	out->protocol = session->tag.protocol;
	out->interface = session->tag.interface;
	out->mode_tech = session->tag.mode_tech;
	out->valid = session->tag.valid;
	return 0;
}
