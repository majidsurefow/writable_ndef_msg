/* Tier B poller TX golden — one STEP per transceive */
/* STEP 0: classic detect 4k probe fail */
/* classic detect 4k probe fail */
0x60U, 0xFEU,
/* STEP 1: classic detect 1k probe nt */
/* classic detect 1k probe nt */
0x60U, 0x3EU,
/* STEP 2: classic AUTH NT sector 0 */
/* classic AUTH NT sector 0 */
0x60U, 0x00U,
/* STEP 3: classic AUTH NR|AR sector 0 */
/* classic AUTH NR|AR sector 0 */
0x91U, 0x07U, 0xBDU, 0x3AU, 0xFFU, 0xF9U, 0xF4U, 0x82U,
/* STEP 4: classic READ block 0 */
/* classic READ block 0 */
0x2CU, 0x7AU, 0xFBU, 0xB1U,
/* STEP 5: classic READ block 1 */
/* classic READ block 1 */
0x7BU, 0xECU, 0x75U, 0x17U,
/* STEP 6: classic READ block 2 */
/* classic READ block 2 */
0x60U, 0x62U, 0x69U, 0x45U,
/* STEP 7: classic READ block 3 */
/* classic READ block 3 */
0xEFU, 0x3FU, 0xCBU, 0x41U,
/* STEP 8: classic AUTH NT sector 1 */
/* classic AUTH NT sector 1 */
0x60U, 0x04U,
/* STEP 9: classic AUTH NR|AR sector 1 */
/* classic AUTH NR|AR sector 1 */
0x11U, 0x3FU, 0xD3U, 0x33U, 0xD3U, 0x66U, 0x90U, 0xCBU,
/* STEP 10: classic READ block 4 */
/* classic READ block 4 */
0x9BU, 0xA7U, 0xC8U, 0x14U,
/* STEP 11: classic READ block 5 */
/* classic READ block 5 */
0x4BU, 0x3FU, 0x06U, 0xBFU,
/* STEP 12: classic READ block 6 */
/* classic READ block 6 */
0x21U, 0x6DU, 0xC6U, 0x3AU,
/* STEP 13: classic READ block 7 */
/* classic READ block 7 */
0xF8U, 0x2BU, 0xBAU, 0x48U,
/* STEP 14: classic AUTH NT sector 2 */
/* classic AUTH NT sector 2 */
0x60U, 0x08U,
/* STEP 15: classic AUTH NR|AR sector 2 */
/* classic AUTH NR|AR sector 2 */
0x91U, 0xC8U, 0x59U, 0x9EU, 0x0EU, 0x56U, 0xFDU, 0xDFU,
/* STEP 16: classic READ block 8 */
/* classic READ block 8 */
0xDFU, 0x7DU, 0x4BU, 0xE2U,
/* STEP 17: classic READ block 9 */
/* classic READ block 9 */
0xD7U, 0x3CU, 0xC7U, 0xFEU,
/* STEP 18: classic READ block 10 */
/* classic READ block 10 */
0xF2U, 0x16U, 0x75U, 0x57U,
/* STEP 19: classic READ block 11 */
/* classic READ block 11 */
0xA6U, 0x7AU, 0x8DU, 0xBCU,
/* STEP 20: classic AUTH NT sector 3 */
/* classic AUTH NT sector 3 */
0x60U, 0x0CU,
/* STEP 21: classic AUTH NR|AR sector 3 */
/* classic AUTH NR|AR sector 3 */
0x59U, 0xDAU, 0xDFU, 0xFEU, 0x61U, 0x21U, 0xE7U, 0x37U,
/* STEP 22: classic READ block 12 */
/* classic READ block 12 */
0x83U, 0x14U, 0x57U, 0x88U,
/* STEP 23: classic READ block 13 */
/* classic READ block 13 */
0x80U, 0xB7U, 0x94U, 0x8FU,
/* STEP 24: classic READ block 14 */
/* classic READ block 14 */
0xAEU, 0xF5U, 0xE4U, 0x48U,
/* STEP 25: classic READ block 15 */
/* classic READ block 15 */
0x2EU, 0x21U, 0x98U, 0xDEU,
/* STEP 26: classic AUTH NT sector 4 */
/* classic AUTH NT sector 4 */
0x60U, 0x10U,
/* STEP 27: classic AUTH NR|AR sector 4 */
/* classic AUTH NR|AR sector 4 */
0xB3U, 0x88U, 0xF7U, 0xE4U, 0x11U, 0x64U, 0xB4U, 0x7AU,
/* STEP 28: classic READ block 16 */
/* classic READ block 16 */
0x57U, 0xA2U, 0x8EU, 0x48U,
/* STEP 29: classic READ block 17 */
/* classic READ block 17 */
0x1BU, 0x53U, 0x8AU, 0xABU,
/* STEP 30: classic READ block 18 */
/* classic READ block 18 */
0x1FU, 0x93U, 0xD5U, 0x9EU,
/* STEP 31: classic READ block 19 */
/* classic READ block 19 */
0x42U, 0x70U, 0xF1U, 0x51U,
/* STEP 32: classic AUTH NT sector 5 */
/* classic AUTH NT sector 5 */
0x60U, 0x14U,
/* STEP 33: classic AUTH NR|AR sector 5 */
/* classic AUTH NR|AR sector 5 */
0x31U, 0xC3U, 0xFFU, 0x30U, 0x2CU, 0xEEU, 0x25U, 0x9FU,
/* STEP 34: classic READ block 20 */
/* classic READ block 20 */
0x67U, 0x75U, 0x77U, 0x25U,
/* STEP 35: classic READ block 21 */
/* classic READ block 21 */
0x0EU, 0xCDU, 0x38U, 0x41U,
/* STEP 36: classic READ block 22 */
/* classic READ block 22 */
0xCEU, 0xBCU, 0x69U, 0x1BU,
/* STEP 37: classic READ block 23 */
/* classic READ block 23 */
0x2EU, 0x9FU, 0xB7U, 0x0AU,
/* STEP 38: classic AUTH NT sector 6 */
/* classic AUTH NT sector 6 */
0x60U, 0x18U,
/* STEP 39: classic AUTH NR|AR sector 6 */
/* classic AUTH NR|AR sector 6 */
0x33U, 0x93U, 0x39U, 0x3DU, 0x79U, 0x5BU, 0x67U, 0x78U,
/* STEP 40: classic READ block 24 */
/* classic READ block 24 */
0x75U, 0xB2U, 0x15U, 0xC1U,
/* STEP 41: classic READ block 25 */
/* classic READ block 25 */
0x57U, 0xDDU, 0xC9U, 0x40U,
/* STEP 42: classic READ block 26 */
/* classic READ block 26 */
0x81U, 0x9CU, 0x97U, 0xB7U,
/* STEP 43: classic READ block 27 */
/* classic READ block 27 */
0x91U, 0x98U, 0x64U, 0x93U,
/* STEP 44: classic AUTH NT sector 7 */
/* classic AUTH NT sector 7 */
0x60U, 0x1CU,
/* STEP 45: classic AUTH NR|AR sector 7 */
/* classic AUTH NR|AR sector 7 */
0xB3U, 0xBDU, 0xCDU, 0xA2U, 0xB2U, 0x37U, 0x94U, 0xDCU,
/* STEP 46: classic READ block 28 */
/* classic READ block 28 */
0x10U, 0xC8U, 0x1BU, 0x81U,
/* STEP 47: classic READ block 29 */
/* classic READ block 29 */
0x4EU, 0x7FU, 0xAAU, 0x84U,
/* STEP 48: classic READ block 30 */
/* classic READ block 30 */
0xF5U, 0xDBU, 0xC0U, 0x86U,
/* STEP 49: classic READ block 31 */
/* classic READ block 31 */
0x3FU, 0xC7U, 0x8CU, 0xD7U,
/* STEP 50: classic AUTH NT sector 8 */
/* classic AUTH NT sector 8 */
0x60U, 0x20U,
/* STEP 51: classic AUTH NR|AR sector 8 */
/* classic AUTH NR|AR sector 8 */
0x81U, 0xFAU, 0xBEU, 0xB0U, 0xABU, 0xE6U, 0x42U, 0xF1U,
/* STEP 52: classic READ block 32 */
/* classic READ block 32 */
0xCDU, 0xE0U, 0x32U, 0xD4U,
/* STEP 53: classic READ block 33 */
/* classic READ block 33 */
0xD4U, 0x8CU, 0x2BU, 0x88U,
/* STEP 54: classic READ block 34 */
/* classic READ block 34 */
0x84U, 0x10U, 0x92U, 0xD5U,
/* STEP 55: classic READ block 35 */
/* classic READ block 35 */
0xECU, 0xF2U, 0x58U, 0x4FU,
/* STEP 56: classic AUTH NT sector 9 */
/* classic AUTH NT sector 9 */
0x60U, 0x24U,
/* STEP 57: classic AUTH NR|AR sector 9 */
/* classic AUTH NR|AR sector 9 */
0x81U, 0x8CU, 0x48U, 0xB2U, 0x7CU, 0xD8U, 0x54U, 0x49U,
/* STEP 58: classic READ block 36 */
/* classic READ block 36 */
0x29U, 0x44U, 0xA7U, 0x55U,
/* STEP 59: classic READ block 37 */
/* classic READ block 37 */
0x1BU, 0xAAU, 0x3FU, 0xA4U,
/* STEP 60: classic READ block 38 */
/* classic READ block 38 */
0x70U, 0x37U, 0xC4U, 0x81U,
/* STEP 61: classic READ block 39 */
/* classic READ block 39 */
0x34U, 0x39U, 0xABU, 0x16U,
/* STEP 62: classic AUTH NT sector 10 */
/* classic AUTH NT sector 10 */
0x60U, 0x28U,
/* STEP 63: classic AUTH NR|AR sector 10 */
/* classic AUTH NR|AR sector 10 */
0x91U, 0x8EU, 0xEFU, 0xD2U, 0x00U, 0x89U, 0xB6U, 0xF4U,
/* STEP 64: classic READ block 40 */
/* classic READ block 40 */
0x2FU, 0xA6U, 0x06U, 0xE4U,
/* STEP 65: classic READ block 41 */
/* classic READ block 41 */
0x0CU, 0x41U, 0xF6U, 0x80U,
/* STEP 66: classic READ block 42 */
/* classic READ block 42 */
0xE1U, 0x9EU, 0x30U, 0xBFU,
/* STEP 67: classic READ block 43 */
/* classic READ block 43 */
0x11U, 0xD5U, 0x98U, 0xFDU,
/* STEP 68: classic AUTH NT sector 11 */
/* classic AUTH NT sector 11 */
0x60U, 0x2CU,
/* STEP 69: classic AUTH NR|AR sector 11 */
/* classic AUTH NR|AR sector 11 */
0x19U, 0x56U, 0x12U, 0x29U, 0x3DU, 0x21U, 0xDDU, 0xD8U,
/* STEP 70: classic READ block 44 */
/* classic READ block 44 */
0xE5U, 0xD5U, 0x2FU, 0xDBU,
/* STEP 71: classic READ block 45 */
/* classic READ block 45 */
0x65U, 0x63U, 0x76U, 0xC4U,
/* STEP 72: classic READ block 46 */
/* classic READ block 46 */
0x60U, 0x95U, 0x81U, 0x4EU,
/* STEP 73: classic READ block 47 */
/* classic READ block 47 */
0x11U, 0x76U, 0xB5U, 0xB4U,
/* STEP 74: classic AUTH NT sector 12 */
/* classic AUTH NT sector 12 */
0x60U, 0x30U,
/* STEP 75: classic AUTH NR|AR sector 12 */
/* classic AUTH NR|AR sector 12 */
0xA3U, 0x19U, 0xA5U, 0x05U, 0xCFU, 0xE6U, 0x4BU, 0x58U,
/* STEP 76: classic READ block 48 */
/* classic READ block 48 */
0xBCU, 0x77U, 0x32U, 0x05U,
/* STEP 77: classic READ block 49 */
/* classic READ block 49 */
0x3AU, 0x23U, 0x86U, 0x9DU,
/* STEP 78: classic READ block 50 */
/* classic READ block 50 */
0xE7U, 0x87U, 0xD8U, 0xBBU,
/* STEP 79: classic READ block 51 */
/* classic READ block 51 */
0x53U, 0x45U, 0x3BU, 0x9CU,
/* STEP 80: classic AUTH NT sector 13 */
/* classic AUTH NT sector 13 */
0x60U, 0x34U,
/* STEP 81: classic AUTH NR|AR sector 13 */
/* classic AUTH NR|AR sector 13 */
0x23U, 0x97U, 0xBFU, 0x1CU, 0x30U, 0xCCU, 0x68U, 0x29U,
/* STEP 82: classic READ block 52 */
/* classic READ block 52 */
0xCEU, 0xC3U, 0x2DU, 0x53U,
/* STEP 83: classic READ block 53 */
/* classic READ block 53 */
0xCEU, 0x3EU, 0x6DU, 0xBBU,
/* STEP 84: classic READ block 54 */
/* classic READ block 54 */
0xDCU, 0x82U, 0x12U, 0x93U,
/* STEP 85: classic READ block 55 */
/* classic READ block 55 */
0x78U, 0x2DU, 0x37U, 0xFCU,
/* STEP 86: classic AUTH NT sector 14 */
/* classic AUTH NT sector 14 */
0x60U, 0x38U,
/* STEP 87: classic AUTH NR|AR sector 14 */
/* classic AUTH NR|AR sector 14 */
0x31U, 0x46U, 0xE3U, 0x15U, 0x4EU, 0x59U, 0x1FU, 0x48U,
/* STEP 88: classic READ block 56 */
/* classic READ block 56 */
0x51U, 0x1EU, 0x51U, 0xB4U,
/* STEP 89: classic READ block 57 */
/* classic READ block 57 */
0x33U, 0xF6U, 0xABU, 0xEEU,
/* STEP 90: classic READ block 58 */
/* classic READ block 58 */
0x2CU, 0xABU, 0x96U, 0xABU,
/* STEP 91: classic READ block 59 */
/* classic READ block 59 */
0x05U, 0x4DU, 0x01U, 0x9EU,
/* STEP 92: classic AUTH NT sector 15 */
/* classic AUTH NT sector 15 */
0x60U, 0x3CU,
/* STEP 93: classic AUTH NR|AR sector 15 */
/* classic AUTH NR|AR sector 15 */
0xB3U, 0xC9U, 0x76U, 0xA9U, 0x97U, 0x13U, 0x3AU, 0x12U,
/* STEP 94: classic READ block 60 */
/* classic READ block 60 */
0x7AU, 0x05U, 0x41U, 0xF3U,
/* STEP 95: classic READ block 61 */
/* classic READ block 61 */
0xF9U, 0x15U, 0xF9U, 0xABU,
/* STEP 96: classic READ block 62 */
/* classic READ block 62 */
0xD2U, 0x89U, 0xEDU, 0xE1U,
/* STEP 97: classic READ block 63 */
/* classic READ block 63 */
0xF3U, 0x49U, 0x46U, 0xC8U,
