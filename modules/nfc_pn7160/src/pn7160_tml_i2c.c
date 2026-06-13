/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML I2C framing — port of NXP source/TML/tml.c (I2C path).
 * Derived from NXP-NCI2.0_LPC55S6x_examples/source/TML/tml.c (I2C INTF_*,
 * tml_Tx, tml_Rx). Replaces NXP I2C calls with Zephyr i2c_*_dt helpers.
 */

#define DT_DRV_COMPAT nxp_pn7160

#include <nfc/pn7160.h>
#include <nfc/pn7160_tml.h>
#include "pn7160_priv.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

#define PN7160_TML_FOOTER_SZ 0U

static int pn7160_tml_i2c_write(const struct i2c_dt_spec *i2c, const uint8_t *data, size_t len)
{
	if (i2c_write_dt(i2c, data, len) == 0) {
		return 0;
	}

	k_msleep(10);

	return i2c_write_dt(i2c, data, len) == 0 ? 0 : -EIO;
}

int pn7160_tml_i2c_send(const struct device *dev, const uint8_t *data, size_t len)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *pdata = dev->data;
	int ret;

	if (data == NULL || len == 0U) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&pdata->bus_mutex, K_FOREVER);
	if (ret != 0) {
		return -EIO;
	}

	ret = pn7160_tml_i2c_write(&cfg->i2c, data, len);

	(void)k_mutex_unlock(&pdata->bus_mutex);

	return ret;
}

int pn7160_tml_i2c_recv(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *pdata = dev->data;
	uint8_t payload_len;
	size_t frame_len;
	int ret;

	if (data == NULL || out_len == NULL || max_len < PN7160_TML_NCI_HDR_LEN) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&pdata->bus_mutex, K_FOREVER);
	if (ret != 0) {
		return -EIO;
	}

	/* NXP tml_Rx: read HEADER_SZ + 1 bytes (NCI header prefix). */
	ret = i2c_read_dt(&cfg->i2c, data, PN7160_TML_NCI_HDR_LEN);
	if (ret != 0) {
		ret = -EIO;
		goto unlock;
	}

	ret = pn7160_tml_frame_len_validate(data, max_len);
	if (ret != 0) {
		goto unlock;
	}

	payload_len = pn7160_tml_payload_len_get(data);
	frame_len = pn7160_tml_frame_len_get(data);

	if (payload_len > 0U) {
		ret = i2c_read_dt(&cfg->i2c, &data[PN7160_TML_NCI_HDR_LEN],
				  (size_t)payload_len + PN7160_TML_FOOTER_SZ);
		if (ret != 0) {
			ret = -EIO;
			goto unlock;
		}
	}

	*out_len = frame_len;
	ret = 0;

unlock:
	(void)k_mutex_unlock(&pdata->bus_mutex);

	return ret;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
