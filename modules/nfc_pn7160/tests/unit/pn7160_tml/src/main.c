/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 TML framing unit tests (pure helpers, no bus I/O).
 */

#include <nfc/pn7160_tml.h>

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

ZTEST_SUITE(pn7160_tml, NULL, NULL, NULL, NULL, NULL);
