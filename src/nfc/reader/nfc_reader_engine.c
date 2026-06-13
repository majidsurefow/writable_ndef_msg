/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader engine — Gate 1 scan on nfc_stack_wq.
 */

#include "reader/nfc_reader_engine.h"

#include "hal/nfc_transport.h"
#include "run/nfc_stack_workq.h"

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_reader, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work scan_work;
static k_timeout_t scan_timeout;
static atomic_t scan_busy;

static nfc_reader_session_t s_session;

static void scan_work_handler(struct k_work *work)
{
	nfc_transport_tag_info_t info;
	int ret;

	ARG_UNUSED(work);

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED ||
	    nfc_transport_get_state() == NFC_TRANSPORT_STATE_STOPPED) {
		ret = nfc_transport_init();
		if (ret != 0 && ret != -EALREADY) {
			LOG_ERR("Transport init failed: %d", ret);
			goto done;
		}
	}

	ret = nfc_transport_discover_start(NFC_TECH_ALL_READER);
	if (ret != 0) {
		LOG_ERR("Discovery start failed: %d", ret);
		goto done;
	}

	ret = nfc_transport_discover_wait(&info, scan_timeout);
	if (ret != 0) {
		LOG_WRN("Scan timeout or error: %d", ret);
		(void)nfc_transport_discover_stop();
		nfc_reader_session_clear(&s_session);
		goto done;
	}

	s_session.active = true;
	s_session.tag = info;

	if (info.valid && info.uid.len > 0U) {
		LOG_INF("Tag UID (%u):", info.uid.len);
		for (uint8_t i = 0U; i < info.uid.len; i++) {
			LOG_INF("  %02x", info.uid.bytes[i]);
		}
	} else {
		LOG_INF("Tag detected (UID not reported by controller)");
	}

	LOG_INF("Protocol: 0x%02x  Interface: 0x%02x  ModeTech: 0x%02x", info.protocol,
		info.interface, info.mode_tech);

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

	if (nfc_reader_scan_busy()) {
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
