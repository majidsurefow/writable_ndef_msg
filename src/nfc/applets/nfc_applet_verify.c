/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * nfc verify — poll emulated tag + diff vs stored .card (Gate 4).
 */

#include "applets/nfc_applet.h"

#include "protocols/ndef/ndef_listener.h"
#include "protocols/ndef/ndef_poller.h"
#include "reader/nfc_reader_engine.h"
#include "run/nfc_stack_workq.h"

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
#include "nfc_stack/nfc_stack.h"
#endif

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(nfc_reader, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work verify_work;
static atomic_t verify_busy;
static atomic_t verify_result;
static char s_verify_slot[CONFIG_NFC_STORE_MAX_TAG_LEN];
static k_timeout_t verify_timeout;

static int nfc_applet_copy_listener_model(ndef_data_t *out)
{
	const nfc_service_t *svc = ndef_listener_get();
	uint8_t buf[CONFIG_NFC_STORE_BLOB_SIZE];
	size_t len = 0U;
	int ret;

	if ((svc == NULL) || (svc->serialize == NULL)) {
		return -ENODEV;
	}

	ret = svc->serialize(buf, sizeof(buf), &len, svc->user_ctx);
	if (ret != 0) {
		return ret;
	}

	return ndef_deserialize(out, buf, len);
}

static void verify_work_handler(struct k_work *work)
{
	const nfc_service_t *svcs[] = { ndef_listener_get() };
	nfc_transport_tag_info_t info;
	const nfc_reader_session_t *session;
	ndef_data_t expected;
	ndef_data_t actual;
	int ret;

	ARG_UNUSED(work);

#if IS_ENABLED(CONFIG_NFC_LISTEN_STACK)
	if (nfc_stack_get_state() == NFC_STACK_STATE_STARTED) {
		(void)nfc_stack_stop();
	}
#endif

	ret = nfc_store_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		atomic_set(&verify_result, ret);
		goto done;
	}

	ret = ndef_listener_init(NULL, NULL);
	if (ret != 0 && ret != -EALREADY) {
		atomic_set(&verify_result, ret);
		goto done;
	}

	ret = nfc_store_load(s_verify_slot, svcs, ARRAY_SIZE(svcs));
	if (ret != 0) {
		atomic_set(&verify_result, ret);
		goto done;
	}

	ret = nfc_applet_copy_listener_model(&expected);
	if (ret != 0) {
		atomic_set(&verify_result, ret);
		goto done;
	}

	ret = nfc_reader_discover_once(verify_timeout, &info);
	if (ret != 0) {
		atomic_set(&verify_result, ret);
		goto done;
	}

	session = nfc_reader_session_get();
	if (session == NULL) {
		atomic_set(&verify_result, -ENODEV);
		goto done;
	}

	ret = ndef_poller_detect(session);
	if (ret != 0) {
		nfc_reader_session_end();
		atomic_set(&verify_result, ret);
		goto done;
	}

	ndef_data_reset(&actual);
	ret = ndef_poller_read(session, &actual);
	nfc_reader_session_end();
	if (ret != 0) {
		atomic_set(&verify_result, ret);
		goto done;
	}

	ret = nfc_applet_verify_compare(&expected, &actual, NULL,
					(info.valid ? &info.uid : NULL));
	if (ret != 0) {
		LOG_WRN("Verify diff for \"%s\": %d", s_verify_slot, ret);
	}

	atomic_set(&verify_result, ret);

done:
	atomic_clear(&verify_busy);
}

int nfc_applet_verify_start(const char *slot, k_timeout_t timeout)
{
	size_t len;
	int ret;

	if (slot == NULL) {
		return -EINVAL;
	}

	len = strlen(slot);
	if (len == 0U || len >= sizeof(s_verify_slot)) {
		return -EINVAL;
	}

	if (nfc_applet_verify_busy() || nfc_applet_read_busy() || nfc_applet_scan_busy() ||
	    nfc_reader_clone_busy()) {
		return -EBUSY;
	}

	(void)strncpy(s_verify_slot, slot, sizeof(s_verify_slot) - 1U);
	s_verify_slot[sizeof(s_verify_slot) - 1U] = '\0';
	verify_timeout = timeout;
	atomic_set(&verify_result, -EINPROGRESS);
	atomic_set(&verify_busy, 1);
	k_work_init(&verify_work, verify_work_handler);
	ret = k_work_submit_to_queue(nfc_stack_wq_get(), &verify_work);
	if (ret < 0) {
		atomic_clear(&verify_busy);
		return ret;
	}

	return 0;
}

bool nfc_applet_verify_busy(void)
{
	return atomic_get(&verify_busy) != 0;
}

int nfc_applet_verify_wait(k_timeout_t deadline)
{
	int64_t end_ms = k_uptime_get() + (int64_t)k_ticks_to_ms_floor64(deadline.ticks);

	while (nfc_applet_verify_busy()) {
		if (k_uptime_get() >= end_ms) {
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(50));
	}

	return 0;
}

int nfc_applet_verify_last_result(void)
{
	return atomic_get(&verify_result);
}
