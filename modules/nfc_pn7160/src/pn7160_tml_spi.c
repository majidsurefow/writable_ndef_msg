/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML SPI framing — port of NXP source/TML/tml.c (SPI path).
 * Derived from NXP-NCI2.0_LPC55S6x_examples/source/TML/tml.c (INTF_WRITE,
 * INTF_READ, tml_Tx, tml_Rx). Replaces NXP SPI calls with spi_*_dt helpers.
 */

#define DT_DRV_COMPAT nxp_pn7160

#include <nfc/pn7160.h>
#include <nfc/pn7160_tml.h>
#include "pn7160_priv.h"

#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define PN7160_SPI_STACK_BUF_MAX (PN7160_TML_MAX_PAYLOAD_LEN + PN7160_TML_NCI_HDR_LEN + 1U)

static int pn7160_tml_spi_transceive(const struct spi_dt_spec *spi,
				     const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
	const struct spi_buf tx = {
		.buf = (void *)tx_buf,
		.len = len,
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx,
		.count = 1,
	};
	struct spi_buf rx = {
		.buf = rx_buf,
		.len = len,
	};
	const struct spi_buf_set rx_set = {
		.buffers = &rx,
		.count = 1,
	};

	if (spi_transceive_dt(spi, &tx_set, &rx_set) == 0) {
		return 0;
	}

	k_msleep(10);

	return spi_transceive_dt(spi, &tx_set, &rx_set) == 0 ? 0 : -EIO;
}

static int pn7160_tml_spi_write(const struct spi_dt_spec *spi, const uint8_t *data, size_t len)
{
	uint8_t buf[PN7160_SPI_STACK_BUF_MAX];
	const struct spi_buf tx = {
		.buf = buf,
		.len = len + 1U,
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx,
		.count = 1,
	};

	if (len > (PN7160_SPI_STACK_BUF_MAX - 1U)) {
		return -EINVAL;
	}

	buf[0] = PN7160_TML_SPI_WRITE_PREFIX;
	for (size_t i = 0U; i < len; i++) {
		buf[i + 1U] = data[i];
	}

	if (spi_write_dt(spi, &tx_set) == 0) {
		return 0;
	}

	k_msleep(10);

	return spi_write_dt(spi, &tx_set) == 0 ? 0 : -EIO;
}

static int pn7160_tml_spi_read(const struct spi_dt_spec *spi, uint8_t *data, size_t len)
{
	uint8_t buf[PN7160_SPI_STACK_BUF_MAX];
	int ret;

	if (len > (PN7160_SPI_STACK_BUF_MAX - 1U)) {
		return -EINVAL;
	}

	for (size_t i = 0U; i < len + 1U; i++) {
		buf[i] = PN7160_TML_SPI_READ_DUMMY;
	}

	ret = pn7160_tml_spi_transceive(spi, buf, buf, len + 1U);
	if (ret != 0) {
		return ret;
	}

	for (size_t i = 0U; i < len; i++) {
		data[i] = buf[i + 1U];
	}

	k_busy_wait(10);

	return 0;
}

int pn7160_tml_spi_send(const struct device *dev, const uint8_t *data, size_t len)
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

	ret = pn7160_tml_spi_write(&cfg->spi, data, len);

	(void)k_mutex_unlock(&pdata->bus_mutex);

	return ret;
}

int pn7160_tml_spi_recv(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *pdata = dev->data;
	const bool dwl_mode = pdata->dwl_mode;
	uint8_t payload_len;
	size_t hdr_read_len;
	size_t footer_sz;
	size_t frame_len;
	int ret;

	if (data == NULL || out_len == NULL) {
		return -EINVAL;
	}

	hdr_read_len = pn7160_tml_hdr_read_len_get(dwl_mode);
	footer_sz = pn7160_tml_footer_sz_get(dwl_mode);

	if (max_len < hdr_read_len) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&pdata->bus_mutex, K_FOREVER);
	if (ret != 0) {
		return -EIO;
	}

	ret = pn7160_tml_spi_read(&cfg->spi, data, hdr_read_len);
	if (ret != 0) {
		ret = -EIO;
		goto unlock;
	}

	ret = pn7160_tml_frame_len_validate_mode(data, max_len, dwl_mode);
	if (ret != 0) {
		goto unlock;
	}

	payload_len = pn7160_tml_payload_len_get_mode(data, dwl_mode);
	frame_len = pn7160_tml_frame_len_get_mode(data, dwl_mode);

	if (payload_len > 0U) {
		ret = pn7160_tml_spi_read(&cfg->spi, &data[hdr_read_len],
					  (size_t)payload_len + footer_sz);
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

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
