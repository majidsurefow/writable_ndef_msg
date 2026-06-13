/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP PN7160 NFC controller — public driver / TML / NCI API (Phase 0 scaffold).
 */

#ifndef NFC_PN7160_H_
#define NFC_PN7160_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief PN7160 device handle from devicetree. */
#define PN7160_DEVICE DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pn7160)

struct pn7160_tml_ops {
	int (*send)(const struct device *dev, const uint8_t *data, size_t len);
	int (*recv)(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len);
};

/** Reset and power the controller via VEN. */
int pn7160_reset(const struct device *dev);

/** Wait for HOST_IRQ with timeout (ms). */
int pn7160_wait_irq(const struct device *dev, k_timeout_t timeout);

/** NCI: probe controller with CORE_RESET (Phase 0.3). */
int pn7160_nci_check_dev_pres(const struct device *dev);

/** NCI: send frame and await response (Phase 0.4). */
int pn7160_nci_host_transceive(const struct device *dev, const uint8_t *tx, size_t tx_len,
			       uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PN7160_H_ */
