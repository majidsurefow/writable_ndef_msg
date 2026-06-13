/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML framing helpers — testable pure functions (NXP tml_Rx logic).
 */

#ifndef NFC_PN7160_TML_H_
#define NFC_PN7160_TML_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NCI header prefix size before payload-length byte (NXP HEADER_SZ). */
#define PN7160_TML_HEADER_SZ 2U

/** Total NCI/TML header read before payload (HEADER_SZ + length byte). */
#define PN7160_TML_NCI_HDR_LEN (PN7160_TML_HEADER_SZ + 1U)

/** Maximum NCI payload length per PN7160 TML (255). */
#define PN7160_TML_MAX_PAYLOAD_LEN 255U

/** SPI TML write prefix byte (NXP INTF_WRITE). */
#define PN7160_TML_SPI_WRITE_PREFIX 0x7FU

/** SPI TML read dummy byte (NXP INTF_READ). */
#define PN7160_TML_SPI_READ_DUMMY 0xFFU

/**
 * @brief Total SPI write transfer length for a TML payload.
 *
 * @param payload_len NCI frame length excluding the 0x7F prefix.
 * @return Bytes on wire including prefix.
 */
static inline size_t pn7160_tml_spi_write_xfer_len(size_t payload_len)
{
	return payload_len + 1U;
}

/**
 * @brief Total SPI read transfer length for @p data_len payload bytes.
 *
 * @param data_len Bytes to clock out after the leading 0xFF dummy.
 * @return Bytes on wire including dummy.
 */
static inline size_t pn7160_tml_spi_read_xfer_len(size_t data_len)
{
	return data_len + 1U;
}

/**
 * @brief Extract NCI payload length from the first three header bytes.
 *
 * @param hdr At least 3 bytes: NCI MT/GID, OID, payload length.
 * @return Payload length byte (hdr[2]).
 */
static inline uint8_t pn7160_tml_payload_len_get(const uint8_t hdr[PN7160_TML_NCI_HDR_LEN])
{
	return hdr[PN7160_TML_HEADER_SZ];
}

/**
 * @brief Compute total received frame length from a TML/NCI header.
 *
 * Matches NXP tml_Rx: total = payload_len + HEADER_SZ + 1.
 *
 * @param hdr At least 3 bytes of NCI header.
 * @return Full frame size in bytes.
 */
static inline size_t pn7160_tml_frame_len_get(const uint8_t hdr[PN7160_TML_NCI_HDR_LEN])
{
	return (size_t)pn7160_tml_payload_len_get(hdr) + PN7160_TML_NCI_HDR_LEN;
}

/**
 * @brief Validate that a TML/NCI frame fits in the receive buffer.
 *
 * @param hdr At least 3 bytes of NCI header.
 * @param max_len Caller buffer capacity.
 * @return 0 if valid, negative errno otherwise.
 */
int pn7160_tml_frame_len_validate(const uint8_t hdr[PN7160_TML_NCI_HDR_LEN], size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* NFC_PN7160_TML_H_ */
