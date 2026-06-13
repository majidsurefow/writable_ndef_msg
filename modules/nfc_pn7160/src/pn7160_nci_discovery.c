/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI discovery — port of NXP NxpNci_ConfigureMode / StartDiscovery /
 * StopDiscovery / WaitForDiscoveryNotification (reader mode, RW-only).
 * Derived from NXP-NCI2.0_LPC55S6x_examples/NfcLibrary/NxpNci20/src/NxpNci20.c
 */

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#define PN7160_NCI_RF_RSP_MT_OID           0x41U
#define PN7160_NCI_RF_NTF_MT_OID           0x61U
#define PN7160_NCI_PROP_RSP_MT_OID         0x4FU
#define PN7160_NCI_RF_DISCOVER_MAP_OID     0x00U
#define PN7160_NCI_RF_DISCOVER_OID         0x03U
#define PN7160_NCI_RF_DISCOVER_SELECT_OID  0x04U
#define PN7160_NCI_RF_INTF_ACTIVATED_OID   0x05U
#define PN7160_NCI_PROP_ACTIVATE_OID       0x02U
#define PN7160_NCI_STATUS_OK               0x00U
#define PN7160_NCI_DISC_START_MAX          30U

static const uint8_t nci_prop_act[] = { 0x2F, 0x02, 0x00 };
static const uint8_t dm_rw[] = { 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x03, 0x01, 0x01,
				 0x04, 0x01, 0x02, 0x80, 0x01, 0x80 };
static const uint8_t nci_stop_discovery[] = { 0x21, 0x06, 0x01, 0x00 };

static const uint8_t default_discovery_tech[] = {
	PN7160_NCI_MODE_POLL | PN7160_NCI_TECH_PASSIVE_NFCA,
	PN7160_NCI_MODE_POLL | PN7160_NCI_TECH_PASSIVE_NFCB,
	PN7160_NCI_MODE_POLL | PN7160_NCI_TECH_PASSIVE_15693,
};

static uint8_t nci_start_discovery[PN7160_NCI_DISC_START_MAX];
static size_t nci_start_discovery_len;

static int pn7160_nci_rf_rsp_ok(const uint8_t *rx, size_t rx_len, uint8_t oid)
{
	if (rx_len < 4U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_RF_RSP_MT_OID || rx[1] != oid || rx[3] != PN7160_NCI_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

static void pn7160_nci_fill_interface_info(struct pn7160_nci_rf_intf *rf_intf, const uint8_t *buf)
{
	uint8_t temp;

	switch (rf_intf->mode_tech) {
	case PN7160_NCI_MODE_POLL | PN7160_NCI_TECH_PASSIVE_NFCA:
		temp = 2U;
		rf_intf->uid_len = buf[temp];
		temp++;
		if (rf_intf->uid_len > sizeof(rf_intf->uid)) {
			rf_intf->uid_len = sizeof(rf_intf->uid);
		}
		memcpy(rf_intf->uid, &buf[temp], rf_intf->uid_len);
		break;

	case PN7160_NCI_MODE_POLL | PN7160_NCI_TECH_PASSIVE_15693:
		rf_intf->uid_len = 8U;
		for (uint8_t i = 0U; i < 8U; i++) {
			rf_intf->uid[i] = buf[2U + (7U - i)];
		}
		break;

	default:
		rf_intf->uid_len = 0U;
		break;
	}
}

static void pn7160_nci_rf_intf_clear(struct pn7160_nci_rf_intf *rf_intf)
{
	memset(rf_intf, 0, sizeof(*rf_intf));
}

static int pn7160_nci_configure_mode_unlocked(const struct device *dev, uint8_t mode)
{
	struct pn7160_data *data = dev->data;
	uint8_t cmd[4U + sizeof(dm_rw)];
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	size_t item_count;
	int ret;

	if (mode == 0U) {
		return 0;
	}

	if ((mode & PN7160_NCI_MODE_RW) != 0U) {
		ret = pn7160_nci_transceive_unlocked(dev, nci_prop_act, sizeof(nci_prop_act), rx,
						     sizeof(data->rx_buf), &rx_len, K_SECONDS(1));
		if (ret != 0) {
			return ret;
		}

		if (rx_len < 4U || rx[0] != PN7160_NCI_PROP_RSP_MT_OID ||
		    rx[1] != PN7160_NCI_PROP_ACTIVATE_OID || rx[3] != PN7160_NCI_STATUS_OK) {
			return -EIO;
		}
	}

	item_count = 0U;

	if ((mode & PN7160_NCI_MODE_RW) != 0U) {
		memcpy(&cmd[4U + (3U * item_count)], dm_rw, sizeof(dm_rw));
		item_count += sizeof(dm_rw) / 3U;
	}

	if (item_count == 0U) {
		return 0;
	}

	cmd[0] = 0x21;
	cmd[1] = PN7160_NCI_RF_DISCOVER_MAP_OID;
	cmd[2] = (uint8_t)(1U + (item_count * 3U));
	cmd[3] = (uint8_t)item_count;

	ret = pn7160_nci_transceive_unlocked(dev, cmd, 3U + cmd[2], rx, sizeof(data->rx_buf),
					     &rx_len, K_SECONDS(1));
	if (ret != 0) {
		return ret;
	}

	return pn7160_nci_rf_rsp_ok(rx, rx_len, PN7160_NCI_RF_DISCOVER_MAP_OID);
}

static int pn7160_nci_build_start_discovery(const uint8_t *tech_tab, size_t tech_count,
					    uint8_t *cmd, size_t cmd_max, size_t *cmd_len)
{
	if (tech_tab == NULL || tech_count == 0U || cmd == NULL || cmd_len == NULL) {
		return -EINVAL;
	}

	if ((tech_count * 2U) + 4U > cmd_max) {
		return -EINVAL;
	}

	cmd[0] = 0x21;
	cmd[1] = PN7160_NCI_RF_DISCOVER_OID;
	cmd[2] = (uint8_t)((tech_count * 2U) + 1U);
	cmd[3] = (uint8_t)tech_count;

	for (size_t i = 0U; i < tech_count; i++) {
		cmd[(i * 2U) + 4U] = tech_tab[i];
		cmd[(i * 2U) + 5U] = 0x01;
	}

	*cmd_len = (tech_count * 2U) + 4U;

	return 0;
}

static int pn7160_nci_start_discovery_unlocked(const struct device *dev, const uint8_t *tech_tab,
					      size_t tech_count)
{
	struct pn7160_data *data = dev->data;
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	int ret;

	ret = pn7160_nci_build_start_discovery(tech_tab, tech_count, nci_start_discovery,
					       sizeof(nci_start_discovery),
					       &nci_start_discovery_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_transceive_unlocked(dev, nci_start_discovery, nci_start_discovery_len, rx,
					     sizeof(data->rx_buf), &rx_len, K_SECONDS(1));
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_rf_rsp_ok(rx, rx_len, PN7160_NCI_RF_DISCOVER_OID);
	if (ret != 0) {
		return ret;
	}

	data->discovery_active = true;
	pn7160_nci_rf_intf_clear(&data->last_rf_intf);

	return 0;
}

static int pn7160_nci_stop_discovery_unlocked(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	int ret;

	ret = pn7160_nci_transceive_unlocked(dev, nci_stop_discovery, sizeof(nci_stop_discovery),
					     rx, sizeof(data->rx_buf), &rx_len, K_SECONDS(1));
	if (ret != 0) {
		return ret;
	}

	(void)pn7160_nci_recv_unlocked(dev, rx, sizeof(data->rx_buf), &rx_len, K_SECONDS(1));

	data->discovery_active = false;

	return 0;
}

static int pn7160_nci_wait_discovery_notification_unlocked(const struct device *dev,
							   struct pn7160_nci_rf_intf *rf_intf,
							   k_timeout_t timeout)
{
	struct pn7160_data *data = dev->data;
	uint8_t rf_discover_select[] = { 0x21, PN7160_NCI_RF_DISCOVER_SELECT_OID, 0x03, 0x01,
					 PN7160_NCI_PROT_ISODEP, PN7160_NCI_INTF_ISODEP };
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	int ret;

	if (rf_intf == NULL) {
		return -EINVAL;
	}

	pn7160_nci_rf_intf_clear(rf_intf);
	data->next_tag_protocol = PN7160_NCI_PROT_UNDETERMINED;

	do {
		ret = pn7160_nci_recv_unlocked(dev, rx, sizeof(data->rx_buf), &rx_len, timeout);
		if (ret != 0) {
			return ret;
		}
	} while (rx_len < 2U || rx[0] != PN7160_NCI_RF_NTF_MT_OID ||
		 (rx[1] != PN7160_NCI_RF_INTF_ACTIVATED_OID &&
		  rx[1] != PN7160_NCI_RF_DISCOVER_OID));

	if (rx[1] == PN7160_NCI_RF_INTF_ACTIVATED_OID) {
		if (rx_len < 11U) {
			return -EIO;
		}

		rf_intf->interface = rx[4];
		rf_intf->protocol = rx[5];
		rf_intf->mode_tech = rx[6];
		rf_intf->more_tags = false;
		pn7160_nci_fill_interface_info(rf_intf, &rx[10]);
	} else {
		if (rx_len < 6U) {
			return -EIO;
		}

		rf_intf->interface = PN7160_NCI_INTF_UNDETERMINED;
		rf_intf->protocol = rx[4];
		rf_intf->mode_tech = rx[5];
		rf_intf->more_tags = true;

		do {
			ret = pn7160_nci_recv_unlocked(dev, rx, sizeof(data->rx_buf), &rx_len,
						       K_MSEC(100));
			if (ret != 0) {
				return ret;
			}
		} while (rx_len < 2U || rx[0] != PN7160_NCI_RF_NTF_MT_OID ||
			 rx[1] != PN7160_NCI_RF_DISCOVER_OID);

		data->next_tag_protocol = rx[4];

		while (rx_len >= 1U && rx[rx_len - 1U] == 0x02U) {
			ret = pn7160_nci_recv_unlocked(dev, rx, sizeof(data->rx_buf), &rx_len,
						       K_MSEC(100));
			if (ret != 0) {
				return ret;
			}
		}

		rf_discover_select[4] = rf_intf->protocol;
		if (rf_intf->protocol == PN7160_NCI_PROT_ISODEP) {
			rf_discover_select[5] = PN7160_NCI_INTF_ISODEP;
		} else if (rf_intf->protocol == PN7160_NCI_PROT_MIFARE) {
			rf_discover_select[5] = PN7160_NCI_INTF_TAGCMD;
		} else {
			rf_discover_select[5] = PN7160_NCI_INTF_FRAME;
		}

		ret = pn7160_nci_transceive_unlocked(dev, rf_discover_select,
						     sizeof(rf_discover_select), rx,
						     sizeof(data->rx_buf), &rx_len, K_SECONDS(1));
		if (ret != 0) {
			return ret;
		}

		if (rx_len >= 4U && rx[0] == PN7160_NCI_RF_RSP_MT_OID &&
		    rx[1] == PN7160_NCI_RF_DISCOVER_SELECT_OID && rx[3] == PN7160_NCI_STATUS_OK) {
			ret = pn7160_nci_recv_unlocked(dev, rx, sizeof(data->rx_buf), &rx_len,
						       K_MSEC(100));
			if (ret == 0 && rx_len >= 11U && rx[0] == PN7160_NCI_RF_NTF_MT_OID &&
			    rx[1] == PN7160_NCI_RF_INTF_ACTIVATED_OID) {
				rf_intf->interface = rx[4];
				rf_intf->protocol = rx[5];
				rf_intf->mode_tech = rx[6];
				rf_intf->more_tags = false;
				pn7160_nci_fill_interface_info(rf_intf, &rx[10]);
			}
		}
	}

	if (rf_intf->interface == PN7160_NCI_INTF_UNDETERMINED) {
		rf_intf->protocol = PN7160_NCI_PROT_UNDETERMINED;
	}

	memcpy(&data->last_rf_intf, rf_intf, sizeof(*rf_intf));

	return 0;
}

int pn7160_nci_configure_mode(const struct device *dev, uint8_t mode)
{
	struct pn7160_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->api_mutex, K_FOREVER);
	ret = pn7160_nci_configure_mode_unlocked(dev, mode);
	k_mutex_unlock(&data->api_mutex);

	return ret;
}

int pn7160_nci_discovery_start(const struct device *dev, const uint8_t *tech_tab,
			       size_t tech_count)
{
	struct pn7160_data *data = dev->data;
	const uint8_t *tech = tech_tab;
	size_t count = tech_count;
	int ret;

	if (tech == NULL || count == 0U) {
		tech = default_discovery_tech;
		count = ARRAY_SIZE(default_discovery_tech);
	}

	k_mutex_lock(&data->api_mutex, K_FOREVER);

	ret = pn7160_nci_configure_mode_unlocked(dev, PN7160_NCI_MODE_RW);
	if (ret != 0) {
		goto out;
	}

	ret = pn7160_nci_start_discovery_unlocked(dev, tech, count);

out:
	k_mutex_unlock(&data->api_mutex);

	return ret;
}

int pn7160_nci_discovery_stop(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->api_mutex, K_FOREVER);
	ret = pn7160_nci_stop_discovery_unlocked(dev);
	k_mutex_unlock(&data->api_mutex);

	return ret;
}

int pn7160_nci_discovery_wait(const struct device *dev, struct pn7160_nci_rf_intf *rf_intf,
			      k_timeout_t timeout)
{
	struct pn7160_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->api_mutex, K_FOREVER);
	ret = pn7160_nci_wait_discovery_notification_unlocked(dev, rf_intf, timeout);
	k_mutex_unlock(&data->api_mutex);

	return ret;
}

bool pn7160_nci_discovery_active(const struct device *dev)
{
	struct pn7160_data *data = dev->data;

	return data->discovery_active;
}

const struct pn7160_nci_rf_intf *pn7160_nci_discovery_last(const struct device *dev)
{
	struct pn7160_data *data = dev->data;

	return &data->last_rf_intf;
}
