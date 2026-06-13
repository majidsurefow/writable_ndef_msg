/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 *
 * PN7160 NCI pure parse helpers — unit-testable without bus I/O.
 */

#include <nfc/pn7160.h>

#define PN7160_NCI_CORE_RESET_NTF_MT_OID 0x60U
#define PN7160_NCI_CORE_RESET_OID        0x00U
#define PN7160_NCI_NTF_NCI_VERSION       0x20U

int pn7160_nci_core_reset_ntf_fw_version(const uint8_t *rx, size_t rx_len, uint8_t fw_ver[3])
{
	if (rx == NULL || fw_ver == NULL || rx_len < 12U) {
		return -EINVAL;
	}

	if (rx[0] != PN7160_NCI_CORE_RESET_NTF_MT_OID || rx[1] != PN7160_NCI_CORE_RESET_OID ||
	    rx[5] != PN7160_NCI_NTF_NCI_VERSION) {
		return -EINVAL;
	}

	fw_ver[0] = rx[9];
	fw_ver[1] = rx[10];
	fw_ver[2] = rx[11];

	return 0;
}
