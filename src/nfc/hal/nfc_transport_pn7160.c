/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 SMART-tier nfc_transport backend (Gate 0).
 */

#include "hal/nfc_transport.h"

#include <errno.h>
#include <string.h>

#include <nfc/pn7160.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nfc_transport_pn7160, CONFIG_LOG_DEFAULT_LEVEL);

static atomic_t s_state = ATOMIC_INIT(NFC_TRANSPORT_STATE_UNINITIALIZED);
static const struct device *s_dev;

static const nfc_transport_caps_t s_caps = {
	.roles = NFC_ROLE_READER,
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

const nfc_transport_caps_t *nfc_transport_get_capabilities(void)
{
	return &s_caps;
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

#if !DT_HAS_COMPAT_STATUS_OKAY(nxp_pn7160)
	return -ENODEV;
#else
	s_dev = PN7160_TRANSPORT_DEV;
	if (!device_is_ready(s_dev)) {
		LOG_ERR("PN7160 device not ready");
		return -ENODEV;
	}

	ret = pn7160_nci_connect(s_dev);
	if (ret != 0) {
		LOG_ERR("NCI connect failed: %d", ret);
		return ret;
	}

	ret = pn7160_nci_configure_settings(s_dev);
	if (ret != 0) {
		LOG_ERR("NCI settings failed: %d", ret);
		return ret;
	}

	atomic_set(&s_state, NFC_TRANSPORT_STATE_INITIALIZED);
	return 0;
#endif
}

int nfc_transport_shutdown(void)
{
	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_STARTED) {
		(void)nfc_transport_discover_stop();
	}

	atomic_set(&s_state, NFC_TRANSPORT_STATE_UNINITIALIZED);
	s_dev = NULL;
	return 0;
}

int nfc_transport_discover_start(nfc_tech_t tech_mask)
{
	int ret;

	ARG_UNUSED(tech_mask);

	if (s_dev == NULL) {
		return -ENODEV;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_UNINITIALIZED) {
		return -ENODEV;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_STARTED) {
		return -EBUSY;
	}

	ret = pn7160_nci_discovery_start(s_dev, NULL, 0U);
	if (ret != 0) {
		return ret;
	}

	atomic_set(&s_state, NFC_TRANSPORT_STATE_STARTED);
	return 0;
}

int nfc_transport_discover_stop(void)
{
	int ret = 0;

	if (s_dev == NULL) {
		return -ENODEV;
	}

	if (nfc_transport_get_state() == NFC_TRANSPORT_STATE_STARTED) {
		ret = pn7160_nci_discovery_stop(s_dev);
		atomic_set(&s_state, NFC_TRANSPORT_STATE_STOPPED);
	}

	return ret;
}

int nfc_transport_discover_wait(nfc_transport_tag_info_t *info, k_timeout_t timeout)
{
	struct pn7160_nci_rf_intf rf;
	int ret;

	if (info == NULL) {
		return -EINVAL;
	}

	if (s_dev == NULL || nfc_transport_get_state() != NFC_TRANSPORT_STATE_STARTED) {
		return -ENODEV;
	}

	ret = pn7160_nci_discovery_wait(s_dev, &rf, timeout);
	if (ret != 0) {
		return ret;
	}

	tag_info_from_rf(&rf, info);
	return 0;
}

int nfc_transport_tag_transceive(const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_max,
				 size_t *rx_len, k_timeout_t timeout)
{
	if (s_dev == NULL) {
		return -ENODEV;
	}

	return pn7160_nci_reader_tag_cmd(s_dev, tx, tx_len, rx, rx_max, rx_len, timeout);
}
