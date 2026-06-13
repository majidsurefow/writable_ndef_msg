/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML framing unit tests (pure helpers, no bus I/O).
 */

#include <nfc/pn7160_tml.h>
#include <nfc/pn7160.h>

#include <zephyr/ztest.h>

ZTEST(pn7160_tml, test_valid_header_frame_len)
{
	const uint8_t hdr[] = { 0x40, 0x00, 0x03 };
	size_t frame_len;

	zassert_equal(pn7160_tml_payload_len_get(hdr), 3U);
	frame_len = pn7160_tml_frame_len_get(hdr);
	zassert_equal(frame_len, 6U);
	zassert_ok(pn7160_tml_frame_len_validate(hdr, 6U));
	zassert_ok(pn7160_tml_frame_len_validate(hdr, 258U));
}

ZTEST(pn7160_tml, test_oversize_reject)
{
	const uint8_t hdr[] = { 0x60, 0x00, 0xFF };

	zassert_equal(pn7160_tml_frame_len_get(hdr), 258U);
	zassert_equal(pn7160_tml_frame_len_validate(hdr, 257U), -EINVAL);
	zassert_equal(pn7160_tml_frame_len_validate(hdr, 100U), -EINVAL);
}

ZTEST(pn7160_tml, test_empty_reject)
{
	const uint8_t hdr[] = { 0x20, 0x00, 0x01 };

	zassert_equal(pn7160_tml_frame_len_validate(hdr, 0U), -EINVAL);
	zassert_equal(pn7160_tml_frame_len_validate(hdr, 2U), -EINVAL);
}

ZTEST(pn7160_tml, test_spi_xfer_lengths)
{
	zassert_equal(PN7160_TML_SPI_WRITE_PREFIX, 0x7FU);
	zassert_equal(PN7160_TML_SPI_READ_DUMMY, 0xFFU);
	zassert_equal(pn7160_tml_spi_write_xfer_len(4U), 5U);
	zassert_equal(pn7160_tml_spi_read_xfer_len(PN7160_TML_NCI_HDR_LEN), 4U);
	zassert_equal(pn7160_tml_spi_read_xfer_len(PN7160_TML_MAX_PAYLOAD_LEN),
		      PN7160_TML_MAX_PAYLOAD_LEN + 1U);
}

ZTEST(pn7160_tml, test_core_reset_ntf_fw_version)
{
	/* Minimal CORE_RESET_NTF layout: indices 9-11 hold FW version. */
	const uint8_t ntf[] = {
		0x60, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
		0x01, 0x02, 0x03,
	};
	uint8_t fw[3];

	zassert_ok(pn7160_nci_core_reset_ntf_fw_version(ntf, sizeof(ntf), fw));
	zassert_equal(fw[0], 0x01U);
	zassert_equal(fw[1], 0x02U);
	zassert_equal(fw[2], 0x03U);
	zassert_equal(pn7160_nci_core_reset_ntf_fw_version(ntf, 11U, fw), -EINVAL);
}

ZTEST(pn7160_tml, test_core_reset_ntf_invalid_oid)
{
	const uint8_t ntf[] = { 0x40, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
				0x01, 0x02, 0x03 };
	uint8_t fw[3];

	zassert_equal(pn7160_nci_core_reset_ntf_fw_version(ntf, sizeof(ntf), fw), -EINVAL);
}

ZTEST_SUITE(pn7160_tml, NULL, NULL, NULL, NULL, NULL);
