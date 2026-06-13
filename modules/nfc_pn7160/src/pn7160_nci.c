/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI 2.0 state machine — port of NXP NxpNci20.c (Phase 0.3+).
 * Derived from NXP-NCI2.0_LPC55S6x_examples/NfcLibrary/NxpNci20/src/NxpNci20.c
 * (NxpNci_CheckDevPres, NxpNci_Connect, NxpNci_HostTransceive).
 */

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#define PN7160_NCI_CORE_RESET_NTF_MT_OID 0x60U
#define PN7160_NCI_CORE_RESET_OID        0x00U
#define PN7160_NCI_CORE_GENERIC_ERROR_OID 0x07U
#define PN7160_NCI_ANTI_TEARING_STATUS   0xE6U
#define PN7160_NCI_STATUS_OK             0x00U

#define PN7160_NCI_CORE_INIT_RSP_MT_OID 0x40U
#define PN7160_NCI_CORE_INIT_OID        0x01U

/* NCI CORE_RESET_CMD — NXP NCICoreReset[] */
static const uint8_t core_reset_cmd[] = { 0x20, 0x00, 0x01, 0x01 };

/* NCI CORE_INIT_CMD — NXP NCICoreInit_2_0[] */
static const uint8_t core_init_cmd[] = { 0x20, 0x01, 0x02, 0x00, 0x00 };

static int pn7160_nci_parse_core_reset_ntf(const struct device *dev, const uint8_t *rx,
					   size_t rx_len)
{
	struct pn7160_data *data = dev->data;
	uint8_t fw_ver[3];
	int ret;

	ret = pn7160_nci_core_reset_ntf_fw_version(rx, rx_len, fw_ver);
	if (ret != 0) {
		return ret;
	}

	data->fw_version[0] = fw_ver[0];
	data->fw_version[1] = fw_ver[1];
	data->fw_version[2] = fw_ver[2];

	return 0;
}

static int pn7160_nci_parse_core_init_rsp(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 4U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_CORE_INIT_RSP_MT_OID || rx[1] != PN7160_NCI_CORE_INIT_OID) {
		return -EINVAL;
	}

	if (rx[3] != PN7160_NCI_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

void pn7160_nci_arm_rx(const struct device *dev)
{
	struct pn7160_data *data = dev->data;

	data->last_rx_len = 0U;
	data->last_rx_err = -EIO;
	k_sem_reset(&data->rx_sem);
	atomic_set(&data->rx_waiting, 1);

	if (atomic_cas(&data->irq_pending, 1, 0)) {
		pn7160_submit_rx_work(dev);
	}
}

static int pn7160_nci_wait_rx(const struct device *dev, k_timeout_t timeout)
{
	struct pn7160_data *data = dev->data;
	int ret;

	pn7160_nci_arm_rx(dev);

	ret = k_sem_take(&data->rx_sem, timeout);
	if (ret != 0) {
		atomic_clear(&data->rx_waiting);
		return -ETIMEDOUT;
	}

	return data->last_rx_err;
}

int pn7160_nci_process(const struct device *dev, const uint8_t *rx, size_t rx_len)
{
	struct pn7160_data *data = dev->data;

	if (rx_len >= 4U && rx[0] == PN7160_NCI_CORE_RESET_NTF_MT_OID &&
	    rx[1] == PN7160_NCI_CORE_GENERIC_ERROR_OID) {
		/* NXP NxpNci_CheckDevPres: CORE_GENERIC_ERROR_NTF anti-tearing. */
		if (rx[3] == PN7160_NCI_ANTI_TEARING_STATUS) {
			data->rf_settings_restored = true;
		}
		return 0;
	}

	if (rx_len >= 3U && rx[0] == PN7160_NCI_CORE_RESET_NTF_MT_OID &&
	    rx[1] == PN7160_NCI_CORE_RESET_OID) {
		return pn7160_nci_parse_core_reset_ntf(dev, rx, rx_len);
	}

	return 0;
}

static int pn7160_nci_transceive_unlocked(const struct device *dev, const uint8_t *tx,
					  size_t tx_len, uint8_t *rx, size_t rx_max,
					  size_t *rx_len, k_timeout_t timeout)
{
	struct pn7160_data *data = dev->data;
	int ret;

	if (tx == NULL || rx == NULL || rx_len == NULL || tx_len == 0U || rx_max == 0U) {
		return -EINVAL;
	}

	ret = pn7160_tml_send(dev, tx, tx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_wait_rx(dev, timeout);
	if (ret != 0) {
		return ret;
	}

	if (data->last_rx_len > rx_max) {
		return -EINVAL;
	}

	memcpy(rx, data->rx_buf, data->last_rx_len);
	*rx_len = data->last_rx_len;

	return 0;
}

int pn7160_nci_transceive(const struct device *dev, const uint8_t *tx, size_t tx_len,
			  uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout)
{
	struct pn7160_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->api_mutex, K_FOREVER);
	ret = pn7160_nci_transceive_unlocked(dev, tx, tx_len, rx, rx_max, rx_len, timeout);
	k_mutex_unlock(&data->api_mutex);

	return ret;
}

static int pn7160_nci_check_dev_pres_unlocked(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	int ret;

	data->rf_settings_restored = false;

	ret = pn7160_nci_transceive_unlocked(dev, core_reset_cmd, sizeof(core_reset_cmd), rx,
					     sizeof(data->rx_buf), &rx_len, K_MSEC(100));
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_core_reset_rsp_validate(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_wait_rx(dev, K_MSEC(100));
	if (ret != 0) {
		/* NXP: empty second RX on timeout is OK. */
		return 0;
	}

	if (data->last_rx_len == 0U) {
		return 0;
	}

	if (data->rx_buf[0] == PN7160_NCI_CORE_RESET_NTF_MT_OID &&
	    data->rx_buf[1] == PN7160_NCI_CORE_GENERIC_ERROR_OID) {
		/* Handled in pn7160_nci_process during IRQ drain. */
		return 0;
	}

	if (data->rx_buf[0] == PN7160_NCI_CORE_RESET_NTF_MT_OID &&
	    data->rx_buf[1] == PN7160_NCI_CORE_RESET_OID) {
		return pn7160_nci_parse_core_reset_ntf(dev, data->rx_buf, data->last_rx_len);
	}

	return -EIO;
}

int pn7160_nci_check_dev_pres(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->api_mutex, K_FOREVER);
	ret = pn7160_nci_check_dev_pres_unlocked(dev);
	k_mutex_unlock(&data->api_mutex);

	return ret;
}

int pn7160_nci_connect(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	int ret;
	int retries = 2;

	k_mutex_lock(&data->api_mutex, K_FOREVER);

	/* NXP NxpNci_Connect: probe with up to two retries (500 ms apart). */
	while (retries-- >= 0) {
		ret = pn7160_nci_check_dev_pres_unlocked(dev);
		if (ret == 0) {
			break;
		}
		if (retries < 0) {
			goto out;
		}
		k_msleep(500);
	}

	ret = pn7160_nci_transceive_unlocked(dev, core_init_cmd, sizeof(core_init_cmd), rx,
					     sizeof(data->rx_buf), &rx_len, K_SECONDS(1));
	if (ret != 0) {
		goto out;
	}

	ret = pn7160_nci_parse_core_init_rsp(rx, rx_len);

out:
	k_mutex_unlock(&data->api_mutex);
	return ret;
}

int pn7160_nci_host_transceive(const struct device *dev, const uint8_t *tx, size_t tx_len,
			       uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout)
{
	return pn7160_nci_transceive(dev, tx, tx_len, rx, rx_max, rx_len, timeout);
}
