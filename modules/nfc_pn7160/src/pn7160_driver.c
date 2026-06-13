/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 Zephyr device driver — bus, GPIO (VEN/IRQ/DWL), IRQ → work signal.
 * Phase 0 scaffold; TML logic in pn7160_tml_i2c.c / pn7160_tml_spi.c.
 */

#define DT_DRV_COMPAT nxp_pn7160

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pn7160, CONFIG_PN7160_LOG_LEVEL);

int pn7160_tml_send(const struct device *dev, const uint8_t *data, size_t len)
{
	const struct pn7160_config *cfg = dev->config;

	return cfg->tml_send(dev, data, len);
}

int pn7160_tml_recv(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len)
{
	const struct pn7160_config *cfg = dev->config;

	return cfg->tml_recv(dev, data, max_len, out_len);
}

static void pn7160_irq_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	/* Phase 0.6: drain RX on nfc_work_q; scaffold only sets flag. */
}

static void pn7160_gpio_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct pn7160_data *data = CONTAINER_OF(cb, struct pn7160_data, irq_cb);

	atomic_set(&data->irq_pending, 1);
	k_work_submit(&data->irq_work);
}

static bool pn7160_bus_is_ready(const struct pn7160_config *cfg)
{
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	if (cfg->tml_send == pn7160_tml_i2c_send) {
		return i2c_is_ready_dt(&cfg->i2c);
	}
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	if (cfg->tml_send == pn7160_tml_spi_send) {
		return spi_is_ready_dt(&cfg->spi);
	}
#endif
	return false;
}

int pn7160_reset(const struct device *dev)
{
	const struct pn7160_config *cfg = dev->config;

	if (!gpio_is_ready_dt(&cfg->ven)) {
		return -ENODEV;
	}

	/* VEN low → high power-on sequence (NXP tml_Connect). */
	(void)gpio_pin_set_dt(&cfg->ven, 0);
	k_msleep(1);
	(void)gpio_pin_set_dt(&cfg->ven, 1);
	k_msleep(3);

	return 0;
}

int pn7160_wait_irq(const struct device *dev, k_timeout_t timeout)
{
	struct pn7160_data *data = dev->data;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (!atomic_cas(&data->irq_pending, 1, 0)) {
		if (sys_timepoint_expired(end)) {
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(1));
	}

	return 0;
}

static int pn7160_init(const struct device *dev)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *data = dev->data;
	int ret;

	if (!pn7160_bus_is_ready(cfg)) {
		LOG_ERR("bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->irq) || !gpio_is_ready_dt(&cfg->ven)) {
		LOG_ERR("GPIO not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&cfg->irq, GPIO_INPUT) != 0) {
		return -EIO;
	}

	if (gpio_pin_configure_dt(&cfg->ven, GPIO_OUTPUT_INACTIVE) != 0) {
		return -EIO;
	}

	if (cfg->dwl.port != NULL && gpio_is_ready_dt(&cfg->dwl)) {
		(void)gpio_pin_configure_dt(&cfg->dwl, GPIO_OUTPUT_INACTIVE);
	}

	k_work_init(&data->irq_work, pn7160_irq_work_handler);
	atomic_clear(&data->irq_pending);

	gpio_init_callback(&data->irq_cb, pn7160_gpio_isr, BIT(cfg->irq.pin));
	ret = gpio_add_callback(cfg->irq.port, &data->irq_cb);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	return pn7160_reset(dev);
}

#define PN7160_SPI_OPERATION                                                                       \
	(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

#define PN7160_CONFIG_I2C(inst)                                                                    \
	{                                                                                          \
		.i2c = I2C_DT_SPEC_INST(inst),                                                     \
		.tml_send = pn7160_tml_i2c_send,                                                   \
		.tml_recv = pn7160_tml_i2c_recv,                                                   \
		.irq = GPIO_DT_SPEC_INST(inst, irq_gpios),                                         \
		.ven = GPIO_DT_SPEC_INST(inst, ven_gpios),                                         \
		.dwl = GPIO_DT_SPEC_INST(inst, dwl_gpios),                                         \
	}

#define PN7160_CONFIG_SPI(inst)                                                                    \
	{                                                                                          \
		.spi = SPI_DT_SPEC_INST_GET(inst, PN7160_SPI_OPERATION, 0),                        \
		.tml_send = pn7160_tml_spi_send,                                                   \
		.tml_recv = pn7160_tml_spi_recv,                                                   \
		.irq = GPIO_DT_SPEC_INST(inst, irq_gpios),                                         \
		.ven = GPIO_DT_SPEC_INST(inst, ven_gpios),                                         \
		.dwl = GPIO_DT_SPEC_INST(inst, dwl_gpios),                                         \
	}

#define PN7160_DEFINE(inst)                                                                        \
	static const struct pn7160_config pn7160_config_##inst =                                   \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (PN7160_CONFIG_SPI(inst)),                  \
			    (PN7160_CONFIG_I2C(inst)));                                           \
	static struct pn7160_data pn7160_data_##inst;                                              \
	DEVICE_DT_INST_DEFINE(inst, pn7160_init, NULL, &pn7160_data_##inst, &pn7160_config_##inst, \
			    POST_KERNEL, CONFIG_PN7160_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PN7160_DEFINE)
