/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PN7160_PRIV_H_
#define PN7160_PRIV_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

struct pn7160_config {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	const struct spi_dt_spec spi;
#endif
	const struct gpio_dt_spec irq;
	const struct gpio_dt_spec ven;
	const struct gpio_dt_spec dwl;
	int (*tml_send)(const struct device *dev, const uint8_t *data, size_t len);
	int (*tml_recv)(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len);
};

struct pn7160_data {
	const struct device *self;
	struct gpio_callback irq_cb;
	struct k_work irq_work;
	struct k_work irq_rx_work;
	struct k_mutex bus_mutex;
	struct k_sem rx_sem;
	atomic_t irq_pending;
	atomic_t rx_waiting;
	size_t last_rx_len;
	int last_rx_err;
	uint8_t fw_version[3];
	uint8_t rx_buf[CONFIG_PN7160_RX_BUF_SIZE];
};

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
int pn7160_tml_i2c_send(const struct device *dev, const uint8_t *data, size_t len);
int pn7160_tml_i2c_recv(const struct device *dev, uint8_t *data, size_t max_len,
			size_t *out_len);
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
int pn7160_tml_spi_send(const struct device *dev, const uint8_t *data, size_t len);
int pn7160_tml_spi_recv(const struct device *dev, uint8_t *data, size_t max_len,
			size_t *out_len);
#endif

int pn7160_tml_send(const struct device *dev, const uint8_t *data, size_t len);
int pn7160_tml_recv(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len);

int pn7160_nci_process(const struct device *dev, const uint8_t *rx, size_t rx_len);
void pn7160_nci_arm_rx(const struct device *dev);
void pn7160_submit_rx_work(const struct device *dev);

#endif /* PN7160_PRIV_H_ */
