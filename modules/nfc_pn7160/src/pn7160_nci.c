/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI 2.0 state machine — port of NXP NxpNci20.c (Phase 0.3+ skeleton).
 */

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

/* NCI CORE_RESET_CMD */
static const uint8_t core_reset_cmd[] = { 0x20, 0x00, 0x01, 0x00 };

int pn7160_nci_check_dev_pres(const struct device *dev)
{
	uint8_t rx[CONFIG_PN7160_RX_BUF_SIZE];
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

	return pn7160_tml_recv(dev, rx, sizeof(rx), &rx_len);
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
