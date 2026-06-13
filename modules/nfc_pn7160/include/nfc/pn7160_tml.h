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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** NCI header prefix size before payload-length byte (NXP HEADER_SZ, normal mode). */
#define PN7160_TML_HEADER_SZ 2U

/** Download-mode header prefix size (NXP HEADER_SZ when isDwlMode). */
#define PN7160_TML_DWL_HEADER_SZ 1U

/** Download-mode footer bytes read after payload (NXP FOOTER_SZ when isDwlMode). */
#define PN7160_TML_DWL_FOOTER_SZ 2U

/** Total NCI/TML header read before payload (HEADER_SZ + length byte). */
#define PN7160_TML_NCI_HDR_LEN (PN7160_TML_HEADER_SZ + 1U)

/** Total download-mode header read before payload (1-byte HEADER_SZ + length). */
#define PN7160_TML_DWL_HDR_LEN (PN7160_TML_DWL_HEADER_SZ + 1U)

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
 * @brief TML header prefix size for normal NCI or download mode.
 *
 * Port of NXP tml.c: HEADER_SZ = isDwlMode ? 1 : 2.
 *
 * @param dwl_mode True when firmware-download framing is active.
 * @return Header prefix byte count before the payload-length field.
 */
static inline size_t pn7160_tml_header_sz_get(bool dwl_mode)
{
	return dwl_mode ? PN7160_TML_DWL_HEADER_SZ : PN7160_TML_HEADER_SZ;
}

/**
 * @brief TML footer size appended after payload on read (download mode only).
 *
 * Port of NXP tml.c: FOOTER_SZ = isDwlMode ? 2 : 0.
 *
 * @param dwl_mode True when firmware-download framing is active.
 * @return Footer byte count read after payload (not included in frame length).
 */
static inline size_t pn7160_tml_footer_sz_get(bool dwl_mode)
{
	return dwl_mode ? PN7160_TML_DWL_FOOTER_SZ : 0U;
}

/**
 * @brief Bytes to read for the initial TML header segment.
 *
 * @param dwl_mode True when firmware-download framing is active.
 * @return HEADER_SZ + 1 (length byte included).
 */
static inline size_t pn7160_tml_hdr_read_len_get(bool dwl_mode)
{
	return pn7160_tml_header_sz_get(dwl_mode) + 1U;
}

/**
 * @brief Extract payload length from a TML header prefix.
 *
 * @param hdr Header buffer (at least @ref pn7160_tml_hdr_read_len_get bytes).
 * @param dwl_mode True when firmware-download framing is active.
 * @return Payload length byte at hdr[HEADER_SZ].
 */
static inline uint8_t pn7160_tml_payload_len_get_mode(const uint8_t *hdr, bool dwl_mode)
{
	return hdr[pn7160_tml_header_sz_get(dwl_mode)];
}

/**
 * @brief Compute total received frame length from a TML header prefix.
 *
 * Matches NXP tml_Rx: total = payload_len + HEADER_SZ + 1 (footer excluded).
 *
 * @param hdr Header buffer (at least @ref pn7160_tml_hdr_read_len_get bytes).
 * @param dwl_mode True when firmware-download framing is active.
 * @return Full frame size in bytes (excluding download footer).
 */
static inline size_t pn7160_tml_frame_len_get_mode(const uint8_t *hdr, bool dwl_mode)
{
	return (size_t)pn7160_tml_payload_len_get_mode(hdr, dwl_mode) +
	       pn7160_tml_hdr_read_len_get(dwl_mode);
}

/**
 * @brief Extract NCI payload length from the first three header bytes.
 *
 * @param hdr At least 3 bytes: NCI MT/GID, OID, payload length.
 * @return Payload length byte (hdr[2]).
 */
static inline uint8_t pn7160_tml_payload_len_get(const uint8_t hdr[PN7160_TML_NCI_HDR_LEN])
{
	return pn7160_tml_payload_len_get_mode(hdr, false);
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
	return pn7160_tml_frame_len_get_mode(hdr, false);
}

/**
 * @brief Validate that a TML frame fits in the receive buffer.
 *
 * @param hdr Header buffer (at least @ref pn7160_tml_hdr_read_len_get bytes).
 * @param max_len Caller buffer capacity.
 * @param dwl_mode True when firmware-download framing is active.
 * @return 0 if valid, negative errno otherwise.
 */
int pn7160_tml_frame_len_validate_mode(const uint8_t *hdr, size_t max_len, bool dwl_mode);

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
