/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI 2.0 state machine — port of NXP NxpNci20.c (Phase 0.3+).
 * Derived from NXP-NCI2.0_LPC55S6x_examples/NfcLibrary/NxpNci20/src/NxpNci20.c
 * (NxpNci_CheckDevPres).
 */

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#define PN7160_NCI_CORE_RESET_RSP_MT_OID 0x40U
#define PN7160_NCI_CORE_RESET_NTF_MT_OID 0x60U
#define PN7160_NCI_CORE_RESET_OID        0x00U
#define PN7160_NCI_STATUS_OK             0x00U
#define PN7160_NCI_NTF_NCI_VERSION       0x20U

/* NCI CORE_RESET_CMD — NXP NCICoreReset[] */
static const uint8_t core_reset_cmd[] = { 0x20, 0x00, 0x01, 0x01 };

static int pn7160_nci_parse_core_reset_ntf(const struct device *dev, const uint8_t *rx,
					   size_t rx_len)
{
	struct pn7160_data *data = dev->data;

	if (rx_len < 12U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_CORE_RESET_NTF_MT_OID || rx[1] != PN7160_NCI_CORE_RESET_OID ||
	    rx[5] != PN7160_NCI_NTF_NCI_VERSION) {
		return -EINVAL;
	}

	data->fw_version[0] = rx[9];
	data->fw_version[1] = rx[10];
	data->fw_version[2] = rx[11];

	return 0;
}

static int pn7160_nci_parse_core_reset_rsp(const uint8_t *rx, size_t rx_len)
{
	if (rx_len < 4U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_CORE_RESET_RSP_MT_OID || rx[1] != PN7160_NCI_CORE_RESET_OID) {
		return -EINVAL;
	}

	if (rx[3] != PN7160_NCI_STATUS_OK) {
		return -EIO;
	}

	return 0;
}

int pn7160_nci_check_dev_pres(const struct device *dev)
{
	struct pn7160_data *data = dev->data;
	uint8_t *rx = data->rx_buf;
	size_t rx_len;
	int ret;

	ret = pn7160_tml_send(dev, core_reset_cmd, sizeof(core_reset_cmd));
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_wait_irq(dev, K_MSEC(100));
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_tml_recv(dev, rx, sizeof(data->rx_buf), &rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_nci_parse_core_reset_rsp(rx, rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_wait_irq(dev, K_MSEC(100));
	if (ret == 0) {
		ret = pn7160_tml_recv(dev, rx, sizeof(data->rx_buf), &rx_len);
		if (ret == 0 && rx_len > 0U) {
			if (rx[0] == PN7160_NCI_CORE_RESET_NTF_MT_OID &&
			    rx[1] == PN7160_NCI_CORE_RESET_OID) {
				(void)pn7160_nci_parse_core_reset_ntf(dev, rx, rx_len);
			}
		}
	}

	return 0;
}

int pn7160_nci_host_transceive(const struct device *dev, const uint8_t *tx, size_t tx_len,
			       uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout)
{
	int ret;

	ret = pn7160_tml_send(dev, tx, tx_len);
	if (ret != 0) {
		return ret;
	}

	ret = pn7160_wait_irq(dev, timeout);
	if (ret != 0) {
		return ret;
	}

	return pn7160_tml_recv(dev, rx, rx_max, rx_len);
}
