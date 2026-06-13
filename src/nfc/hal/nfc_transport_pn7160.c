/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 SMART-tier nfc_transport backend (Gate 1).
 */

#include "hal/nfc_transport.h"
#include "run/nfc_stack_workq.h"

#include "../../stats.h"

#include <errno.h>
#include <string.h>

#include <nfc/pn7160.h>
#include <nfc/pn7160_tml.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nfc_transport_pn7160, CONFIG_LOG_DEFAULT_LEVEL);

BUILD_ASSERT(NFC_TRANSPORT_MAX_RESPONSE_LEN == PN7160_TML_MAX_PAYLOAD_LEN);

#define NFC_SESSION_NONE   0
#define NFC_SESSION_POLL   1
#define NFC_SESSION_LISTEN 2

static atomic_t s_state = ATOMIC_INIT(NFC_TRANSPORT_STATE_UNINITIALIZED);
static atomic_t s_session = ATOMIC_INIT(NFC_SESSION_NONE);
static const struct device *s_dev;

static nfc_transport_config_t s_config = {
	.fwi = NFC_TRANSPORT_DEFAULT_FWI,
};
static nfc_transport_stats_t s_stats;
static struct k_spinlock s_stats_lock;

static nfc_transport_ops_t s_ops;
static void *s_ops_user_ctx;
static bool s_ops_registered;

static nfc_uid_t s_pending_uid;

static struct k_work s_listen_recv_work;
static atomic_t s_listen_recv_stop;

static const nfc_transport_caps_t s_caps = {
	.roles = NFC_ROLE_READER | NFC_ROLE_LISTEN,
	.technologies = NFC_TECH_ALL_READER,
	.tier = NFC_TIER_SMART,
};

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_pn7160)
#define PN7160_TRANSPORT_DEV DEVICE_DT_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pn7160))
#else
#define PN7160_TRANSPORT_DEV NULL
#endif

static void tag_info_from_rf(const struct pn7160_nci_rf_intf *rf, nfc_transport_tag_info_t *info)
{
	memset(info, 0, sizeof(*info));
	info->protocol = rf->protocol;
	info->interface = rf->interface;
	info->mode_tech = rf->mode_tech;

	if (rf->uid_len > 0U) {
		info->uid.len = rf->uid_len;
		memcpy(info->uid.bytes, rf->uid, rf->uid_len);
		info->valid = true;
	}
}

static bool uid_len_valid(uint8_t len)
{
	return len == NFC_UID_LEN_SINGLE || len == NFC_UID_LEN_DOUBLE || len == NFC_UID_LEN_TRIPLE;
}

static void listen_recv_work_handler(struct k_work *work)
{
	uint8_t data[NFC_TRANSPORT_MAX_RESPONSE_LEN];
	size_t data_len;
	int ret;

	ARG_UNUSED(work);

	while (atomic_get(&s_listen_recv_stop) == 0 &&
	       atomic_get(&s_session) == NFC_SESSION_LISTEN) {
		ret = pn7160_nci_card_mode_recv(s_dev, data, sizeof(data), &data_len,
						K_MSEC(500));
		if (ret == -ETIMEDOUT) {
			continue;
		}
		if (ret != 0) {
			STATS_ERROR(&s_stats_lock, s_stats, ret);
			break;
		}

		STATS_INC(&s_stats_lock, s_stats, fragment_rx_count);

		/*
		 * Gate 1 skeleton: APDU up-dispatch via on_apdu deferred to Gate 3
		 * (net_buf pool + apdu_assembler).
		 */
		if (!s_ops_registered || s_ops.on_apdu == NULL) {
			STATS_INC(&s_stats_lock, s_stats, apdu_dropped_no_consumer);
		}
	}
}

const nfc_transport_caps_t *nfc_transport_get_capabilities(void)
{
	return &s_caps;
}

const nfc_transport_config_t *nfc_transport_get_config(void)
{
	return &s_config;
}

int nfc_transport_get_stats(nfc_transport_stats_t *out)
{
	return STATS_COPY_OUT(&s_stats_lock, s_stats, out);
}

nfc_transport_state_t nfc_transport_get_state(void)
{
	return (nfc_transport_state_t)atomic_get(&s_state);
}

int nfc_transport_init(void)
{
	nfc_transport_state_t st = nfc_transport_get_state();
	int ret;

	if (st != NFC_TRANSPORT_STATE_UNINITIALIZED && st != NFC_TRANSPORT_STATE_STOPPED) {
		return -EALREADY;
	}

	STATS_RESET(s_stats);

#if !DT_HAS_COMPAT_STATUS_OKAY(nxp_pn7160)
	return -ENODEV;
#else
	s_dev = PN7160_TRANSPORT_DEV;
	if (!device_is_ready(s_dev)) {
		LOG_ERR("PN7160 device not ready");
		STATS_ERROR(&s_stats_lock, s_stats, -ENODEV);
		return -ENODEV;
	}

	ret = pn7160_nci_connect(s_dev);
	if (ret != 0) {
		LOG_ERR("NCI connect failed: %d", ret);
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	ret = pn7160_nci_configure_settings(s_dev);
	if (ret != 0) {
		LOG_ERR("NCI settings failed: %d", ret);
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_INITIALIZED);
	return 0;
#endif
}

int nfc_transport_shutdown(void)
{
	if (atomic_get(&s_session) == NFC_SESSION_POLL) {
		(void)nfc_transport_discover_stop();
	} else if (atomic_get(&s_session) == NFC_SESSION_LISTEN) {
		(void)nfc_transport_stop();
	}

	atomic_clear(&s_listen_recv_stop);
	(void)k_work_cancel_sync(&s_listen_recv_work, NULL);

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_UNINITIALIZED);
	s_dev = NULL;
	s_ops_registered = false;
	memset(&s_ops, 0, sizeof(s_ops));
	s_ops_user_ctx = NULL;

	return 0;
}

int nfc_transport_register_callbacks(const nfc_transport_ops_t *ops, void *user_ctx)
{
	nfc_transport_state_t st = nfc_transport_get_state();

	if (st == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (st == NFC_TRANSPORT_STATE_STARTED) {
		return -EBUSY;
	}

	if (ops == NULL) {
		memset(&s_ops, 0, sizeof(s_ops));
		s_ops_user_ctx = NULL;
		s_ops_registered = false;
		return 0;
	}

	s_ops = *ops;
	s_ops_user_ctx = user_ctx;
	s_ops_registered = true;
	return 0;
}

int nfc_transport_discover_start(nfc_tech_t tech_mask)
{
	int ret;

	ARG_UNUSED(tech_mask);

	if (s_dev == NULL || nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_NONE) {
		return -EBUSY;
	}

	ret = pn7160_nci_discovery_start(s_dev, NULL, 0U);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	STATS_INC(&s_stats_lock, s_stats, discover_start_count);
	atomic_set(&s_session, NFC_SESSION_POLL);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_STARTED);
	return 0;
}

int nfc_transport_discover_stop(void)
{
	int ret = 0;

	if (s_dev == NULL) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_POLL) {
		return 0;
	}

	ret = pn7160_nci_discovery_stop(s_dev);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
	}

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_STOPPED);
	return ret;
}

int nfc_transport_discover_wait(nfc_transport_tag_info_t *info, k_timeout_t timeout)
{
	struct pn7160_nci_rf_intf rf;
	int ret;

	if (info == NULL) {
		return -EINVAL;
	}

	if (s_dev == NULL || atomic_get(&s_session) != NFC_SESSION_POLL) {
		return -ENODEV;
	}

	ret = pn7160_nci_discovery_wait(s_dev, &rf, timeout);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	tag_info_from_rf(&rf, info);
	STATS_INC(&s_stats_lock, s_stats, tag_detect_count);
	return 0;
}

int nfc_transport_tag_transceive(const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_max,
				 size_t *rx_len, k_timeout_t timeout)
{
	int ret;

	if (s_dev == NULL || atomic_get(&s_session) != NFC_SESSION_POLL) {
		return -ENODEV;
	}

	ret = pn7160_nci_reader_tag_cmd(s_dev, tx, tx_len, rx, rx_max, rx_len, timeout);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	STATS_INC(&s_stats_lock, s_stats, transceive_count);
	return 0;
}

int nfc_transport_start(const nfc_uid_t *uid)
{
	int ret;

	if (s_dev == NULL || nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_NONE) {
		return -EBUSY;
	}

	if (uid != NULL) {
		if (!uid_len_valid(uid->len)) {
			return -EINVAL;
		}
		s_pending_uid = *uid;
	}

	ret = pn7160_nci_listen_start(s_dev, NULL, 0U);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	atomic_clear(&s_listen_recv_stop);
	k_work_init(&s_listen_recv_work, listen_recv_work_handler);
	(void)k_work_submit_to_queue(nfc_stack_wq_get(), &s_listen_recv_work);

	if (s_ops_registered && s_ops.on_field_on != NULL) {
		s_ops.on_field_on(s_ops_user_ctx);
		STATS_INC(&s_stats_lock, s_stats, field_on_count);
	}

	atomic_set(&s_session, NFC_SESSION_LISTEN);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_STARTED);
	return 0;
}

int nfc_transport_stop(void)
{
	int ret = 0;

	if (s_dev == NULL) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) != NFC_SESSION_LISTEN) {
		return 0;
	}

	atomic_set(&s_listen_recv_stop, 1);
	(void)k_work_cancel_sync(&s_listen_recv_work, NULL);

	ret = pn7160_nci_listen_stop(s_dev);
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
	}

	if (s_ops_registered && s_ops.on_field_off != NULL) {
		s_ops.on_field_off(s_ops_user_ctx);
		STATS_INC(&s_stats_lock, s_stats, field_off_count);
	}

	atomic_set(&s_session, NFC_SESSION_NONE);
	atomic_set(&s_state, NFC_TRANSPORT_STATE_STOPPED);
	return ret;
}

int nfc_transport_set_uid(const nfc_uid_t *uid)
{
	if (uid == NULL) {
		return -EINVAL;
	}

	if (!uid_len_valid(uid->len)) {
		return -EINVAL;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (atomic_get(&s_session) == NFC_SESSION_LISTEN) {
		return -EBUSY;
	}

	s_pending_uid = *uid;
	STATS_INC(&s_stats_lock, s_stats, uid_rotation_count);
	return 0;
}

int nfc_transport_send_response(const uint8_t *buf, size_t len)
{
	int ret;

	if (buf == NULL || len == 0U || len > NFC_TRANSPORT_MAX_RESPONSE_LEN) {
		return -EINVAL;
	}

	if (atomic_get(&s_session) != NFC_SESSION_LISTEN) {
		return -ENODEV;
	}

	ret = pn7160_nci_card_mode_send(s_dev, buf, len, K_SECONDS(2));
	if (ret != 0) {
		STATS_ERROR(&s_stats_lock, s_stats, ret);
		return ret;
	}

	STATS_INC(&s_stats_lock, s_stats, response_tx_count);
	return 0;
}

int nfc_transport_submit_work(struct k_work *work)
{
	if (work == NULL) {
		return -EINVAL;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	(void)k_work_submit_to_queue(nfc_stack_wq_get(), work);
	return 0;
}
