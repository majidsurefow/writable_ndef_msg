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

/** NCI mode / technology flags (NXP Nfc.h). */
#define PN7160_NCI_INTF_UNDETERMINED 0x0U
#define PN7160_NCI_INTF_FRAME        0x1U
#define PN7160_NCI_INTF_ISODEP       0x2U
#define PN7160_NCI_INTF_NFCDEP       0x3U
#define PN7160_NCI_INTF_TAGCMD       0x80U

#define PN7160_NCI_PROT_UNDETERMINED 0x0U
#define PN7160_NCI_PROT_T1T          0x1U
#define PN7160_NCI_PROT_T2T          0x2U
#define PN7160_NCI_PROT_T3T          0x3U
#define PN7160_NCI_PROT_ISODEP       0x4U
#define PN7160_NCI_PROT_NFCDEP       0x5U
#define PN7160_NCI_PROT_T5T          0x6U
#define PN7160_NCI_PROT_MIFARE       0x80U

#define PN7160_NCI_MODE_POLL         0x00U
#define PN7160_NCI_MODE_LISTEN       0x80U
#define PN7160_NCI_MODE_MASK         0x80U
#define PN7160_NCI_TECH_PASSIVE_NFCA 0U
#define PN7160_NCI_TECH_PASSIVE_NFCB 1U
#define PN7160_NCI_TECH_PASSIVE_NFCF 2U
#define PN7160_NCI_TECH_PASSIVE_15693 6U
#define PN7160_NCI_MODE_RW           (1U << 2)
#define PN7160_NCI_MODE_CARDEMU      (1U << 0)

/** Discovered remote tag summary (UID + protocol). */
struct pn7160_nci_rf_intf {
	uint8_t interface;
	uint8_t protocol;
	uint8_t mode_tech;
	bool more_tags;
	uint8_t uid_len;
	uint8_t uid[10];
};

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

/**
 * @brief Configure discovery map and routing for the given mode bitmask.
 *
 * Supports @ref PN7160_NCI_MODE_RW and/or @ref PN7160_NCI_MODE_CARDEMU (NXP
 * `NxpNci_ConfigureMode`).
 *
 * @param dev PN7160 device.
 * @param mode NXP NXPNCI_MODE_* flags.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_configure_mode(const struct device *dev, uint8_t mode);

/**
 * @brief Start RF discovery (ConfigureMode RW + RF_DISCOVER_CMD).
 *
 * Uses @p tech_tab or the default NFC-A / NFC-B / ISO15693 poll table when NULL.
 *
 * @param dev PN7160 device.
 * @param tech_tab Mode|tech entries (see PN7160_NCI_MODE_POLL | PN7160_NCI_TECH_*).
 * @param tech_count Number of entries in @p tech_tab.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_discovery_start(const struct device *dev, const uint8_t *tech_tab,
			       size_t tech_count);

/**
 * @brief Stop RF discovery loop.
 *
 * @param dev PN7160 device.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_discovery_stop(const struct device *dev);

/**
 * @brief Wait for a discovery notification (IRQ-driven, WQ drain).
 *
 * Blocks until RF_DISCOVER_NTF / RF_INTF_ACTIVATED_NTF or @p timeout.
 *
 * @param dev PN7160 device.
 * @param rf_intf Output tag summary (UID when available).
 * @param timeout Maximum wait time.
 * @return 0 on success, `-ETIMEDOUT`, or other negative errno.
 */
int pn7160_nci_discovery_wait(const struct device *dev, struct pn7160_nci_rf_intf *rf_intf,
			      k_timeout_t timeout);

/**
 * @brief Return whether discovery is active after @ref pn7160_nci_discovery_start.
 */
bool pn7160_nci_discovery_active(const struct device *dev);

/**
 * @brief Return the last tag summary cached by @ref pn7160_nci_discovery_wait.
 */
const struct pn7160_nci_rf_intf *pn7160_nci_discovery_last(const struct device *dev);

/**
 * @brief Send a raw tag command on the active RF interface (NxpNci_ReaderTagCmd).
 *
 * Wraps NCI DATA_PACKET (0x00 0x00 len + payload), then waits for the tag
 * response DATA_PACKET. Requires an activated tag from discovery.
 *
 * @param dev PN7160 device.
 * @param cmd Tag command payload (e.g. MIFARE READ).
 * @param cmd_len Command length.
 * @param answer Response buffer.
 * @param answer_max Response buffer capacity.
 * @param answer_len Actual response length on success.
 * @param timeout Maximum wait per NCI exchange.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_reader_tag_cmd(const struct device *dev, const uint8_t *cmd, size_t cmd_len,
			      uint8_t *answer, size_t answer_max, size_t *answer_len,
			      k_timeout_t timeout);

/**
 * @brief Start card-emulation listen discovery (ConfigureMode CARDEMU + RF_DISCOVER).
 *
 * Uses @p tech_tab or the default NFC-A / NFC-B listen table from NXP RWandCE when NULL.
 *
 * @param dev PN7160 device.
 * @param tech_tab Mode|tech entries (see PN7160_NCI_MODE_LISTEN | PN7160_NCI_TECH_*).
 * @param tech_count Number of entries in @p tech_tab.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_listen_start(const struct device *dev, const uint8_t *tech_tab, size_t tech_count);

/**
 * @brief Stop card-emulation listen discovery loop.
 *
 * Alias of @ref pn7160_nci_discovery_stop.
 */
int pn7160_nci_listen_stop(const struct device *dev);

/**
 * @brief Return whether listen discovery is active after @ref pn7160_nci_listen_start.
 */
bool pn7160_nci_listen_active(const struct device *dev);

/**
 * @brief Start combined reader + card-emulation discovery (RWandCE table).
 *
 * Configures @ref PN7160_NCI_MODE_RW | @ref PN7160_NCI_MODE_CARDEMU and starts the
 * six-entry poll + listen table from NXP `nfc_example_RWandCE.c`.
 */
int pn7160_nci_rw_ce_discovery_start(const struct device *dev);

/**
 * @brief Wait for a card-mode DATA_PACKET from a remote reader (NxpNci_CardModeReceive).
 *
 * Blocks until an NCI DATA_PACKET (`00 00 len payload`) arrives or @p timeout expires.
 *
 * @param dev PN7160 device.
 * @param data Payload buffer.
 * @param data_max Payload buffer capacity.
 * @param data_len Actual payload length on success.
 * @param timeout Maximum wait time.
 * @return 0 on success, `-ETIMEDOUT`, `-EIO` if the frame is not a DATA_PACKET, or other errno.
 */
int pn7160_nci_card_mode_recv(const struct device *dev, uint8_t *data, size_t data_max,
			      size_t *data_len, k_timeout_t timeout);

/**
 * @brief Send a card-mode DATA_PACKET response (NxpNci_CardModeSend).
 *
 * Wraps payload in NCI DATA_PACKET (`00 00 len + payload`) and transceives.
 *
 * @param dev PN7160 device.
 * @param data Payload to send.
 * @param data_len Payload length (max 255 bytes per NCI length byte).
 * @param timeout Maximum wait per NCI exchange.
 * @return 0 on success, negative errno otherwise.
 */
int pn7160_nci_card_mode_send(const struct device *dev, const uint8_t *data, size_t data_len,
			      k_timeout_t timeout);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NFC_PN7160_H_ */
