/*
 * Copyright (c) 2026 NXP Semiconductors
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML SPI framing — port of NXP source/TML/tml.c (SPI path).
 * Phase 0 scaffold: structure and API only; full NCI traffic gated on HIL.
 */

#define DT_DRV_COMPAT nxp_pn7160

#include <nfc/pn7160.h>
#include "pn7160_priv.h"

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(pn7160);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define PN7160_SPI_WRITE_PREFIX 0x7F
#define PN7160_SPI_READ_DUMMY   0xFF

#define PN7160_SPI_OPERATION                                                                   \
	(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8))

int pn7160_tml_spi_send(const struct device *dev, const uint8_t *data, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	/* NXP INTF_WRITE: [0x7F][payload...] in one CS-held transfer — Phase 0.2. */
	LOG_DBG("SPI TML send not implemented (scaffold)");
	return -ENOTSUP;
}

int pn7160_tml_spi_recv(const struct device *dev, uint8_t *data, size_t max_len, size_t *out_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data);
	ARG_UNUSED(max_len);
	ARG_UNUSED(out_len);

	/* NXP INTF_READ: full-duplex [0xFF][rx...], drop first byte, 10 us delay — Phase 0.2. */
	LOG_DBG("SPI TML recv not implemented (scaffold)");
	return -ENOTSUP;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
