/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML I2C framing — port of NXP source/TML/tml.c (I2C path).
 */

#define DT_DRV_COMPAT nxp_pn7160

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

int pn7160_tml_i2c_send(const struct device *dev, const uint8_t *data, size_t len)
{
	const struct pn7160_config *cfg = dev->config;
	uint8_t hdr[2] = { (uint8_t)(len & 0xFF), (uint8_t)((len >> 8) & 0xFF) };

	if (i2c_write_dt(&cfg->i2c, hdr, sizeof(hdr)) != 0) {
		return -EIO;
	}

	return i2c_write_dt(&cfg->i2c, data, len) == 0 ? 0 : -EIO;
}

int pn7160_tml_i2c_recv(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len)
{
	const struct pn7160_config *cfg = dev->config;
	uint8_t hdr[2];
	uint16_t payload_len;

	if (i2c_read_dt(&cfg->i2c, hdr, sizeof(hdr)) != 0) {
		return -EIO;
	}

	payload_len = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
	if (payload_len == 0 || payload_len > max_len) {
		return -EINVAL;
	}

	if (i2c_read_dt(&cfg->i2c, data, payload_len) != 0) {
		return -EIO;
	}

	*out_len = payload_len;
	return 0;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
