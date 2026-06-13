/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NFC reader engine — async discovery scan (Gate 0).
 */

#include "reader/nfc_reader_engine.h"

#include "hal/nfc_transport.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_reader, CONFIG_LOG_DEFAULT_LEVEL);

static K_THREAD_STACK_DEFINE(reader_wq_stack, CONFIG_NFC_READER_WORKQ_STACK_SIZE);
static struct k_work_q reader_wq;
static struct k_work scan_work;
static k_timeout_t scan_timeout;
static atomic_t scan_busy;

static void reader_wq_init_once(void)
{
	static bool started;

	if (started) {
		return;
	}

	k_work_queue_init(&reader_wq);
	k_work_queue_start(&reader_wq, reader_wq_stack,
			   K_THREAD_STACK_SIZEOF(reader_wq_stack),
			   CONFIG_NFC_READER_WORKQ_PRIORITY, NULL);
	started = true;
}

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
		goto done;
	}

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

	reader_wq_init_once();

	atomic_set(&scan_busy, 1);
	scan_timeout = timeout;
	k_work_init(&scan_work, scan_work_handler);
	ret = k_work_submit_to_queue(&reader_wq, &scan_work);
	if (ret < 0) {
		atomic_clear(&scan_busy);
		return ret;
	}

	return 0;
}
