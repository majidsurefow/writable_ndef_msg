/* Auto-generated from Flipper Slix_cap_missed.nfc — do not edit. */
#ifndef SLIX_FIXTURE_SLIX_CAP_MISSED_H_
#define SLIX_FIXTURE_SLIX_CAP_MISSED_H_

#include "nfc_session_mock.h"

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

static const uint8_t slix_Slix_cap_missed_model[] = {
0x01U, 0xE0U, 0x04U, 0x01U, 0x08U, 0x49U, 0xD0U, 0xDCU,
0x81U, 0x01U, 0x3DU, 0x01U, 0x01U, 0x01U, 0x50U, 0x00U,
0x04U, 0x03U, 0x0AU, 0x82U, 0xEDU, 0x86U, 0x39U, 0x61U,
0xD2U, 0x03U, 0x14U, 0x1EU, 0x32U, 0xB6U, 0xCAU, 0x00U,
0x3CU, 0x36U, 0x42U, 0x0CU, 0x33U, 0x53U, 0x30U, 0x37U,
0x32U, 0x32U, 0x34U, 0x30U, 0x30U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0xFFU, 0x04U, 0x01U, 0x01U, 0x00U, 0x00U,
0x00U, 0xA3U, 0x03U, 0x1EU, 0x00U, 0x26U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x0FU, 0x00U, 0x76U, 0x03U, 0x65U,
0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x85U, 0x01U, 0x34U,
0x00U, 0x75U, 0x09U, 0x05U, 0x00U, 0x01U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0xD7U, 0xFAU, 0x00U, 0x1CU, 0x9EU, 0x1CU, 0x67U,
0x27U, 0x00U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U, 0x30U,
0x30U, 0x30U, 0x30U, 0x30U, 0x00U, 0x00U, 0x00U, 0x97U,
0x25U, 0x55U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x32U, 0x8CU, 0x00U, 0x30U, 0x53U, 0x48U, 0x80U,
0xDEU, 0x00U, 0x00U, 0x00U, 0x00U, 0xF4U, 0xC3U, 0x58U,
0x2BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x11U, 0xF3U, 0x00U, 0x2CU, 0xDDU, 0xC3U, 0x3EU,
0x91U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xE5U, 0xFFU, 0x00U,
0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
0x00U, 0x53U, 0x4CU, 0x01U, 0x02U, 0x00U, 0x00U, 0x00U,
0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x0FU, 0x0FU, 0x0FU,
0x0FU, 0x0FU, 0x0FU, 0x0FU, 0x0FU, 0x00U, 0x00U, 0x00U,
0x00U, 0xA6U, 0x25U, 0x54U, 0x03U, 0x74U, 0x24U, 0xC4U,
0x38U, 0x36U, 0xF4U, 0x89U, 0x70U, 0x76U, 0x1AU, 0x72U,
0x27U, 0x54U, 0xD9U, 0xE7U, 0x3DU, 0x38U, 0xCBU, 0x4CU,
0x1BU, 0x3EU, 0xFDU, 0x0EU, 0xDFU, 0x8AU, 0xF6U, 0x7EU,
0x3DU, 0x00U, 0x32U, 0x02U, 0x01U, 0x01U,
};

#define SLIX_SLIX_CAP_MISSED_MODEL_LEN sizeof(slix_Slix_cap_missed_model)

static const uint8_t slix_Slix_cap_missed_step0_rx[] = {
0x00U, 0x01U, 0x81U, 0xDCU, 0xD0U, 0x49U, 0x08U, 0x01U,
0x04U, 0xE0U,
};

static const uint8_t slix_Slix_cap_missed_step1_rx[] = {
0x00U, 0x50U, 0x00U, 0x04U,
};

static const uint8_t slix_Slix_cap_missed_step2_rx[] = {
0x00U, 0x03U, 0x0AU, 0x82U, 0xEDU,
};

static const uint8_t slix_Slix_cap_missed_step3_rx[] = {
0x00U, 0x86U, 0x39U, 0x61U, 0xD2U,
};

static const uint8_t slix_Slix_cap_missed_step4_rx[] = {
0x00U, 0x03U, 0x14U, 0x1EU, 0x32U,
};

static const uint8_t slix_Slix_cap_missed_step5_rx[] = {
0x00U, 0xB6U, 0xCAU, 0x00U, 0x3CU,
};

static const uint8_t slix_Slix_cap_missed_step6_rx[] = {
0x00U, 0x36U, 0x42U, 0x0CU, 0x33U,
};

static const uint8_t slix_Slix_cap_missed_step7_rx[] = {
0x00U, 0x53U, 0x30U, 0x37U, 0x32U,
};

static const uint8_t slix_Slix_cap_missed_step8_rx[] = {
0x00U, 0x32U, 0x34U, 0x30U, 0x30U,
};

static const uint8_t slix_Slix_cap_missed_step9_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step10_rx[] = {
0x00U, 0x00U, 0xFFU, 0x04U, 0x01U,
};

static const uint8_t slix_Slix_cap_missed_step11_rx[] = {
0x00U, 0x01U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step12_rx[] = {
0x00U, 0xA3U, 0x03U, 0x1EU, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step13_rx[] = {
0x00U, 0x26U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step14_rx[] = {
0x00U, 0x00U, 0x00U, 0x0FU, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step15_rx[] = {
0x00U, 0x76U, 0x03U, 0x65U, 0x01U,
};

static const uint8_t slix_Slix_cap_missed_step16_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step17_rx[] = {
0x00U, 0x85U, 0x01U, 0x34U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step18_rx[] = {
0x00U, 0x75U, 0x09U, 0x05U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step19_rx[] = {
0x00U, 0x01U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step20_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step21_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step22_rx[] = {
0x00U, 0xD7U, 0xFAU, 0x00U, 0x1CU,
};

static const uint8_t slix_Slix_cap_missed_step23_rx[] = {
0x00U, 0x9EU, 0x1CU, 0x67U, 0x27U,
};

static const uint8_t slix_Slix_cap_missed_step24_rx[] = {
0x00U, 0x00U, 0x30U, 0x30U, 0x30U,
};

static const uint8_t slix_Slix_cap_missed_step25_rx[] = {
0x00U, 0x30U, 0x30U, 0x30U, 0x30U,
};

static const uint8_t slix_Slix_cap_missed_step26_rx[] = {
0x00U, 0x30U, 0x30U, 0x30U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step27_rx[] = {
0x00U, 0x00U, 0x00U, 0x97U, 0x25U,
};

static const uint8_t slix_Slix_cap_missed_step28_rx[] = {
0x00U, 0x55U, 0x08U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step29_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step30_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step31_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step32_rx[] = {
0x00U, 0x32U, 0x8CU, 0x00U, 0x30U,
};

static const uint8_t slix_Slix_cap_missed_step33_rx[] = {
0x00U, 0x53U, 0x48U, 0x80U, 0xDEU,
};

static const uint8_t slix_Slix_cap_missed_step34_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step35_rx[] = {
0x00U, 0xF4U, 0xC3U, 0x58U, 0x2BU,
};

static const uint8_t slix_Slix_cap_missed_step36_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step37_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step38_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step39_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step40_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step41_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step42_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step43_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step44_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step45_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step46_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step47_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step48_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step49_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step50_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step51_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step52_rx[] = {
0x00U, 0x11U, 0xF3U, 0x00U, 0x2CU,
};

static const uint8_t slix_Slix_cap_missed_step53_rx[] = {
0x00U, 0xDDU, 0xC3U, 0x3EU, 0x91U,
};

static const uint8_t slix_Slix_cap_missed_step54_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step55_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step56_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step57_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step58_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step59_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step60_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step61_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step62_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step63_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step64_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step65_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step66_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step67_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step68_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step69_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step70_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step71_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step72_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step73_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step74_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step75_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step76_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step77_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step78_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step79_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step80_rx[] = {
0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
};

static const uint8_t slix_Slix_cap_missed_step81_rx[] = {
0x00U, 0xE5U, 0xFFU, 0x00U, 0x01U,
};

static const uint8_t slix_Slix_cap_missed_step82_rx[] = {
0x00U, 0x0FU,
};

static const uint8_t slix_Slix_cap_missed_step83_rx[] = {
0xA6U, 0x25U, 0x54U, 0x03U, 0x74U, 0x24U, 0xC4U, 0x38U,
0x36U, 0xF4U, 0x89U, 0x70U, 0x76U, 0x1AU, 0x72U, 0x27U,
0x54U, 0xD9U, 0xE7U, 0x3DU, 0x38U, 0xCBU, 0x4CU, 0x1BU,
0x3EU, 0xFDU, 0x0EU, 0xDFU, 0x8AU, 0xF6U, 0x7EU, 0x3DU,
};

static const uint8_t slix_Slix_cap_missed_step84_rx[] = {
};

static const nfc_session_mock_step_t slix_Slix_cap_missed_read_steps[] = {
	{ .rx = slix_Slix_cap_missed_step0_rx, .rx_len = sizeof(slix_Slix_cap_missed_step0_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step1_rx, .rx_len = sizeof(slix_Slix_cap_missed_step1_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step2_rx, .rx_len = sizeof(slix_Slix_cap_missed_step2_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step3_rx, .rx_len = sizeof(slix_Slix_cap_missed_step3_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step4_rx, .rx_len = sizeof(slix_Slix_cap_missed_step4_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step5_rx, .rx_len = sizeof(slix_Slix_cap_missed_step5_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step6_rx, .rx_len = sizeof(slix_Slix_cap_missed_step6_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step7_rx, .rx_len = sizeof(slix_Slix_cap_missed_step7_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step8_rx, .rx_len = sizeof(slix_Slix_cap_missed_step8_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step9_rx, .rx_len = sizeof(slix_Slix_cap_missed_step9_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step10_rx, .rx_len = sizeof(slix_Slix_cap_missed_step10_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step11_rx, .rx_len = sizeof(slix_Slix_cap_missed_step11_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step12_rx, .rx_len = sizeof(slix_Slix_cap_missed_step12_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step13_rx, .rx_len = sizeof(slix_Slix_cap_missed_step13_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step14_rx, .rx_len = sizeof(slix_Slix_cap_missed_step14_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step15_rx, .rx_len = sizeof(slix_Slix_cap_missed_step15_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step16_rx, .rx_len = sizeof(slix_Slix_cap_missed_step16_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step17_rx, .rx_len = sizeof(slix_Slix_cap_missed_step17_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step18_rx, .rx_len = sizeof(slix_Slix_cap_missed_step18_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step19_rx, .rx_len = sizeof(slix_Slix_cap_missed_step19_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step20_rx, .rx_len = sizeof(slix_Slix_cap_missed_step20_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step21_rx, .rx_len = sizeof(slix_Slix_cap_missed_step21_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step22_rx, .rx_len = sizeof(slix_Slix_cap_missed_step22_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step23_rx, .rx_len = sizeof(slix_Slix_cap_missed_step23_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step24_rx, .rx_len = sizeof(slix_Slix_cap_missed_step24_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step25_rx, .rx_len = sizeof(slix_Slix_cap_missed_step25_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step26_rx, .rx_len = sizeof(slix_Slix_cap_missed_step26_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step27_rx, .rx_len = sizeof(slix_Slix_cap_missed_step27_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step28_rx, .rx_len = sizeof(slix_Slix_cap_missed_step28_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step29_rx, .rx_len = sizeof(slix_Slix_cap_missed_step29_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step30_rx, .rx_len = sizeof(slix_Slix_cap_missed_step30_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step31_rx, .rx_len = sizeof(slix_Slix_cap_missed_step31_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step32_rx, .rx_len = sizeof(slix_Slix_cap_missed_step32_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step33_rx, .rx_len = sizeof(slix_Slix_cap_missed_step33_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step34_rx, .rx_len = sizeof(slix_Slix_cap_missed_step34_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step35_rx, .rx_len = sizeof(slix_Slix_cap_missed_step35_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step36_rx, .rx_len = sizeof(slix_Slix_cap_missed_step36_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step37_rx, .rx_len = sizeof(slix_Slix_cap_missed_step37_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step38_rx, .rx_len = sizeof(slix_Slix_cap_missed_step38_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step39_rx, .rx_len = sizeof(slix_Slix_cap_missed_step39_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step40_rx, .rx_len = sizeof(slix_Slix_cap_missed_step40_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step41_rx, .rx_len = sizeof(slix_Slix_cap_missed_step41_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step42_rx, .rx_len = sizeof(slix_Slix_cap_missed_step42_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step43_rx, .rx_len = sizeof(slix_Slix_cap_missed_step43_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step44_rx, .rx_len = sizeof(slix_Slix_cap_missed_step44_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step45_rx, .rx_len = sizeof(slix_Slix_cap_missed_step45_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step46_rx, .rx_len = sizeof(slix_Slix_cap_missed_step46_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step47_rx, .rx_len = sizeof(slix_Slix_cap_missed_step47_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step48_rx, .rx_len = sizeof(slix_Slix_cap_missed_step48_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step49_rx, .rx_len = sizeof(slix_Slix_cap_missed_step49_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step50_rx, .rx_len = sizeof(slix_Slix_cap_missed_step50_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step51_rx, .rx_len = sizeof(slix_Slix_cap_missed_step51_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step52_rx, .rx_len = sizeof(slix_Slix_cap_missed_step52_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step53_rx, .rx_len = sizeof(slix_Slix_cap_missed_step53_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step54_rx, .rx_len = sizeof(slix_Slix_cap_missed_step54_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step55_rx, .rx_len = sizeof(slix_Slix_cap_missed_step55_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step56_rx, .rx_len = sizeof(slix_Slix_cap_missed_step56_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step57_rx, .rx_len = sizeof(slix_Slix_cap_missed_step57_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step58_rx, .rx_len = sizeof(slix_Slix_cap_missed_step58_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step59_rx, .rx_len = sizeof(slix_Slix_cap_missed_step59_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step60_rx, .rx_len = sizeof(slix_Slix_cap_missed_step60_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step61_rx, .rx_len = sizeof(slix_Slix_cap_missed_step61_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step62_rx, .rx_len = sizeof(slix_Slix_cap_missed_step62_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step63_rx, .rx_len = sizeof(slix_Slix_cap_missed_step63_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step64_rx, .rx_len = sizeof(slix_Slix_cap_missed_step64_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step65_rx, .rx_len = sizeof(slix_Slix_cap_missed_step65_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step66_rx, .rx_len = sizeof(slix_Slix_cap_missed_step66_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step67_rx, .rx_len = sizeof(slix_Slix_cap_missed_step67_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step68_rx, .rx_len = sizeof(slix_Slix_cap_missed_step68_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step69_rx, .rx_len = sizeof(slix_Slix_cap_missed_step69_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step70_rx, .rx_len = sizeof(slix_Slix_cap_missed_step70_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step71_rx, .rx_len = sizeof(slix_Slix_cap_missed_step71_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step72_rx, .rx_len = sizeof(slix_Slix_cap_missed_step72_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step73_rx, .rx_len = sizeof(slix_Slix_cap_missed_step73_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step74_rx, .rx_len = sizeof(slix_Slix_cap_missed_step74_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step75_rx, .rx_len = sizeof(slix_Slix_cap_missed_step75_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step76_rx, .rx_len = sizeof(slix_Slix_cap_missed_step76_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step77_rx, .rx_len = sizeof(slix_Slix_cap_missed_step77_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step78_rx, .rx_len = sizeof(slix_Slix_cap_missed_step78_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step79_rx, .rx_len = sizeof(slix_Slix_cap_missed_step79_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step80_rx, .rx_len = sizeof(slix_Slix_cap_missed_step80_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step81_rx, .rx_len = sizeof(slix_Slix_cap_missed_step81_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step82_rx, .rx_len = sizeof(slix_Slix_cap_missed_step82_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step83_rx, .rx_len = sizeof(slix_Slix_cap_missed_step83_rx), .err = 0 },
	{ .rx = slix_Slix_cap_missed_step84_rx, .rx_len = sizeof(slix_Slix_cap_missed_step84_rx), .err = 0 },
};

#define SLIX_SLIX_CAP_MISSED_READ_STEP_COUNT ARRAY_SIZE(slix_Slix_cap_missed_read_steps)

#endif
