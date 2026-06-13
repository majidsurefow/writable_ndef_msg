/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI RF / core settings — port of NXP NxpNci_ConfigureSettings().
 * Derived from NXP-NCI2.0_LPC55S6x_examples/NfcLibrary/inc/Nfc_settings.h
 * and NfcLibrary/NxpNci20/src/NxpNci20.c (NxpNci_ConfigureSettings).
 */

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#define PN7160_NCI_CORE_SET_CONFIG_RSP_MT_OID 0x40U
#define PN7160_NCI_CORE_SET_CONFIG_OID       0x02U
#define PN7160_NCI_CORE_GET_CONFIG_OID       0x03U
#define PN7160_NCI_CORE_RESET_RSP_MT_OID     0x40U
#define PN7160_NCI_CORE_RESET_OID            0x00U
#define PN7160_NCI_CORE_INIT_RSP_MT_OID      0x40U
#define PN7160_NCI_CORE_INIT_OID             0x01U
#define PN7160_NCI_PROP_RSP_MT_OID           0x4FU
#define PN7160_NCI_PROP_STANDBY_OID          0x00U
#define PN7160_NCI_STATUS_OK                 0x00U
#define PN7160_NCI_SETTING_TS_LEN            32U

/* NXP Nfc_settings.h — NXP_CORE_CONF */
static const uint8_t core_conf[] = { 0x20, 0x02, 0x05, 0x01, 0x00, 0x02, 0xFE, 0x01 };

/* NXP Nfc_settings.h — NXP_CORE_CONF_EXTN */
static const uint8_t core_conf_extn[] = { 0x20, 0x02, 0x09, 0x02, 0xA0, 0x40, 0x01, 0x00,
					  0xA0, 0x95, 0x01, 0x01 };

/* NXP Nfc_settings.h — NXP_CORE_STANDBY */
static const uint8_t core_standby[] = { 0x2F, 0x00, 0x01, 0x00 };

/* NXP Nfc_settings.h — NXP_CLK_CONF (Xtal) */
static const uint8_t clk_conf[] = { 0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x08 };

/* NXP Nfc_settings.h — NXP_TVDD_CONF (4.75V) */
static const uint8_t tvdd_conf[] = { 0x20, 0x02, 0x0F, 0x01, 0xA0, 0x0E, 0x0B, 0x11, 0x01,
				     0x01, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00, 0xD0, 0x0C };

/* NXP Nfc_settings.h — NXP_RF_CONF (OM27160 demo kit tuning) */
static const uint8_t rf_conf[] = {
	0x20, 0x02, 0x4C, 0x09, 0xA0, 0x0D, 0x03, 0x78, 0x0D, 0x02, 0xA0, 0x0D, 0x03, 0x78, 0x14,
	0x02, 0xA0, 0x0D, 0x06, 0x4C, 0x44, 0x65, 0x09, 0x00, 0x00, 0xA0, 0x0D, 0x06, 0x4C, 0x2D,
	0x05, 0x35, 0x1E, 0x01, 0xA0, 0x0D, 0x06, 0x82, 0x4A, 0x55, 0x07, 0x00, 0x07, 0xA0, 0x0D,
	0x06, 0x44, 0x44, 0x03, 0x04, 0xC4, 0x00, 0xA0, 0x0D, 0x06, 0x46, 0x30, 0x50, 0x00, 0x18,
	0x00, 0xA0, 0x0D, 0x06, 0x48, 0x30, 0x50, 0x00, 0x18, 0x00, 0xA0, 0x0D, 0x06, 0x4A, 0x30,
	0x50, 0x00, 0x08, 0x00
};

static const uint8_t core_reset_cmd[] = { 0x20, 0x00, 0x01, 0x00 };
static const uint8_t core_init_cmd[] = { 0x20, 0x01, 0x02, 0x00, 0x00 };
static const uint8_t read_ts_cmd[] = { 0x20, 0x03, 0x03, 0x01, 0xA0, 0x14 };

static const uint8_t setting_current_ts[PN7160_NCI_SETTING_TS_LEN] = __TIMESTAMP__;

static int pn7160_nci_validate_core_set_config_rsp(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 5U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_CORE_SET_CONFIG_RSP_MT_OID ||
	    rx[1] != PN7160_NCI_CORE_SET_CONFIG_OID || rx[3] != PN7160_NCI_STATUS_OK ||
	    rx[4] != PN7160_NCI_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

static int pn7160_nci_validate_core_get_config_rsp(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 4U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_CORE_SET_CONFIG_RSP_MT_OID ||
	    rx[1] != PN7160_NCI_CORE_GET_CONFIG_OID || rx[3] != PN7160_NCI_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

static int pn7160_nci_validate_prop_standby_rsp(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 4U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_PROP_RSP_MT_OID || rx[1] != PN7160_NCI_PROP_STANDBY_OID ||
	    rx[3] != PN7160_NCI_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

static int pn7160_nci_settings_transceive(const struct device *dev, const uint8_t *tx,
					    size_t tx_len, uint8_t *rx, size_t rx_max,
					    size_t *rx_len)
{
	return pn7160_nci_transceive_unlocked(dev, tx, tx_len, rx, rx_max, rx_len, K_SECONDS(1));
}

static int pn7160_nci_configure_settings_unlocked(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	uint8_t *rx = data->rx_buf;
	uint8_t write_ts_cmd[7U + PN7160_NCI_SETTING_TS_LEN];
	size_t rx_len;
	bool reset_required = false;
	int ret;

	ret = pn7160_nci_settings_transceive(dev, core_standby, sizeof(core_standby), rx,
					     sizeof(data->rx_buf), &rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_validate_prop_standby_rsp(rx, rx_len);
	if (ret != 0) {
		return ret;
	}
	reset_required = true;

	ret = pn7160_nci_settings_transceive(dev, core_conf, sizeof(core_conf), rx,
					     sizeof(data->rx_buf), &rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_validate_core_set_config_rsp(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_settings_transceive(dev, read_ts_cmd, sizeof(read_ts_cmd), rx,
					     sizeof(data->rx_buf), &rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_validate_core_get_config_rsp(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	if (rx_len >= (8U + PN7160_NCI_SETTING_TS_LEN) &&
	    memcmp(&rx[8], setting_current_ts, PN7160_NCI_SETTING_TS_LEN) == 0 &&
	    !data->rf_settings_restored) {
		LOG_DBG("NCI settings timestamp unchanged, skipping RF blobs");
	} else {
		ret = pn7160_nci_settings_transceive(dev, core_conf_extn, sizeof(core_conf_extn), rx,
						     sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_validate_core_set_config_rsp(rx, rx_len);
		if (ret != 0) {
			return ret;
		}
		reset_required = true;

		ret = pn7160_nci_settings_transceive(dev, clk_conf, sizeof(clk_conf), rx,
						     sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_validate_core_set_config_rsp(rx, rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_settings_transceive(dev, tvdd_conf, sizeof(tvdd_conf), rx,
						     sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_validate_core_set_config_rsp(rx, rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_settings_transceive(dev, rf_conf, sizeof(rf_conf), rx,
						     sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_validate_core_set_config_rsp(rx, rx_len);
		if (ret != 0) {
			return ret;
		}

		write_ts_cmd[0] = 0x20;
		write_ts_cmd[1] = 0x02;
		write_ts_cmd[2] = 0x24;
		write_ts_cmd[3] = 0x01;
		write_ts_cmd[4] = 0xA0;
		write_ts_cmd[5] = 0x14;
		write_ts_cmd[6] = 0x20;
		memcpy(&write_ts_cmd[7], setting_current_ts, PN7160_NCI_SETTING_TS_LEN);

		ret = pn7160_nci_settings_transceive(dev, write_ts_cmd, sizeof(write_ts_cmd), rx,
						     sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_validate_core_set_config_rsp(rx, rx_len);
		if (ret != 0) {
			return ret;
		}

		data->rf_settings_restored = false;
	}

	if (reset_required) {
		ret = pn7160_nci_settings_transceive(dev, core_reset_cmd, sizeof(core_reset_cmd),
						     rx, sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		ret = pn7160_nci_core_reset_rsp_validate(rx, rx_len);
		if (ret != 0) {
			return ret;
		}

		(void)pn7160_nci_wait_rx_unlocked(dev, K_MSEC(100));

		ret = pn7160_nci_settings_transceive(dev, core_init_cmd, sizeof(core_init_cmd), rx,
						     sizeof(data->rx_buf), &rx_len);
		if (ret != 0) {
			return ret;
		}

		if (rx_len < 4U || rx[0] != PN7160_NCI_CORE_INIT_RSP_MT_OID ||
		    rx[1] != PN7160_NCI_CORE_INIT_OID || rx[3] != PN7160_NCI_STATUS_OK) {
			return -EIO;
		}
	}

	return 0;
}

int pn7160_nci_configure_settings(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->api_mutex, K_FOREVER);
	ret = pn7160_nci_configure_settings_unlocked(dev);
	k_mutex_unlock(&data->api_mutex);

	return ret;
}
