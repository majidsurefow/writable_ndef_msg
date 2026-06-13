/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI card mode — port of NXP NxpNci_CardModeReceive / NxpNci_CardModeSend.
 * Derived from NXP-NCI2.0_LPC55S6x_examples/NfcLibrary/NxpNci20/src/NxpNci20.c
 */

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

int pn7160_nci_card_mode_recv(const struct device *dev, uint8_t *data, size_t data_max,
			      size_t *data_len, k_timeout_t timeout)
{
	struct pn7160_data *data_ctx = dev->data;
	uint8_t *rx = data_ctx->rx_buf;
	size_t rx_len;
	size_t payload_len;
	int ret;

	if (data == NULL || data_len == NULL || data_max == 0U) {
		return -EINVAL;
	}

	k_mutex_lock(&data_ctx->api_mutex, K_FOREVER);

	ret = pn7160_nci_recv_unlocked(dev, rx, sizeof(data_ctx->rx_buf), &rx_len, timeout);
	if (ret != 0) {
		goto out;
	}

	if (rx_len < 3U || rx[0] != 0x00U || rx[1] != 0x00U) {
		ret = -EIO;
		goto out;
	}

	payload_len = rx[2];
	if (payload_len > data_max || (3U + payload_len) > rx_len) {
		ret = -EINVAL;
		goto out;
	}

	memcpy(data, &rx[3], payload_len);
	*data_len = payload_len;
	ret = 0;

out:
	k_mutex_unlock(&data_ctx->api_mutex);
	return ret;
}

int pn7160_nci_card_mode_send(const struct device *dev, const uint8_t *data, size_t data_len,
			      k_timeout_t timeout)
{
	struct pn7160_data *data_ctx = dev->data;
	uint8_t tx[3U + CONFIG_PN7160_RX_BUF_SIZE];
	uint8_t *rx = data_ctx->rx_buf;
	size_t rx_len;
	int ret;

	if (data == NULL || data_len == 0U) {
		return -EINVAL;
	}

	if (data_len > (sizeof(tx) - 3U)) {
		return -EINVAL;
	}

	tx[0] = 0x00U;
	tx[1] = 0x00U;
	tx[2] = (uint8_t)data_len;
	memcpy(&tx[3], data, data_len);

	k_mutex_lock(&data_ctx->api_mutex, K_FOREVER);

	ret = pn7160_nci_transceive_unlocked(dev, tx, data_len + 3U, rx, sizeof(data_ctx->rx_buf),
					     &rx_len, timeout);

	k_mutex_unlock(&data_ctx->api_mutex);
	return ret;
}
