/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 Zephyr device driver — bus, GPIO (VEN/IRQ/DWL), IRQ → work signal.
 * DWL enter/leave logic derived from NXP tml_EnterDwlMode / tml_LeaveDwlMode.
 */

#define DT_DRV_COMPAT nxp_pn7160

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(pn7160, CONFIG_PN7160_LOG_LEVEL);

static struct k_work_q pn7160_work_q;
static K_KERNEL_STACK_DEFINE(pn7160_work_q_stack, CONFIG_PN7160_WORKQ_STACK_SIZE);
static bool pn7160_work_q_started;

static void pn7160_work_q_start(void)
{
	if (pn7160_work_q_started) {
		return;
	}

	k_work_queue_init(&pn7160_work_q);
	k_work_queue_start(&pn7160_work_q, pn7160_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(pn7160_work_q_stack),
			   CONFIG_PN7160_WORKQ_PRIORITY, NULL);
	pn7160_work_q_started = true;
}

static void pn7160_irq_rx_work_handler(struct k_work *work)
{
	struct pn7160_data *data = CONTAINER_OF(work, struct pn7160_data, irq_rx_work);
	const struct device *dev = data->self;
	const struct pn7160_config *cfg = dev->config;
	size_t rx_len = 0U;
	int ret;

	if (!atomic_cas(&data->rx_waiting, 1, 0)) {
		return;
	}

	(void)gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_DISABLE);

	ret = pn7160_tml_recv(dev, data->rx_buf, sizeof(data->rx_buf), &rx_len);
	if (ret == 0) {
		ret = pn7160_nci_process(dev, data->rx_buf, rx_len);
		atomic_clear(&data->irq_pending);
	}

	data->last_rx_err = ret;
	data->last_rx_len = rx_len;
	k_sem_give(&data->rx_sem);

	(void)gpio_pin_interrupt_configure_dt(&cfg->irq, GPIO_INT_EDGE_TO_ACTIVE);
}

static void pn7160_irq_work_handler(struct k_work *work)
{
	struct pn7160_data *data = CONTAINER_OF(work, struct pn7160_data, irq_work);

	k_work_submit_to_queue(&pn7160_work_q, &data->irq_rx_work);
}

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

void pn7160_submit_rx_work(const struct device *dev)
{
	struct pn7160_data *data = dev->data;

	k_work_submit_to_queue(&pn7160_work_q, &data->irq_rx_work);
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

	/* NXP tml_Reset: VEN low 10 ms, then high 10 ms. */
	(void)gpio_pin_set_dt(&cfg->ven, 0);
	k_msleep(10);
	(void)gpio_pin_set_dt(&cfg->ven, 1);
	k_msleep(10);

	return 0;
}

static bool pn7160_dwl_gpio_available(const struct pn7160_config *cfg)
{
	return cfg->dwl.port != NULL && gpio_is_ready_dt(&cfg->dwl);
}

int pn7160_dwl_enter(const struct device *dev)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *data = dev->data;

	if (!pn7160_dwl_gpio_available(cfg)) {
		return -ENOTSUP;
	}

	/* NXP tml_EnterDwlMode: isDwlMode=true, DWL high, VEN reset (10 ms / 10 ms). */
	data->dwl_mode = true;
	if (gpio_pin_set_dt(&cfg->dwl, 1) != 0) {
		return -EIO;
	}

	return pn7160_reset(dev);
}

int pn7160_dwl_leave(const struct device *dev)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *data = dev->data;

	if (!pn7160_dwl_gpio_available(cfg)) {
		return -ENOTSUP;
	}

	/* NXP tml_LeaveDwlMode: isDwlMode=false, DWL low, VEN reset (10 ms / 10 ms). */
	data->dwl_mode = false;
	if (gpio_pin_set_dt(&cfg->dwl, 0) != 0) {
		return -EIO;
	}

	return pn7160_reset(dev);
}

bool pn7160_dwl_mode_get(const struct device *dev)
{
	struct pn7160_data *data = dev->data;

	return data->dwl_mode;
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

const uint8_t *pn7160_fw_version_get(const struct device *dev)
{
	struct pn7160_data *data = dev->data;

	return data->fw_version;
}

static int pn7160_init(const struct device *dev)
{
	const struct pn7160_config *cfg = dev->config;
	struct pn7160_data *data = dev->data;
	int ret;

	pn7160_work_q_start();

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

	data->self = dev;
	k_mutex_init(&data->bus_mutex);
	k_mutex_init(&data->api_mutex);
	k_sem_init(&data->rx_sem, 0, 1);
	k_work_init(&data->irq_work, pn7160_irq_work_handler);
	k_work_init(&data->irq_rx_work, pn7160_irq_rx_work_handler);
	atomic_clear(&data->irq_pending);
	atomic_clear(&data->rx_waiting);
	data->last_rx_len = 0U;
	data->last_rx_err = 0;
	data->dwl_mode = false;
	data->rf_settings_restored = false;
	data->discovery_active = false;
	data->next_tag_protocol = PN7160_NCI_PROT_UNDETERMINED;
	memset(&data->last_rf_intf, 0, sizeof(data->last_rf_intf));
	data->fw_version[0] = 0U;
	data->fw_version[1] = 0U;
	data->fw_version[2] = 0U;

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
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                     \
		.tml_send = pn7160_tml_i2c_send,                                                   \
		.tml_recv = pn7160_tml_i2c_recv,                                                   \
		.irq = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                         \
		.ven = GPIO_DT_SPEC_INST_GET(inst, ven_gpios),                                         \
		.dwl = GPIO_DT_SPEC_INST_GET_OR(inst, dwl_gpios, {0}),                                         \
	}

#define PN7160_CONFIG_SPI(inst)                                                                    \
	{                                                                                          \
		.spi = SPI_DT_SPEC_INST_GET(inst, PN7160_SPI_OPERATION, 0),                        \
		.tml_send = pn7160_tml_spi_send,                                                   \
		.tml_recv = pn7160_tml_spi_recv,                                                   \
		.irq = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                         \
		.ven = GPIO_DT_SPEC_INST_GET(inst, ven_gpios),                                         \
		.dwl = GPIO_DT_SPEC_INST_GET_OR(inst, dwl_gpios, {0}),                                         \
	}

#define PN7160_DEFINE(inst)                                                                        \
	static const struct pn7160_config pn7160_config_##inst =                                   \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (PN7160_CONFIG_SPI(inst)),                  \
			    (PN7160_CONFIG_I2C(inst)));                                           \
	static struct pn7160_data pn7160_data_##inst;                                              \
	DEVICE_DT_INST_DEFINE(inst, pn7160_init, NULL, &pn7160_data_##inst, &pn7160_config_##inst, \
			    POST_KERNEL, CONFIG_PN7160_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PN7160_DEFINE)
