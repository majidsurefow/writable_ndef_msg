/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader engine — Gate 1 scan + Gate 2 NDEF clone on nfc_stack_wq.
 */

#include "reader/nfc_reader_engine.h"

#include "hal/nfc_transport.h"
#include "run/nfc_stack_workq.h"

#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF)
#include "protocols/ndef/ndef_listener.h"
#include "protocols/ndef/ndef_poller.h"
#endif

#if IS_ENABLED(CONFIG_NFC_STORE)
#include "store/nfc_store.h"
#endif

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nfc_reader, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work scan_work;
static k_timeout_t scan_timeout;
static atomic_t scan_busy;
#if IS_ENABLED(CONFIG_NFC_STORE)
static atomic_t clone_busy;
static char s_clone_tag[CONFIG_NFC_STORE_MAX_TAG_LEN];
static struct k_work clone_work;
static atomic_t read_busy;
static char s_read_tag[CONFIG_NFC_STORE_MAX_TAG_LEN];
static k_timeout_t read_timeout;
static struct k_work read_work;
#endif

static nfc_reader_session_t s_session;

#if IS_ENABLED(CONFIG_NFC_STORE)
static bool s_store_ready;

static int nfc_reader_store_ensure(void)
{
	int ret;

	if (s_store_ready) {
		return 0;
	}

	ret = nfc_store_init(NULL);
	if (ret != 0 && ret != -EALREADY) {
		return ret;
	}

	s_store_ready = true;
	return 0;
}
#endif

static void nfc_reader_pollers_run(const char *tag)
{
#if IS_ENABLED(CONFIG_NFC_PROTOCOL_NDEF) && IS_ENABLED(CONFIG_NFC_STORE)
	const nfc_service_t *svcs[] = { ndef_listener_get() };
	ndef_data_t data;
	int ret;

	if (!s_session.active) {
		LOG_WRN("Clone skipped: no active reader session (scan first)");
		return;
	}

	ret = ndef_poller_detect(&s_session);
	if (ret != 0) {
		LOG_WRN("NDEF poller detect failed: %d", ret);
		goto end_session;
	}

	ndef_data_reset(&data);
	ret = ndef_poller_read(&s_session, &data);
	if (ret != 0) {
		LOG_WRN("NDEF poller read failed: %d", ret);
		goto end_session;
	}

	ret = ndef_listener_init(&data, NULL);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("ndef_listener_init failed: %d", ret);
		goto end_session;
	}

	ret = nfc_store_save(tag, svcs, ARRAY_SIZE(svcs));
	if (ret != 0) {
		LOG_ERR("nfc_store_save failed: %d", ret);
	} else {
		LOG_INF("Clone saved tag \"%s\"", tag);
	}

end_session:
	nfc_reader_session_end();
#else
	ARG_UNUSED(tag);
	LOG_WRN("Clone not built (enable CONFIG_NFC_PROTOCOL_NDEF + CONFIG_NFC_STORE)");
	if (s_session.active) {
		nfc_reader_session_end();
	}
#endif
}

#if IS_ENABLED(CONFIG_NFC_STORE)
static void clone_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	nfc_reader_pollers_run(s_clone_tag);
	atomic_clear(&clone_busy);
}
#endif

int nfc_reader_discover_once(k_timeout_t timeout, nfc_transport_tag_info_t *info)
{
	int ret;

	if (info == NULL) {
		return -EINVAL;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED ||
	    nfc_transport_get_state() == NFC_TRANSPORT_STATE_STOPPED) {
		ret = nfc_transport_init();
		if (ret != 0 && ret != -EALREADY) {
			return ret;
		}
	}

	ret = nfc_transport_discover_start(NFC_TECH_ALL_READER);
	if (ret != 0) {
		return ret;
	}

	ret = nfc_transport_discover_wait(info, timeout);
	if (ret != 0) {
		(void)nfc_transport_discover_stop();
		nfc_reader_session_clear(&s_session);
		return ret;
	}

	s_session.active = true;
	s_session.tag = *info;
	return 0;
}

static void nfc_reader_log_tag_info(const nfc_transport_tag_info_t *info)
{
	if (info == NULL) {
		return;
	}

	if (info->valid && info->uid.len > 0U) {
		LOG_INF("Tag UID (%u):", info->uid.len);
		for (uint8_t i = 0U; i < info->uid.len; i++) {
			LOG_INF("  %02x", info->uid.bytes[i]);
		}
	} else {
		LOG_INF("Tag detected (UID not reported by controller)");
	}

	LOG_INF("Protocol: 0x%02x  Interface: 0x%02x  ModeTech: 0x%02x", info->protocol,
		info->interface, info->mode_tech);
}

static void scan_work_handler(struct k_work *work)
{
	nfc_transport_tag_info_t info;
	int ret;

	ARG_UNUSED(work);

	ret = nfc_reader_discover_once(scan_timeout, &info);
	if (ret != 0) {
		LOG_WRN("Scan timeout or error: %d", ret);
		goto done;
	}

	nfc_reader_log_tag_info(&info);

done:
	atomic_clear(&scan_busy);
}

void nfc_reader_session_clear(nfc_reader_session_t *session)
{
	if (session == NULL) {
		return;
	}

	memset(session, 0, sizeof(*session));
}

const nfc_reader_session_t *nfc_reader_session_get(void)
{
	if (!s_session.active) {
		return NULL;
	}

	return &s_session;
}

void nfc_reader_session_end(void)
{
	if (!s_session.active) {
		return;
	}

	(void)nfc_transport_discover_stop();
	nfc_reader_session_clear(&s_session);
}

int nfc_reader_session_transceive(nfc_reader_session_t *session, const uint8_t *tx, size_t tx_len,
				  uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout)
{
	if (session == NULL || tx == NULL || rx == NULL || rx_len == NULL) {
		return -EINVAL;
	}

	if (!session->active) {
		return -ENODEV;
	}

	return nfc_transport_tag_transceive(tx, tx_len, rx, rx_max, rx_len, timeout);
}

bool nfc_reader_scan_busy(void)
{
	return atomic_get(&scan_busy) != 0;
}

int nfc_reader_scan_start(k_timeout_t timeout)
{
	int ret;

	if (nfc_reader_scan_busy() || nfc_reader_read_busy()) {
		return -EBUSY;
	}

	atomic_set(&scan_busy, 1);
	scan_timeout = timeout;
	k_work_init(&scan_work, scan_work_handler);
	ret = k_work_submit_to_queue(nfc_stack_wq_get(), &scan_work);
	if (ret < 0) {
		atomic_clear(&scan_busy);
		return ret;
	}

	return 0;
}

bool nfc_reader_clone_busy(void)
{
#if IS_ENABLED(CONFIG_NFC_STORE)
	return atomic_get(&clone_busy) != 0;
#else
	return false;
#endif
}

#if IS_ENABLED(CONFIG_NFC_STORE)
static void read_work_handler(struct k_work *work)
{
	nfc_transport_tag_info_t info;
	int ret;

	ARG_UNUSED(work);

	ret = nfc_reader_discover_once(read_timeout, &info);
	if (ret != 0) {
		LOG_WRN("Read scan failed: %d", ret);
		goto done;
	}

	nfc_reader_log_tag_info(&info);
	nfc_reader_pollers_run(s_read_tag);

done:
	atomic_clear(&read_busy);
}
#endif

int nfc_reader_read_start(const char *tag, k_timeout_t timeout)
{
#if IS_ENABLED(CONFIG_NFC_STORE)
	size_t len;
	int ret;

	ret = nfc_reader_store_ensure();
	if (ret != 0) {
		return ret;
	}

	if (tag == NULL) {
		return -EINVAL;
	}

	len = strlen(tag);
	if (len == 0U || len >= sizeof(s_read_tag)) {
		return -EINVAL;
	}

	if (nfc_reader_read_busy() || nfc_reader_scan_busy() || nfc_reader_clone_busy()) {
		return -EBUSY;
	}

	(void)strncpy(s_read_tag, tag, sizeof(s_read_tag) - 1U);
	s_read_tag[sizeof(s_read_tag) - 1U] = '\0';

	read_timeout = timeout;
	atomic_set(&read_busy, 1);
	k_work_init(&read_work, read_work_handler);
	ret = k_work_submit_to_queue(nfc_stack_wq_get(), &read_work);
	if (ret < 0) {
		atomic_clear(&read_busy);
		return ret;
	}

	return 0;
#else
	ARG_UNUSED(tag);
	ARG_UNUSED(timeout);
	return -ENOTSUP;
#endif
}

bool nfc_reader_read_busy(void)
{
#if IS_ENABLED(CONFIG_NFC_STORE)
	return atomic_get(&read_busy) != 0;
#else
	return false;
#endif
}

int nfc_reader_clone_start(const char *tag)
{
#if IS_ENABLED(CONFIG_NFC_STORE)
	size_t len;
	int ret;

	ret = nfc_reader_store_ensure();
	if (ret != 0) {
		return ret;
	}

	if (tag == NULL) {
		return -EINVAL;
	}

	len = strlen(tag);
	if (len == 0U || len >= sizeof(s_clone_tag)) {
		return -EINVAL;
	}

	if (nfc_reader_clone_busy() || nfc_reader_read_busy()) {
		return -EBUSY;
	}

	if (nfc_reader_session_get() == NULL) {
		return -ENODEV;
	}

	(void)strncpy(s_clone_tag, tag, sizeof(s_clone_tag) - 1U);
	s_clone_tag[sizeof(s_clone_tag) - 1U] = '\0';

	atomic_set(&clone_busy, 1);
	k_work_init(&clone_work, clone_work_handler);
	ret = k_work_submit_to_queue(nfc_stack_wq_get(), &clone_work);
	if (ret < 0) {
		atomic_clear(&clone_busy);
		return ret;
	}

	return 0;
#else
	ARG_UNUSED(tag);
	return -ENOTSUP;
#endif
}
