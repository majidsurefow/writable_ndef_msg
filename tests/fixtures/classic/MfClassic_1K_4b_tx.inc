/* Tier B poller TX golden — one STEP per transceive */
/* STEP 0: classic TX detect 4k probe */
/* classic TX detect 4k probe */
0x60U, 0xFEU,
/* STEP 1: classic TX detect 1k probe */
/* classic TX detect 1k probe */
0x60U, 0x3EU,
/* STEP 2: classic TX AUTH NT sector 0 */
/* classic TX AUTH NT sector 0 */
0x60U, 0x00U,
/* STEP 3: classic TX AUTH NR|AR sector 0 */
/* classic TX AUTH NR|AR sector 0 */
0x91U, 0x07U, 0xBDU, 0x3AU, 0xFFU, 0xF9U, 0xF4U, 0x82U,
/* STEP 4: classic TX READ block 0 */
/* classic TX READ block 0 */
0x2CU, 0x7AU, 0xFBU, 0xB1U,
/* STEP 5: classic TX READ block 1 */
/* classic TX READ block 1 */
0xE9U, 0x7EU, 0x7AU, 0xD1U,
/* STEP 6: classic TX READ block 2 */
/* classic TX READ block 2 */
0xC5U, 0x81U, 0xB3U, 0x0AU,
/* STEP 7: classic TX READ block 3 */
/* classic TX READ block 3 */
0x31U, 0xA8U, 0x0FU, 0x8EU,
/* STEP 8: classic TX AUTH NT sector 1 */
/* classic TX AUTH NT sector 1 */
0x60U, 0x04U,
/* STEP 9: classic TX AUTH NR|AR sector 1 */
/* classic TX AUTH NR|AR sector 1 */
0x11U, 0x3FU, 0xD3U, 0x33U, 0xD3U, 0x66U, 0x90U, 0xCBU,
/* STEP 10: classic TX READ block 4 */
/* classic TX READ block 4 */
0x9BU, 0xA7U, 0xC8U, 0x14U,
/* STEP 11: classic TX READ block 5 */
/* classic TX READ block 5 */
0x60U, 0xCBU, 0xA5U, 0xA7U,
/* STEP 12: classic TX READ block 6 */
/* classic TX READ block 6 */
0x0BU, 0xABU, 0xAEU, 0x26U,
/* STEP 13: classic TX READ block 7 */
/* classic TX READ block 7 */
0xDAU, 0x79U, 0x8FU, 0x7EU,
/* STEP 14: classic TX AUTH NT sector 2 */
/* classic TX AUTH NT sector 2 */
0x60U, 0x08U,
/* STEP 15: classic TX AUTH NR|AR sector 2 */
/* classic TX AUTH NR|AR sector 2 */
0x91U, 0xC8U, 0x59U, 0x9EU, 0x0EU, 0x56U, 0xFDU, 0xDFU,
/* STEP 16: classic TX READ block 8 */
/* classic TX READ block 8 */
0xDFU, 0x7DU, 0x4BU, 0xE2U,
/* STEP 17: classic TX READ block 9 */
/* classic TX READ block 9 */
0x51U, 0x8DU, 0x94U, 0xCBU,
/* STEP 18: classic TX READ block 10 */
/* classic TX READ block 10 */
0x5DU, 0x8AU, 0x02U, 0x55U,
/* STEP 19: classic TX READ block 11 */
/* classic TX READ block 11 */
0x11U, 0xB6U, 0xAAU, 0x2EU,
/* STEP 20: classic TX AUTH NT sector 3 */
/* classic TX AUTH NT sector 3 */
0x60U, 0x0CU,
/* STEP 21: classic TX AUTH NR|AR sector 3 */
/* classic TX AUTH NR|AR sector 3 */
0x59U, 0xDAU, 0xDFU, 0xFEU, 0x61U, 0x21U, 0xE7U, 0x37U,
/* STEP 22: classic TX READ block 12 */
/* classic TX READ block 12 */
0x83U, 0x14U, 0x57U, 0x88U,
/* STEP 23: classic TX READ block 13 */
/* classic TX READ block 13 */
0x3EU, 0x15U, 0x84U, 0x57U,
/* STEP 24: classic TX READ block 14 */
/* classic TX READ block 14 */
0x2FU, 0x4DU, 0xD9U, 0x50U,
/* STEP 25: classic TX READ block 15 */
/* classic TX READ block 15 */
0xA8U, 0xD7U, 0xB2U, 0x70U,
/* STEP 26: classic TX AUTH NT sector 4 */
/* classic TX AUTH NT sector 4 */
0x60U, 0x10U,
/* STEP 27: classic TX AUTH NR|AR sector 4 */
/* classic TX AUTH NR|AR sector 4 */
0xB3U, 0x88U, 0xF7U, 0xE4U, 0x11U, 0x64U, 0xB4U, 0x7AU,
/* STEP 28: classic TX READ block 16 */
/* classic TX READ block 16 */
0x57U, 0xA2U, 0x8EU, 0x48U,
/* STEP 29: classic TX READ block 17 */
/* classic TX READ block 17 */
0x60U, 0xCDU, 0xB0U, 0x54U,
/* STEP 30: classic TX READ block 18 */
/* classic TX READ block 18 */
0xE0U, 0x83U, 0x87U, 0x87U,
/* STEP 31: classic TX READ block 19 */
/* classic TX READ block 19 */
0x38U, 0x54U, 0x34U, 0xC3U,
/* STEP 32: classic TX AUTH NT sector 5 */
/* classic TX AUTH NT sector 5 */
0x60U, 0x14U,
/* STEP 33: classic TX AUTH NR|AR sector 5 */
/* classic TX AUTH NR|AR sector 5 */
0x31U, 0xC3U, 0xFFU, 0x30U, 0x2CU, 0xEEU, 0x25U, 0x9FU,
/* STEP 34: classic TX READ block 20 */
/* classic TX READ block 20 */
0x67U, 0x75U, 0x77U, 0x25U,
/* STEP 35: classic TX READ block 21 */
/* classic TX READ block 21 */
0x6FU, 0xC6U, 0xDBU, 0x23U,
/* STEP 36: classic TX READ block 22 */
/* classic TX READ block 22 */
0xA9U, 0xA6U, 0xBDU, 0xE4U,
/* STEP 37: classic TX READ block 23 */
/* classic TX READ block 23 */
0x48U, 0x3BU, 0x83U, 0x02U,
/* STEP 38: classic TX AUTH NT sector 6 */
/* classic TX AUTH NT sector 6 */
0x60U, 0x18U,
/* STEP 39: classic TX AUTH NR|AR sector 6 */
/* classic TX AUTH NR|AR sector 6 */
0x33U, 0x93U, 0x39U, 0x3DU, 0x79U, 0x5BU, 0x67U, 0x78U,
/* STEP 40: classic TX READ block 24 */
/* classic TX READ block 24 */
0x75U, 0xB2U, 0x15U, 0xC1U,
/* STEP 41: classic TX READ block 25 */
/* classic TX READ block 25 */
0xDEU, 0x8BU, 0x95U, 0x33U,
/* STEP 42: classic TX READ block 26 */
/* classic TX READ block 26 */
0x85U, 0x8EU, 0xE4U, 0x9AU,
/* STEP 43: classic TX READ block 27 */
/* classic TX READ block 27 */
0xECU, 0x72U, 0x9DU, 0x93U,
/* STEP 44: classic TX AUTH NT sector 7 */
/* classic TX AUTH NT sector 7 */
0x60U, 0x1CU,
/* STEP 45: classic TX AUTH NR|AR sector 7 */
/* classic TX AUTH NR|AR sector 7 */
0xB3U, 0xBDU, 0xCDU, 0xA2U, 0xB2U, 0x37U, 0x94U, 0xDCU,
/* STEP 46: classic TX READ block 28 */
/* classic TX READ block 28 */
0x10U, 0xC8U, 0x1BU, 0x81U,
/* STEP 47: classic TX READ block 29 */
/* classic TX READ block 29 */
0xE1U, 0xA3U, 0xE7U, 0xD7U,
/* STEP 48: classic TX READ block 30 */
/* classic TX READ block 30 */
0x12U, 0x4CU, 0x84U, 0x63U,
/* STEP 49: classic TX READ block 31 */
/* classic TX READ block 31 */
0x36U, 0x03U, 0xFAU, 0x70U,
/* STEP 50: classic TX AUTH NT sector 8 */
/* classic TX AUTH NT sector 8 */
0x60U, 0x20U,
/* STEP 51: classic TX AUTH NR|AR sector 8 */
/* classic TX AUTH NR|AR sector 8 */
0x81U, 0xFAU, 0xBEU, 0xB0U, 0xABU, 0xE6U, 0x42U, 0xF1U,
/* STEP 52: classic TX READ block 32 */
/* classic TX READ block 32 */
0xCDU, 0xE0U, 0x32U, 0xD4U,
/* STEP 53: classic TX READ block 33 */
/* classic TX READ block 33 */
0x65U, 0x15U, 0xC7U, 0x84U,
/* STEP 54: classic TX READ block 34 */
/* classic TX READ block 34 */
0x16U, 0x5FU, 0x99U, 0xC6U,
/* STEP 55: classic TX READ block 35 */
/* classic TX READ block 35 */
0xB3U, 0xF0U, 0x7AU, 0x46U,
/* STEP 56: classic TX AUTH NT sector 9 */
/* classic TX AUTH NT sector 9 */
0x60U, 0x24U,
/* STEP 57: classic TX AUTH NR|AR sector 9 */
/* classic TX AUTH NR|AR sector 9 */
0x81U, 0x8CU, 0x48U, 0xB2U, 0x7CU, 0xD8U, 0x54U, 0x49U,
/* STEP 58: classic TX READ block 36 */
/* classic TX READ block 36 */
0x29U, 0x44U, 0xA7U, 0x55U,
/* STEP 59: classic TX READ block 37 */
/* classic TX READ block 37 */
0x28U, 0x86U, 0x15U, 0x4FU,
/* STEP 60: classic TX READ block 38 */
/* classic TX READ block 38 */
0xFBU, 0x84U, 0xA1U, 0x46U,
/* STEP 61: classic TX READ block 39 */
/* classic TX READ block 39 */
0xBAU, 0x22U, 0x64U, 0x2CU,
/* STEP 62: classic TX AUTH NT sector 10 */
/* classic TX AUTH NT sector 10 */
0x60U, 0x28U,
/* STEP 63: classic TX AUTH NR|AR sector 10 */
/* classic TX AUTH NR|AR sector 10 */
0x91U, 0x8EU, 0xEFU, 0xD2U, 0x00U, 0x89U, 0xB6U, 0xF4U,
/* STEP 64: classic TX READ block 40 */
/* classic TX READ block 40 */
0x2FU, 0xA6U, 0x06U, 0xE4U,
/* STEP 65: classic TX READ block 41 */
/* classic TX READ block 41 */
0x04U, 0x8AU, 0x5EU, 0x83U,
/* STEP 66: classic TX READ block 42 */
/* classic TX READ block 42 */
0x9FU, 0x23U, 0xE3U, 0x6DU,
/* STEP 67: classic TX READ block 43 */
/* classic TX READ block 43 */
0x35U, 0x72U, 0x4AU, 0xDCU,
/* STEP 68: classic TX AUTH NT sector 11 */
/* classic TX AUTH NT sector 11 */
0x60U, 0x2CU,
/* STEP 69: classic TX AUTH NR|AR sector 11 */
/* classic TX AUTH NR|AR sector 11 */
0x19U, 0x56U, 0x12U, 0x29U, 0x3DU, 0x21U, 0xDDU, 0xD8U,
/* STEP 70: classic TX READ block 44 */
/* classic TX READ block 44 */
0xE5U, 0xD5U, 0x2FU, 0xDBU,
/* STEP 71: classic TX READ block 45 */
/* classic TX READ block 45 */
0xDCU, 0x3EU, 0x29U, 0x3CU,
/* STEP 72: classic TX READ block 46 */
/* classic TX READ block 46 */
0xE7U, 0x8FU, 0x3EU, 0x4DU,
/* STEP 73: classic TX READ block 47 */
/* classic TX READ block 47 */
0x8DU, 0x1BU, 0x11U, 0xA8U,
/* STEP 74: classic TX AUTH NT sector 12 */
/* classic TX AUTH NT sector 12 */
0x60U, 0x30U,
/* STEP 75: classic TX AUTH NR|AR sector 12 */
/* classic TX AUTH NR|AR sector 12 */
0xA3U, 0x19U, 0xA5U, 0x05U, 0xCFU, 0xE6U, 0x4BU, 0x58U,
/* STEP 76: classic TX READ block 48 */
/* classic TX READ block 48 */
0xBCU, 0x77U, 0x32U, 0x05U,
/* STEP 77: classic TX READ block 49 */
/* classic TX READ block 49 */
0x88U, 0x98U, 0xE0U, 0x37U,
/* STEP 78: classic TX READ block 50 */
/* classic TX READ block 50 */
0x4CU, 0x54U, 0xEAU, 0xC6U,
/* STEP 79: classic TX READ block 51 */
/* classic TX READ block 51 */
0x64U, 0x2CU, 0x83U, 0x3BU,
/* STEP 80: classic TX AUTH NT sector 13 */
/* classic TX AUTH NT sector 13 */
0x60U, 0x34U,
/* STEP 81: classic TX AUTH NR|AR sector 13 */
/* classic TX AUTH NR|AR sector 13 */
0x23U, 0x97U, 0xBFU, 0x1CU, 0x30U, 0xCCU, 0x68U, 0x29U,
/* STEP 82: classic TX READ block 52 */
/* classic TX READ block 52 */
0xCEU, 0xC3U, 0x2DU, 0x53U,
/* STEP 83: classic TX READ block 53 */
/* classic TX READ block 53 */
0xACU, 0xDEU, 0xE2U, 0x90U,
/* STEP 84: classic TX READ block 54 */
/* classic TX READ block 54 */
0x26U, 0xA1U, 0xE2U, 0x69U,
/* STEP 85: classic TX READ block 55 */
/* classic TX READ block 55 */
0xCEU, 0xB8U, 0xBBU, 0xFBU,
/* STEP 86: classic TX AUTH NT sector 14 */
/* classic TX AUTH NT sector 14 */
0x60U, 0x38U,
/* STEP 87: classic TX AUTH NR|AR sector 14 */
/* classic TX AUTH NR|AR sector 14 */
0x31U, 0x46U, 0xE3U, 0x15U, 0x4EU, 0x59U, 0x1FU, 0x48U,
/* STEP 88: classic TX READ block 56 */
/* classic TX READ block 56 */
0x51U, 0x1EU, 0x51U, 0xB4U,
/* STEP 89: classic TX READ block 57 */
/* classic TX READ block 57 */
0x92U, 0x78U, 0x62U, 0x2BU,
/* STEP 90: classic TX READ block 58 */
/* classic TX READ block 58 */
0x2AU, 0xFFU, 0x9CU, 0x25U,
/* STEP 91: classic TX READ block 59 */
/* classic TX READ block 59 */
0xD5U, 0x33U, 0x08U, 0x6BU,
/* STEP 92: classic TX AUTH NT sector 15 */
/* classic TX AUTH NT sector 15 */
0x60U, 0x3CU,
/* STEP 93: classic TX AUTH NR|AR sector 15 */
/* classic TX AUTH NR|AR sector 15 */
0xB3U, 0xC9U, 0x76U, 0xA9U, 0x97U, 0x13U, 0x3AU, 0x12U,
/* STEP 94: classic TX READ block 60 */
/* classic TX READ block 60 */
0x7AU, 0x05U, 0x41U, 0xF3U,
/* STEP 95: classic TX READ block 61 */
/* classic TX READ block 61 */
0x30U, 0xA2U, 0xC4U, 0xD4U,
/* STEP 96: classic TX READ block 62 */
/* classic TX READ block 62 */
0x7BU, 0xAFU, 0x48U, 0xB9U,
/* STEP 97: classic TX READ block 63 */
/* classic TX READ block 63 */
0x8FU, 0xF0U, 0xC2U, 0xA6U,
