/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP PN7160 NFC controller — public driver / TML / NCI API (Phase 0).
 */

#ifndef NFC_PN7160_H_
#define NFC_PN7160_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup pn7160_api PN7160 driver API
 * @{
 */

/**
 * @brief PN7160 device handle from devicetree.
 *
 * Resolves the first status-okay `nxp,pn7160` instance. Callers must verify
 * @ref device_is_ready() before any other API use.
 */
#define PN7160_DEVICE DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pn7160)

/**
 * @brief Reset and power the controller via VEN.
 *
 * Applies the NXP 10 ms / 10 ms VEN sequence (inactive, then active).
 *
 * @param dev PN7160 device.
 * @return 0 on success, negative errno on GPIO failure.
 */
int pn7160_reset(const struct device *dev);

/**
 * @brief Enter firmware-download mode (DWL active + VEN reset).
 *
 * Port of NXP `tml_EnterDwlMode()`: sets download TML framing (1-byte header,
 * 2-byte footer on read), asserts **DWL_REQ** high, then applies the standard
 * VEN reset sequence (inactive 10 ms, active 10 ms).
 *
 * Requires `dwl-gpios` in devicetree.
 *
 * @param dev PN7160 device.
 * @return 0 on success, `-ENOTSUP` if DWL GPIO is absent, negative errno on GPIO failure.
 */
int pn7160_dwl_enter(const struct device *dev);

/**
 * @brief Leave firmware-download mode (DWL inactive + VEN reset).
 *
 * Port of NXP `tml_LeaveDwlMode()`: clears download TML framing, deasserts
 * **DWL_REQ**, then applies the VEN reset sequence (10 ms / 10 ms).
 *
 * @param dev PN7160 device.
 * @return 0 on success, `-ENOTSUP` if DWL GPIO is absent, negative errno on GPIO failure.
 */
int pn7160_dwl_leave(const struct device *dev);

/**
 * @brief Return whether download-mode TML framing is active.
 *
 * @param dev PN7160 device.
 * @return True after @ref pn7160_dwl_enter until @ref pn7160_dwl_leave.
 */
bool pn7160_dwl_mode_get(const struct device *dev);

/**
 * @brief Wait for HOST_IRQ assertion with timeout.
 *
 * Clears the driver IRQ-pending flag set by the GPIO ISR. Used internally by
 * legacy paths; @ref pn7160_nci_transceive uses IRQ-driven work-queue drain.
 *
 * @param dev PN7160 device.
 * @param timeout Maximum wait time.
 * @return 0 when IRQ was pending, `-ETIMEDOUT` on timeout.
 */
int pn7160_wait_irq(const struct device *dev, k_timeout_t timeout);

/**
 * @brief Probe controller presence with NCI CORE_RESET.
 *
 * Sends CORE_RESET, receives CORE_RESET_RSP and optional CORE_RESET_NTF,
 * and caches the 3-byte firmware version when present.
 *
 * @param dev PN7160 device (must be ready).
 * @return 0 on successful probe, negative errno otherwise.
 */
int pn7160_nci_check_dev_pres(const struct device *dev);

/**
 * @brief Connect to the controller (CORE_RESET probe + CORE_INIT).
 *
 * Port of NXP `NxpNci_Connect()`: probes with retries, then sends
 * CORE_INIT `{20 01 02 00 00}` and validates CORE_INIT_RSP.
 *
 * @param dev PN7160 device (must be ready).
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_connect(const struct device *dev);

/**
 * @brief Return cached 3-byte firmware version from last CORE_RESET_NTF.
 *
 * Bytes may be zero until @ref pn7160_nci_check_dev_pres succeeds.
 *
 * @param dev PN7160 device.
 * @return Pointer to three version bytes (stable for device lifetime).
 */
const uint8_t *pn7160_fw_version_get(const struct device *dev);

/**
 * @brief Validate an NCI CORE_RESET_RSP frame.
 *
 * @param rx Received NCI frame.
 * @param rx_len Frame length in bytes.
 * @return 0 on success, `-EINVAL` on parse failure, `-EIO` on NCI error status.
 */
int pn7160_nci_core_reset_rsp_validate(const uint8_t *rx, size_t rx_len);

/**
 * @brief Parse 3-byte firmware version from a CORE_RESET_NTF frame.
 *
 * Pure helper for unit tests and HAL use.
 *
 * @param rx Received NCI frame.
 * @param rx_len Frame length in bytes.
 * @param fw_ver Output buffer (3 bytes).
 * @return 0 on success, `-EINVAL` on parse failure.
 */
int pn7160_nci_core_reset_ntf_fw_version(const uint8_t *rx, size_t rx_len, uint8_t fw_ver[3]);

/**
 * @brief Send an NCI command and receive the response (IRQ-driven).
 *
 * Transmits @p tx over TML, then blocks until the module work queue drains
 * HOST_IRQ (TML receive + NCI process) or @p timeout expires.
 *
 * @param dev PN7160 device.
 * @param tx NCI command buffer.
 * @param tx_len Command length.
 * @param rx Response buffer.
 * @param rx_max Response buffer capacity.
 * @param rx_len Actual response length on success.
 * @param timeout Maximum wait for the response.
 * @return 0 on success, negative errno on bus/timeout/parse failure.
 */
int pn7160_nci_transceive(const struct device *dev, const uint8_t *tx, size_t tx_len,
			  uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout);

/**
 * @brief Alias for @ref pn7160_nci_transceive (NXP HostTransceive port name).
 */
int pn7160_nci_host_transceive(const struct device *dev, const uint8_t *tx, size_t tx_len,
			       uint8_t *rx, size_t rx_max, size_t *rx_len, k_timeout_t timeout);

/**
 * @brief Apply NXP RF / core settings blobs (NxpNci_ConfigureSettings).
 *
 * Sends CORE_STANDBY, CORE_CONF, and conditionally CLK/TVDD/RF blobs from
 * Nfc_settings.h. Re-runs CORE_RESET + CORE_INIT when a controller reset is
 * required. Call after @ref pn7160_nci_connect().
 *
 * @param dev PN7160 device.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_configure_settings(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NFC_PN7160_H_ */
