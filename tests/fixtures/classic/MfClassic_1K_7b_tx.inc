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
0x70U, 0x40U, 0xE5U, 0x47U, 0x62U, 0x6CU, 0x35U, 0x2CU,
/* STEP 4: classic READ block 0 */
/* classic READ block 0 */
0x09U, 0xF7U, 0x14U, 0x52U,
/* STEP 5: classic READ block 1 */
/* classic READ block 1 */
0xB6U, 0x19U, 0x08U, 0xB2U,
/* STEP 6: classic READ block 2 */
/* classic READ block 2 */
0x96U, 0xDBU, 0xCBU, 0x6EU,
/* STEP 7: classic READ block 3 */
/* classic READ block 3 */
0x26U, 0x62U, 0x3EU, 0xD4U,
/* STEP 8: classic AUTH NT sector 1 */
/* classic AUTH NT sector 1 */
0x60U, 0x04U,
/* STEP 9: classic AUTH NR|AR sector 1 */
/* classic AUTH NR|AR sector 1 */
0x72U, 0x32U, 0x87U, 0xE0U, 0x6FU, 0x5DU, 0xFBU, 0xDDU,
/* STEP 10: classic READ block 4 */
/* classic READ block 4 */
0x85U, 0x6EU, 0x2BU, 0x4AU,
/* STEP 11: classic READ block 5 */
/* classic READ block 5 */
0xA9U, 0xE6U, 0xE6U, 0xF2U,
/* STEP 12: classic READ block 6 */
/* classic READ block 6 */
0x34U, 0xBBU, 0xC0U, 0x9DU,
/* STEP 13: classic READ block 7 */
/* classic READ block 7 */
0x96U, 0x78U, 0xCCU, 0xB8U,
/* STEP 14: classic AUTH NT sector 2 */
/* classic AUTH NT sector 2 */
0x60U, 0x08U,
/* STEP 15: classic AUTH NR|AR sector 2 */
/* classic AUTH NR|AR sector 2 */
0xE4U, 0x43U, 0xE4U, 0x26U, 0x8BU, 0xBBU, 0x8BU, 0xCFU,
/* STEP 16: classic READ block 8 */
/* classic READ block 8 */
0x85U, 0x81U, 0x85U, 0x33U,
/* STEP 17: classic READ block 9 */
/* classic READ block 9 */
0x97U, 0x78U, 0x94U, 0x74U,
/* STEP 18: classic READ block 10 */
/* classic READ block 10 */
0xA2U, 0x30U, 0xF1U, 0xA8U,
/* STEP 19: classic READ block 11 */
/* classic READ block 11 */
0x30U, 0xABU, 0x55U, 0x1CU,
/* STEP 20: classic AUTH NT sector 3 */
/* classic AUTH NT sector 3 */
0x60U, 0x0CU,
/* STEP 21: classic AUTH NR|AR sector 3 */
/* classic AUTH NR|AR sector 3 */
0x67U, 0x87U, 0x4CU, 0xA0U, 0x01U, 0x68U, 0x54U, 0xC8U,
/* STEP 22: classic READ block 12 */
/* classic READ block 12 */
0xB9U, 0x92U, 0xDAU, 0x66U,
/* STEP 23: classic READ block 13 */
/* classic READ block 13 */
0x5CU, 0x34U, 0xA7U, 0xC6U,
/* STEP 24: classic READ block 14 */
/* classic READ block 14 */
0x14U, 0x99U, 0xB2U, 0x42U,
/* STEP 25: classic READ block 15 */
/* classic READ block 15 */
0xC8U, 0x6DU, 0x49U, 0x19U,
/* STEP 26: classic AUTH NT sector 4 */
/* classic AUTH NT sector 4 */
0x60U, 0x10U,
/* STEP 27: classic AUTH NR|AR sector 4 */
/* classic AUTH NR|AR sector 4 */
0x71U, 0x27U, 0x61U, 0x23U, 0xC0U, 0x0CU, 0xEDU, 0xCFU,
/* STEP 28: classic READ block 16 */
/* classic READ block 16 */
0xDCU, 0xB6U, 0xDFU, 0x77U,
/* STEP 29: classic READ block 17 */
/* classic READ block 17 */
0x56U, 0xC7U, 0x2FU, 0x3DU,
/* STEP 30: classic READ block 18 */
/* classic READ block 18 */
0x1FU, 0xB1U, 0x31U, 0xD4U,
/* STEP 31: classic READ block 19 */
/* classic READ block 19 */
0xF1U, 0xF1U, 0x58U, 0x89U,
/* STEP 32: classic AUTH NT sector 5 */
/* classic AUTH NT sector 5 */
0x60U, 0x14U,
/* STEP 33: classic AUTH NR|AR sector 5 */
/* classic AUTH NR|AR sector 5 */
0xF2U, 0x65U, 0x25U, 0xCFU, 0xF4U, 0xDDU, 0x88U, 0x57U,
/* STEP 34: classic READ block 20 */
/* classic READ block 20 */
0x1CU, 0x1DU, 0xF3U, 0x79U,
/* STEP 35: classic READ block 21 */
/* classic READ block 21 */
0xB7U, 0x24U, 0xDDU, 0xC9U,
/* STEP 36: classic READ block 22 */
/* classic READ block 22 */
0x93U, 0xA6U, 0x95U, 0x1DU,
/* STEP 37: classic READ block 23 */
/* classic READ block 23 */
0x8AU, 0x02U, 0x1AU, 0x7DU,
/* STEP 38: classic AUTH NT sector 6 */
/* classic AUTH NT sector 6 */
0x60U, 0x18U,
/* STEP 39: classic AUTH NR|AR sector 6 */
/* classic AUTH NR|AR sector 6 */
0xECU, 0xA3U, 0xA4U, 0x94U, 0xDDU, 0xE8U, 0x16U, 0xF3U,
/* STEP 40: classic READ block 24 */
/* classic READ block 24 */
0x38U, 0xDCU, 0x6AU, 0xA6U,
/* STEP 41: classic READ block 25 */
/* classic READ block 25 */
0x1FU, 0x75U, 0x9FU, 0xB4U,
/* STEP 42: classic READ block 26 */
/* classic READ block 26 */
0x32U, 0xB9U, 0xDEU, 0x7CU,
/* STEP 43: classic READ block 27 */
/* classic READ block 27 */
0x04U, 0x00U, 0x92U, 0x29U,
/* STEP 44: classic AUTH NT sector 7 */
/* classic AUTH NT sector 7 */
0x60U, 0x1CU,
/* STEP 45: classic AUTH NR|AR sector 7 */
/* classic AUTH NR|AR sector 7 */
0xA7U, 0xE0U, 0x00U, 0xECU, 0xF4U, 0xFDU, 0x0CU, 0x16U,
/* STEP 46: classic READ block 28 */
/* classic READ block 28 */
0x72U, 0xD1U, 0x9EU, 0x3BU,
/* STEP 47: classic READ block 29 */
/* classic READ block 29 */
0x17U, 0x56U, 0xEDU, 0x45U,
/* STEP 48: classic READ block 30 */
/* classic READ block 30 */
0xE6U, 0xA3U, 0x8CU, 0xA7U,
/* STEP 49: classic READ block 31 */
/* classic READ block 31 */
0xC2U, 0xAAU, 0x44U, 0x70U,
/* STEP 50: classic AUTH NT sector 8 */
/* classic AUTH NT sector 8 */
0x60U, 0x20U,
/* STEP 51: classic AUTH NR|AR sector 8 */
/* classic AUTH NR|AR sector 8 */
0x79U, 0xE2U, 0x76U, 0x73U, 0x38U, 0x10U, 0xC7U, 0x2DU,
/* STEP 52: classic READ block 32 */
/* classic READ block 32 */
0x00U, 0xD5U, 0x1AU, 0x9BU,
/* STEP 53: classic READ block 33 */
/* classic READ block 33 */
0x97U, 0x22U, 0x59U, 0x12U,
/* STEP 54: classic READ block 34 */
/* classic READ block 34 */
0x7DU, 0x4DU, 0x54U, 0x5FU,
/* STEP 55: classic READ block 35 */
/* classic READ block 35 */
0x35U, 0x6DU, 0xF7U, 0xB2U,
/* STEP 56: classic AUTH NT sector 9 */
/* classic AUTH NT sector 9 */
0x60U, 0x24U,
/* STEP 57: classic AUTH NR|AR sector 9 */
/* classic AUTH NR|AR sector 9 */
0x73U, 0x86U, 0x24U, 0x13U, 0xCDU, 0x66U, 0x2BU, 0xF7U,
/* STEP 58: classic READ block 36 */
/* classic READ block 36 */
0x5FU, 0x8FU, 0x82U, 0x0AU,
/* STEP 59: classic READ block 37 */
/* classic READ block 37 */
0x08U, 0x8DU, 0x16U, 0xC8U,
/* STEP 60: classic READ block 38 */
/* classic READ block 38 */
0x1DU, 0x3FU, 0xBDU, 0xE7U,
/* STEP 61: classic READ block 39 */
/* classic READ block 39 */
0xA9U, 0xD7U, 0x18U, 0x96U,
/* STEP 62: classic AUTH NT sector 10 */
/* classic AUTH NT sector 10 */
0x60U, 0x28U,
/* STEP 63: classic AUTH NR|AR sector 10 */
/* classic AUTH NR|AR sector 10 */
0xADU, 0x86U, 0x55U, 0x47U, 0x76U, 0xF8U, 0x03U, 0xDCU,
/* STEP 64: classic READ block 40 */
/* classic READ block 40 */
0xEDU, 0x75U, 0x1FU, 0x9DU,
/* STEP 65: classic READ block 41 */
/* classic READ block 41 */
0x14U, 0x41U, 0x77U, 0xDAU,
/* STEP 66: classic READ block 42 */
/* classic READ block 42 */
0x2CU, 0xE4U, 0x73U, 0xA5U,
/* STEP 67: classic READ block 43 */
/* classic READ block 43 */
0x8DU, 0x5DU, 0x54U, 0x1CU,
/* STEP 68: classic AUTH NT sector 11 */
/* classic AUTH NT sector 11 */
0x60U, 0x2CU,
/* STEP 69: classic AUTH NR|AR sector 11 */
/* classic AUTH NR|AR sector 11 */
0x66U, 0xB1U, 0xF1U, 0xEEU, 0x3CU, 0xD9U, 0xC5U, 0x03U,
/* STEP 70: classic READ block 44 */
/* classic READ block 44 */
0x25U, 0x1CU, 0xC1U, 0xFEU,
/* STEP 71: classic READ block 45 */
/* classic READ block 45 */
0x4AU, 0xC4U, 0x9FU, 0xEDU,
/* STEP 72: classic READ block 46 */
/* classic READ block 46 */
0x03U, 0xAAU, 0x56U, 0x9CU,
/* STEP 73: classic READ block 47 */
/* classic READ block 47 */
0xEAU, 0x1BU, 0xE8U, 0x46U,
/* STEP 74: classic AUTH NT sector 12 */
/* classic AUTH NT sector 12 */
0x60U, 0x30U,
/* STEP 75: classic AUTH NR|AR sector 12 */
/* classic AUTH NR|AR sector 12 */
0x71U, 0x21U, 0x27U, 0xA6U, 0x40U, 0x2CU, 0xBEU, 0xBDU,
/* STEP 76: classic READ block 48 */
/* classic READ block 48 */
0x8DU, 0x9CU, 0x91U, 0x2CU,
/* STEP 77: classic READ block 49 */
/* classic READ block 49 */
0xE7U, 0x09U, 0x11U, 0x93U,
/* STEP 78: classic READ block 50 */
/* classic READ block 50 */
0xACU, 0xF8U, 0x70U, 0xAAU,
/* STEP 79: classic READ block 51 */
/* classic READ block 51 */
0xEAU, 0x86U, 0xAEU, 0x7DU,
/* STEP 80: classic AUTH NT sector 13 */
/* classic AUTH NT sector 13 */
0x60U, 0x34U,
/* STEP 81: classic AUTH NR|AR sector 13 */
/* classic AUTH NR|AR sector 13 */
0xF3U, 0xB6U, 0x4DU, 0x3FU, 0x4AU, 0x2BU, 0x0FU, 0xBAU,
/* STEP 82: classic READ block 52 */
/* classic READ block 52 */
0x6AU, 0x75U, 0xE4U, 0x87U,
/* STEP 83: classic READ block 53 */
/* classic READ block 53 */
0x80U, 0x37U, 0x83U, 0xD1U,
/* STEP 84: classic READ block 54 */
/* classic READ block 54 */
0x63U, 0x6DU, 0x07U, 0x0BU,
/* STEP 85: classic READ block 55 */
/* classic READ block 55 */
0xC4U, 0x93U, 0x0BU, 0x56U,
/* STEP 86: classic AUTH NT sector 14 */
/* classic AUTH NT sector 14 */
0x60U, 0x38U,
/* STEP 87: classic AUTH NR|AR sector 14 */
/* classic AUTH NR|AR sector 14 */
0xE4U, 0x84U, 0x98U, 0x22U, 0x99U, 0xAAU, 0xD6U, 0xB3U,
/* STEP 88: classic READ block 56 */
/* classic READ block 56 */
0xEDU, 0x63U, 0x6EU, 0x9CU,
/* STEP 89: classic READ block 57 */
/* classic READ block 57 */
0x67U, 0x88U, 0x83U, 0xD2U,
/* STEP 90: classic READ block 58 */
/* classic READ block 58 */
0xA6U, 0xB8U, 0xA8U, 0x19U,
/* STEP 91: classic READ block 59 */
/* classic READ block 59 */
0xC4U, 0x9AU, 0xFFU, 0xD3U,
/* STEP 92: classic AUTH NT sector 15 */
/* classic AUTH NT sector 15 */
0x60U, 0x3CU,
/* STEP 93: classic AUTH NR|AR sector 15 */
/* classic AUTH NR|AR sector 15 */
0xE6U, 0x07U, 0xF8U, 0xECU, 0x13U, 0xEDU, 0xB9U, 0x94U,
/* STEP 94: classic READ block 60 */
/* classic READ block 60 */
0x0FU, 0x60U, 0xE7U, 0xE6U,
/* STEP 95: classic READ block 61 */
/* classic READ block 61 */
0x4EU, 0x2EU, 0x8BU, 0xD7U,
/* STEP 96: classic READ block 62 */
/* classic READ block 62 */
0x8CU, 0x10U, 0x43U, 0xE5U,
/* STEP 97: classic READ block 63 */
/* classic READ block 63 */
0x25U, 0xCFU, 0xC3U, 0xA4U,
