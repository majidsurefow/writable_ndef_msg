/* Tier B poller TX golden — one STEP per transceive */
/* STEP 0: classic detect 4k probe nt */
/* classic detect 4k probe nt */
0x60U, 0xFEU,
/* STEP 1: classic AUTH NT sector 0 */
/* classic AUTH NT sector 0 */
0x60U, 0x00U,
/* STEP 2: classic AUTH NR|AR sector 0 */
/* classic AUTH NR|AR sector 0 */
0x22U, 0xF6U, 0x96U, 0x33U, 0xEAU, 0x28U, 0x80U, 0x04U,
/* STEP 3: classic READ block 0 */
/* classic READ block 0 */
0x24U, 0xC1U, 0x9BU, 0x97U,
/* STEP 4: classic READ block 1 */
/* classic READ block 1 */
0x27U, 0x5FU, 0x5FU, 0xC9U,
/* STEP 5: classic READ block 2 */
/* classic READ block 2 */
0xC2U, 0xDBU, 0x38U, 0x01U,
/* STEP 6: classic READ block 3 */
/* classic READ block 3 */
0x60U, 0x75U, 0x69U, 0x69U,
/* STEP 7: classic AUTH NT sector 1 */
/* classic AUTH NT sector 1 */
0x60U, 0x04U,
/* STEP 8: classic AUTH NR|AR sector 1 */
/* classic AUTH NR|AR sector 1 */
0x22U, 0x48U, 0xF3U, 0x58U, 0x46U, 0xE5U, 0x0BU, 0x35U,
/* STEP 9: classic READ block 4 */
/* classic READ block 4 */
0x09U, 0x56U, 0xBAU, 0xD0U,
/* STEP 10: classic READ block 5 */
/* classic READ block 5 */
0x19U, 0x3DU, 0x18U, 0x04U,
/* STEP 11: classic READ block 6 */
/* classic READ block 6 */
0x50U, 0x8FU, 0x33U, 0x30U,
/* STEP 12: classic READ block 7 */
/* classic READ block 7 */
0xE9U, 0x7AU, 0x2CU, 0xE2U,
/* STEP 13: classic AUTH NT sector 2 */
/* classic AUTH NT sector 2 */
0x60U, 0x08U,
/* STEP 14: classic AUTH NR|AR sector 2 */
/* classic AUTH NR|AR sector 2 */
0xEAU, 0x6EU, 0xF9U, 0xF1U, 0xD0U, 0xDAU, 0xEEU, 0x0AU,
/* STEP 15: classic READ block 8 */
/* classic READ block 8 */
0xF9U, 0x0CU, 0xAAU, 0x7EU,
/* STEP 16: classic READ block 9 */
/* classic READ block 9 */
0x21U, 0x52U, 0xD5U, 0x5EU,
/* STEP 17: classic READ block 10 */
/* classic READ block 10 */
0x8BU, 0x9BU, 0xB8U, 0x2CU,
/* STEP 18: classic READ block 11 */
/* classic READ block 11 */
0x8DU, 0x31U, 0xFEU, 0x4BU,
/* STEP 19: classic AUTH NT sector 3 */
/* classic AUTH NT sector 3 */
0x60U, 0x0CU,
/* STEP 20: classic AUTH NR|AR sector 3 */
/* classic AUTH NR|AR sector 3 */
0xA0U, 0x7AU, 0x15U, 0x9CU, 0xD1U, 0x3DU, 0x12U, 0x4DU,
/* STEP 21: classic READ block 12 */
/* classic READ block 12 */
0xAFU, 0xA4U, 0xCFU, 0x15U,
/* STEP 22: classic READ block 13 */
/* classic READ block 13 */
0xD2U, 0x7FU, 0x0BU, 0x87U,
/* STEP 23: classic READ block 14 */
/* classic READ block 14 */
0xB2U, 0xD0U, 0xD5U, 0xF7U,
/* STEP 24: classic READ block 15 */
/* classic READ block 15 */
0x67U, 0x25U, 0x37U, 0x36U,
/* STEP 25: classic AUTH NT sector 4 */
/* classic AUTH NT sector 4 */
0x60U, 0x10U,
/* STEP 26: classic AUTH NR|AR sector 4 */
/* classic AUTH NR|AR sector 4 */
0x22U, 0xEEU, 0x51U, 0xA3U, 0xCAU, 0xF9U, 0xA5U, 0x42U,
/* STEP 27: classic READ block 16 */
/* classic READ block 16 */
0xD4U, 0xBAU, 0x7BU, 0x5AU,
/* STEP 28: classic READ block 17 */
/* classic READ block 17 */
0xA0U, 0x0EU, 0xF9U, 0xF3U,
/* STEP 29: classic READ block 18 */
/* classic READ block 18 */
0xE2U, 0xC8U, 0x90U, 0x34U,
/* STEP 30: classic READ block 19 */
/* classic READ block 19 */
0x9FU, 0xF1U, 0xC8U, 0xA9U,
/* STEP 31: classic AUTH NT sector 5 */
/* classic AUTH NT sector 5 */
0x60U, 0x14U,
/* STEP 32: classic AUTH NR|AR sector 5 */
/* classic AUTH NR|AR sector 5 */
0x80U, 0xCDU, 0x7AU, 0x12U, 0x41U, 0x6FU, 0x73U, 0xDCU,
/* STEP 33: classic READ block 20 */
/* classic READ block 20 */
0x53U, 0x30U, 0x7BU, 0x93U,
/* STEP 34: classic READ block 21 */
/* classic READ block 21 */
0x5BU, 0x4DU, 0xBBU, 0x28U,
/* STEP 35: classic READ block 22 */
/* classic READ block 22 */
0xF6U, 0xB5U, 0xE4U, 0x59U,
/* STEP 36: classic READ block 23 */
/* classic READ block 23 */
0x2AU, 0x44U, 0xF0U, 0x44U,
/* STEP 37: classic AUTH NT sector 6 */
/* classic AUTH NT sector 6 */
0x60U, 0x18U,
/* STEP 38: classic AUTH NR|AR sector 6 */
/* classic AUTH NR|AR sector 6 */
0xE2U, 0xFAU, 0xF1U, 0xF9U, 0xF0U, 0x42U, 0xC8U, 0x51U,
/* STEP 39: classic READ block 24 */
/* classic READ block 24 */
0x22U, 0x36U, 0x1BU, 0x52U,
/* STEP 40: classic READ block 25 */
/* classic READ block 25 */
0x94U, 0x47U, 0xB7U, 0x7BU,
/* STEP 41: classic READ block 26 */
/* classic READ block 26 */
0x06U, 0x9AU, 0x73U, 0x2AU,
/* STEP 42: classic READ block 27 */
/* classic READ block 27 */
0xA5U, 0x4FU, 0xACU, 0xA0U,
/* STEP 43: classic AUTH NT sector 7 */
/* classic AUTH NT sector 7 */
0x60U, 0x1CU,
/* STEP 44: classic AUTH NR|AR sector 7 */
/* classic AUTH NR|AR sector 7 */
0x82U, 0xBBU, 0x37U, 0xD5U, 0xFDU, 0xE5U, 0x47U, 0xC7U,
/* STEP 45: classic READ block 28 */
/* classic READ block 28 */
0x0BU, 0xDCU, 0x50U, 0xA1U,
/* STEP 46: classic READ block 29 */
/* classic READ block 29 */
0x7AU, 0x27U, 0x31U, 0xE8U,
/* STEP 47: classic READ block 30 */
/* classic READ block 30 */
0x2BU, 0x44U, 0xDDU, 0x02U,
/* STEP 48: classic READ block 31 */
/* classic READ block 31 */
0x5AU, 0x6BU, 0xB6U, 0xB2U,
/* STEP 49: classic AUTH NT sector 8 */
/* classic AUTH NT sector 8 */
0x60U, 0x20U,
/* STEP 50: classic AUTH NR|AR sector 8 */
/* classic AUTH NR|AR sector 8 */
0xA2U, 0xFEU, 0xB3U, 0xADU, 0xFAU, 0xFBU, 0x17U, 0x7CU,
/* STEP 51: classic READ block 32 */
/* classic READ block 32 */
0x41U, 0xD0U, 0xE0U, 0x4AU,
/* STEP 52: classic READ block 33 */
/* classic READ block 33 */
0x85U, 0x07U, 0xA6U, 0x51U,
/* STEP 53: classic READ block 34 */
/* classic READ block 34 */
0x68U, 0x6AU, 0x64U, 0xD9U,
/* STEP 54: classic READ block 35 */
/* classic READ block 35 */
0xCBU, 0x40U, 0x9EU, 0xA9U,
/* STEP 55: classic AUTH NT sector 9 */
/* classic AUTH NT sector 9 */
0x60U, 0x24U,
/* STEP 56: classic AUTH NR|AR sector 9 */
/* classic AUTH NR|AR sector 9 */
0x22U, 0x4EU, 0x45U, 0xC0U, 0xC1U, 0x1AU, 0x0AU, 0x4AU,
/* STEP 57: classic READ block 36 */
/* classic READ block 36 */
0x80U, 0xA5U, 0x59U, 0x4AU,
/* STEP 58: classic READ block 37 */
/* classic READ block 37 */
0x8BU, 0x44U, 0xDBU, 0x0BU,
/* STEP 59: classic READ block 38 */
/* classic READ block 38 */
0xE3U, 0xCBU, 0xC2U, 0x9BU,
/* STEP 60: classic READ block 39 */
/* classic READ block 39 */
0xAEU, 0x50U, 0x82U, 0xF7U,
/* STEP 61: classic AUTH NT sector 10 */
/* classic AUTH NT sector 10 */
0x60U, 0x28U,
/* STEP 62: classic AUTH NR|AR sector 10 */
/* classic AUTH NR|AR sector 10 */
0xB2U, 0x9BU, 0x53U, 0x69U, 0x78U, 0xD6U, 0x96U, 0x7DU,
/* STEP 63: classic READ block 40 */
/* classic READ block 40 */
0xDEU, 0x30U, 0x57U, 0xAFU,
/* STEP 64: classic READ block 41 */
/* classic READ block 41 */
0x37U, 0x17U, 0xADU, 0xFCU,
/* STEP 65: classic READ block 42 */
/* classic READ block 42 */
0xB0U, 0xE3U, 0xB3U, 0x7DU,
/* STEP 66: classic READ block 43 */
/* classic READ block 43 */
0xBBU, 0x8EU, 0x74U, 0x0EU,
/* STEP 67: classic AUTH NT sector 11 */
/* classic AUTH NT sector 11 */
0x60U, 0x2CU,
/* STEP 68: classic AUTH NR|AR sector 11 */
/* classic AUTH NR|AR sector 11 */
0xF0U, 0xE6U, 0x23U, 0x8EU, 0x6BU, 0xDAU, 0x9BU, 0xE5U,
/* STEP 69: classic READ block 44 */
/* classic READ block 44 */
0x32U, 0x57U, 0x64U, 0xA2U,
/* STEP 70: classic READ block 45 */
/* classic READ block 45 */
0x27U, 0x73U, 0xDFU, 0xE1U,
/* STEP 71: classic READ block 46 */
/* classic READ block 46 */
0x66U, 0x41U, 0x46U, 0xABU,
/* STEP 72: classic READ block 47 */
/* classic READ block 47 */
0xF1U, 0x90U, 0x25U, 0x51U,
/* STEP 73: classic AUTH NT sector 12 */
/* classic AUTH NT sector 12 */
0x60U, 0x30U,
/* STEP 74: classic AUTH NR|AR sector 12 */
/* classic AUTH NR|AR sector 12 */
0x2AU, 0x74U, 0x6BU, 0x8AU, 0x88U, 0xA1U, 0xA8U, 0x5FU,
/* STEP 75: classic READ block 48 */
/* classic READ block 48 */
0xECU, 0x28U, 0x86U, 0x6FU,
/* STEP 76: classic READ block 49 */
/* classic READ block 49 */
0x56U, 0x24U, 0x8BU, 0xBFU,
/* STEP 77: classic READ block 50 */
/* classic READ block 50 */
0xEDU, 0x46U, 0x98U, 0x45U,
/* STEP 78: classic READ block 51 */
/* classic READ block 51 */
0x25U, 0x76U, 0x49U, 0xEFU,
/* STEP 79: classic AUTH NT sector 13 */
/* classic AUTH NT sector 13 */
0x60U, 0x34U,
/* STEP 80: classic AUTH NR|AR sector 13 */
/* classic AUTH NR|AR sector 13 */
0x82U, 0x33U, 0x40U, 0xFDU, 0xA3U, 0x2CU, 0xABU, 0x9CU,
/* STEP 81: classic READ block 52 */
/* classic READ block 52 */
0xEBU, 0xF7U, 0xC2U, 0xF7U,
/* STEP 82: classic READ block 53 */
/* classic READ block 53 */
0x84U, 0xF7U, 0xB9U, 0x43U,
/* STEP 83: classic READ block 54 */
/* classic READ block 54 */
0x25U, 0xF7U, 0x69U, 0x60U,
/* STEP 84: classic READ block 55 */
/* classic READ block 55 */
0x5AU, 0x12U, 0xFEU, 0xB6U,
/* STEP 85: classic AUTH NT sector 14 */
/* classic AUTH NT sector 14 */
0x60U, 0x38U,
/* STEP 86: classic AUTH NR|AR sector 14 */
/* classic AUTH NR|AR sector 14 */
0xBAU, 0xE4U, 0xE5U, 0x64U, 0x58U, 0x23U, 0x2EU, 0xAEU,
/* STEP 87: classic READ block 56 */
/* classic READ block 56 */
0x3CU, 0x34U, 0xE8U, 0xD5U,
/* STEP 88: classic READ block 57 */
/* classic READ block 57 */
0x5DU, 0xADU, 0x18U, 0x35U,
/* STEP 89: classic READ block 58 */
/* classic READ block 58 */
0x07U, 0x24U, 0x5CU, 0x0DU,
/* STEP 90: classic READ block 59 */
/* classic READ block 59 */
0x2EU, 0x09U, 0xA7U, 0x5BU,
/* STEP 91: classic AUTH NT sector 15 */
/* classic AUTH NT sector 15 */
0x60U, 0x3CU,
/* STEP 92: classic AUTH NR|AR sector 15 */
/* classic AUTH NR|AR sector 15 */
0xD2U, 0x86U, 0x8DU, 0x62U, 0xDAU, 0x05U, 0xECU, 0xCFU,
/* STEP 93: classic READ block 60 */
/* classic READ block 60 */
0x09U, 0x9FU, 0xFDU, 0x0EU,
/* STEP 94: classic READ block 61 */
/* classic READ block 61 */
0x96U, 0x7FU, 0x63U, 0xDBU,
/* STEP 95: classic READ block 62 */
/* classic READ block 62 */
0xA3U, 0x93U, 0x4BU, 0x8BU,
/* STEP 96: classic READ block 63 */
/* classic READ block 63 */
0x27U, 0xA9U, 0xB4U, 0xB1U,
/* STEP 97: classic AUTH NT sector 16 */
/* classic AUTH NT sector 16 */
0x60U, 0x40U,
/* STEP 98: classic AUTH NR|AR sector 16 */
/* classic AUTH NR|AR sector 16 */
0xCAU, 0xFAU, 0xE5U, 0x2DU, 0x63U, 0xF7U, 0x7DU, 0xF8U,
/* STEP 99: classic READ block 64 */
/* classic READ block 64 */
0x1EU, 0x4AU, 0xE1U, 0x37U,
/* STEP 100: classic READ block 65 */
/* classic READ block 65 */
0xA7U, 0x2FU, 0x28U, 0x4BU,
/* STEP 101: classic READ block 66 */
/* classic READ block 66 */
0x8AU, 0x11U, 0xC1U, 0x1FU,
/* STEP 102: classic READ block 67 */
/* classic READ block 67 */
0xF6U, 0x96U, 0xC1U, 0x00U,
/* STEP 103: classic AUTH NT sector 17 */
/* classic AUTH NT sector 17 */
0x60U, 0x44U,
/* STEP 104: classic AUTH NR|AR sector 17 */
/* classic AUTH NR|AR sector 17 */
0x82U, 0xD6U, 0xC2U, 0xDCU, 0x8CU, 0x07U, 0xB8U, 0xFAU,
/* STEP 105: classic READ block 68 */
/* classic READ block 68 */
0xA3U, 0x3FU, 0xD0U, 0x5BU,
/* STEP 106: classic READ block 69 */
/* classic READ block 69 */
0x1DU, 0xD1U, 0xE4U, 0x88U,
/* STEP 107: classic READ block 70 */
/* classic READ block 70 */
0x01U, 0x45U, 0x73U, 0xE3U,
/* STEP 108: classic READ block 71 */
/* classic READ block 71 */
0x25U, 0x87U, 0x65U, 0xAAU,
/* STEP 109: classic AUTH NT sector 18 */
/* classic AUTH NT sector 18 */
0x60U, 0x48U,
/* STEP 110: classic AUTH NR|AR sector 18 */
/* classic AUTH NR|AR sector 18 */
0xC2U, 0x66U, 0x69U, 0x81U, 0x46U, 0x1EU, 0xD8U, 0xEFU,
/* STEP 111: classic READ block 72 */
/* classic READ block 72 */
0x9AU, 0xDDU, 0x96U, 0x6EU,
/* STEP 112: classic READ block 73 */
/* classic READ block 73 */
0x5DU, 0x7BU, 0x3DU, 0xBFU,
/* STEP 113: classic READ block 74 */
/* classic READ block 74 */
0x10U, 0x84U, 0x9FU, 0x8FU,
/* STEP 114: classic READ block 75 */
/* classic READ block 75 */
0xBEU, 0x59U, 0x49U, 0xC4U,
/* STEP 115: classic AUTH NT sector 19 */
/* classic AUTH NT sector 19 */
0x60U, 0x4CU,
/* STEP 116: classic AUTH NR|AR sector 19 */
/* classic AUTH NR|AR sector 19 */
0x82U, 0x26U, 0x4FU, 0xF5U, 0x6AU, 0xC3U, 0x48U, 0x07U,
/* STEP 117: classic READ block 76 */
/* classic READ block 76 */
0xB5U, 0x7CU, 0xB3U, 0xEAU,
/* STEP 118: classic READ block 77 */
/* classic READ block 77 */
0x93U, 0xB4U, 0xB6U, 0x61U,
/* STEP 119: classic READ block 78 */
/* classic READ block 78 */
0xF0U, 0x56U, 0x23U, 0xD5U,
/* STEP 120: classic READ block 79 */
/* classic READ block 79 */
0x6BU, 0x4EU, 0x44U, 0x0FU,
/* STEP 121: classic AUTH NT sector 20 */
/* classic AUTH NT sector 20 */
0x60U, 0x50U,
/* STEP 122: classic AUTH NR|AR sector 20 */
/* classic AUTH NR|AR sector 20 */
0xEAU, 0xA6U, 0x83U, 0x17U, 0x38U, 0x56U, 0x85U, 0xD6U,
/* STEP 123: classic READ block 80 */
/* classic READ block 80 */
0xD7U, 0x03U, 0x7AU, 0x13U,
/* STEP 124: classic READ block 81 */
/* classic READ block 81 */
0xB2U, 0x93U, 0xCDU, 0xA4U,
/* STEP 125: classic READ block 82 */
/* classic READ block 82 */
0x9DU, 0x64U, 0x9DU, 0x86U,
/* STEP 126: classic READ block 83 */
/* classic READ block 83 */
0x69U, 0x91U, 0x2CU, 0x3FU,
/* STEP 127: classic AUTH NT sector 21 */
/* classic AUTH NT sector 21 */
0x60U, 0x54U,
/* STEP 128: classic AUTH NR|AR sector 21 */
/* classic AUTH NR|AR sector 21 */
0xA0U, 0x15U, 0x53U, 0xDEU, 0x53U, 0x87U, 0x8EU, 0xDDU,
/* STEP 129: classic READ block 84 */
/* classic READ block 84 */
0xD4U, 0x12U, 0x18U, 0x11U,
/* STEP 130: classic READ block 85 */
/* classic READ block 85 */
0xAFU, 0xDFU, 0x78U, 0x6DU,
/* STEP 131: classic READ block 86 */
/* classic READ block 86 */
0x73U, 0xCCU, 0xFFU, 0xB1U,
/* STEP 132: classic READ block 87 */
/* classic READ block 87 */
0x6EU, 0x5BU, 0x36U, 0x74U,
/* STEP 133: classic AUTH NT sector 22 */
/* classic AUTH NT sector 22 */
0x60U, 0x58U,
/* STEP 134: classic AUTH NR|AR sector 22 */
/* classic AUTH NR|AR sector 22 */
0xEAU, 0x7EU, 0xF5U, 0x4DU, 0xE7U, 0xD6U, 0xAAU, 0xB2U,
/* STEP 135: classic READ block 88 */
/* classic READ block 88 */
0xBEU, 0xDCU, 0x1BU, 0xF5U,
/* STEP 136: classic READ block 89 */
/* classic READ block 89 */
0x1EU, 0xCDU, 0x81U, 0x30U,
/* STEP 137: classic READ block 90 */
/* classic READ block 90 */
0xB5U, 0xA9U, 0xEFU, 0xF8U,
/* STEP 138: classic READ block 91 */
/* classic READ block 91 */
0x8BU, 0x7FU, 0xA2U, 0xF0U,
/* STEP 139: classic AUTH NT sector 23 */
/* classic AUTH NT sector 23 */
0x60U, 0x5CU,
/* STEP 140: classic AUTH NR|AR sector 23 */
/* classic AUTH NR|AR sector 23 */
0x20U, 0x6BU, 0xB1U, 0xBFU, 0xE7U, 0x24U, 0xB7U, 0x96U,
/* STEP 141: classic READ block 92 */
/* classic READ block 92 */
0x9BU, 0xBCU, 0x1AU, 0x0BU,
/* STEP 142: classic READ block 93 */
/* classic READ block 93 */
0xB7U, 0x5BU, 0xB5U, 0xE3U,
/* STEP 143: classic READ block 94 */
/* classic READ block 94 */
0x7FU, 0x80U, 0xB4U, 0x68U,
/* STEP 144: classic READ block 95 */
/* classic READ block 95 */
0x7CU, 0x5EU, 0x37U, 0x6EU,
/* STEP 145: classic AUTH NT sector 24 */
/* classic AUTH NT sector 24 */
0x60U, 0x60U,
/* STEP 146: classic AUTH NR|AR sector 24 */
/* classic AUTH NR|AR sector 24 */
0x8AU, 0x64U, 0xA5U, 0xF2U, 0x16U, 0x55U, 0x96U, 0x7BU,
/* STEP 147: classic READ block 96 */
/* classic READ block 96 */
0x76U, 0xFBU, 0x34U, 0x01U,
/* STEP 148: classic READ block 97 */
/* classic READ block 97 */
0xDEU, 0x2AU, 0x6EU, 0xB5U,
/* STEP 149: classic READ block 98 */
/* classic READ block 98 */
0x91U, 0x4CU, 0xA8U, 0x2FU,
/* STEP 150: classic READ block 99 */
/* classic READ block 99 */
0x67U, 0x10U, 0xDCU, 0xFDU,
/* STEP 151: classic AUTH NT sector 25 */
/* classic AUTH NT sector 25 */
0x60U, 0x64U,
/* STEP 152: classic AUTH NR|AR sector 25 */
/* classic AUTH NR|AR sector 25 */
0x80U, 0xE6U, 0xFAU, 0xE1U, 0x2EU, 0xF1U, 0xEAU, 0xE0U,
/* STEP 153: classic READ block 100 */
/* classic READ block 100 */
0x43U, 0xECU, 0xA2U, 0x5EU,
/* STEP 154: classic READ block 101 */
/* classic READ block 101 */
0x60U, 0x6AU, 0x67U, 0xD5U,
/* STEP 155: classic READ block 102 */
/* classic READ block 102 */
0x89U, 0x42U, 0xE5U, 0xA6U,
/* STEP 156: classic READ block 103 */
/* classic READ block 103 */
0x82U, 0x3EU, 0xA0U, 0x39U,
/* STEP 157: classic AUTH NT sector 26 */
/* classic AUTH NT sector 26 */
0x60U, 0x68U,
/* STEP 158: classic AUTH NR|AR sector 26 */
/* classic AUTH NR|AR sector 26 */
0x12U, 0xF6U, 0x33U, 0x2FU, 0x5AU, 0x53U, 0x70U, 0x79U,
/* STEP 159: classic READ block 104 */
/* classic READ block 104 */
0x9EU, 0x22U, 0xDCU, 0x56U,
/* STEP 160: classic READ block 105 */
/* classic READ block 105 */
0xA9U, 0xB7U, 0xAAU, 0xB4U,
/* STEP 161: classic READ block 106 */
/* classic READ block 106 */
0xFCU, 0x78U, 0x5CU, 0x94U,
/* STEP 162: classic READ block 107 */
/* classic READ block 107 */
0xCAU, 0x7BU, 0x90U, 0xBFU,
/* STEP 163: classic AUTH NT sector 27 */
/* classic AUTH NT sector 27 */
0x60U, 0x6CU,
/* STEP 164: classic AUTH NR|AR sector 27 */
/* classic AUTH NR|AR sector 27 */
0x92U, 0x78U, 0x92U, 0x8DU, 0x50U, 0xDDU, 0x1EU, 0x39U,
/* STEP 165: classic READ block 108 */
/* classic READ block 108 */
0xA1U, 0x52U, 0x80U, 0xDDU,
/* STEP 166: classic READ block 109 */
/* classic READ block 109 */
0xE5U, 0x24U, 0x75U, 0xA3U,
/* STEP 167: classic READ block 110 */
/* classic READ block 110 */
0x84U, 0xC1U, 0x92U, 0x82U,
/* STEP 168: classic READ block 111 */
/* classic READ block 111 */
0x30U, 0x71U, 0x60U, 0xACU,
/* STEP 169: classic AUTH NT sector 28 */
/* classic AUTH NT sector 28 */
0x60U, 0x70U,
/* STEP 170: classic AUTH NR|AR sector 28 */
/* classic AUTH NR|AR sector 28 */
0xAAU, 0xD2U, 0x3DU, 0x3DU, 0xC7U, 0xC4U, 0xA0U, 0xB5U,
/* STEP 171: classic READ block 112 */
/* classic READ block 112 */
0x0CU, 0xD8U, 0x41U, 0xADU,
/* STEP 172: classic READ block 113 */
/* classic READ block 113 */
0x8FU, 0x9DU, 0x5FU, 0xEFU,
/* STEP 173: classic READ block 114 */
/* classic READ block 114 */
0x9BU, 0x26U, 0xBDU, 0x7BU,
/* STEP 174: classic READ block 115 */
/* classic READ block 115 */
0x58U, 0x3CU, 0x44U, 0xE4U,
/* STEP 175: classic AUTH NT sector 29 */
/* classic AUTH NT sector 29 */
0x60U, 0x74U,
/* STEP 176: classic AUTH NR|AR sector 29 */
/* classic AUTH NR|AR sector 29 */
0xA0U, 0xC7U, 0x4FU, 0x6CU, 0xE1U, 0x0DU, 0xCFU, 0x19U,
/* STEP 177: classic READ block 116 */
/* classic READ block 116 */
0x27U, 0x71U, 0x06U, 0xA0U,
/* STEP 178: classic READ block 117 */
/* classic READ block 117 */
0xCDU, 0xD2U, 0x5EU, 0x06U,
/* STEP 179: classic READ block 118 */
/* classic READ block 118 */
0xE8U, 0x54U, 0x5CU, 0xDCU,
/* STEP 180: classic READ block 119 */
/* classic READ block 119 */
0xA9U, 0x19U, 0x0AU, 0x26U,
/* STEP 181: classic AUTH NT sector 30 */
/* classic AUTH NT sector 30 */
0x60U, 0x78U,
/* STEP 182: classic AUTH NR|AR sector 30 */
/* classic AUTH NR|AR sector 30 */
0xB2U, 0xBEU, 0x51U, 0xF7U, 0xB9U, 0x20U, 0x44U, 0xA6U,
/* STEP 183: classic READ block 120 */
/* classic READ block 120 */
0xE3U, 0x14U, 0x05U, 0x05U,
/* STEP 184: classic READ block 121 */
/* classic READ block 121 */
0x33U, 0x9BU, 0xC5U, 0x35U,
/* STEP 185: classic READ block 122 */
/* classic READ block 122 */
0x7BU, 0x46U, 0xEAU, 0xFFU,
/* STEP 186: classic READ block 123 */
/* classic READ block 123 */
0x31U, 0x20U, 0x3EU, 0xBDU,
/* STEP 187: classic AUTH NT sector 31 */
/* classic AUTH NT sector 31 */
0x60U, 0x7CU,
/* STEP 188: classic AUTH NR|AR sector 31 */
/* classic AUTH NR|AR sector 31 */
0xB0U, 0xAAU, 0x38U, 0xBBU, 0x2AU, 0xB2U, 0x4EU, 0x1CU,
/* STEP 189: classic READ block 124 */
/* classic READ block 124 */
0x84U, 0xEDU, 0xC5U, 0xB5U,
/* STEP 190: classic READ block 125 */
/* classic READ block 125 */
0xD1U, 0xB7U, 0xC9U, 0xA6U,
/* STEP 191: classic READ block 126 */
/* classic READ block 126 */
0x6BU, 0xCCU, 0x37U, 0x68U,
/* STEP 192: classic READ block 127 */
/* classic READ block 127 */
0x57U, 0x32U, 0x2DU, 0x28U,
/* STEP 193: classic AUTH NT sector 32 */
/* classic AUTH NT sector 32 */
0x60U, 0x80U,
/* STEP 194: classic AUTH NR|AR sector 32 */
/* classic AUTH NR|AR sector 32 */
0x2AU, 0xF5U, 0x53U, 0x52U, 0x7AU, 0x96U, 0x99U, 0x55U,
/* STEP 195: classic READ block 128 */
/* classic READ block 128 */
0xD6U, 0xB7U, 0xA7U, 0x2BU,
/* STEP 196: classic READ block 129 */
/* classic READ block 129 */
0xC1U, 0x2EU, 0xA6U, 0x82U,
/* STEP 197: classic READ block 130 */
/* classic READ block 130 */
0x8FU, 0xD3U, 0xF0U, 0xF1U,
/* STEP 198: classic READ block 131 */
/* classic READ block 131 */
0x64U, 0xF5U, 0xEBU, 0x7EU,
/* STEP 199: classic READ block 132 */
/* classic READ block 132 */
0xFBU, 0x05U, 0x19U, 0xC1U,
/* STEP 200: classic READ block 133 */
/* classic READ block 133 */
0xBAU, 0xB9U, 0xD9U, 0x50U,
/* STEP 201: classic READ block 134 */
/* classic READ block 134 */
0x7DU, 0xF2U, 0xF2U, 0x23U,
/* STEP 202: classic READ block 135 */
/* classic READ block 135 */
0xBDU, 0x09U, 0x39U, 0x9BU,
/* STEP 203: classic READ block 136 */
/* classic READ block 136 */
0x1AU, 0x7CU, 0x28U, 0x61U,
/* STEP 204: classic READ block 137 */
/* classic READ block 137 */
0x9CU, 0x77U, 0xE3U, 0x07U,
/* STEP 205: classic READ block 138 */
/* classic READ block 138 */
0x86U, 0x0DU, 0xE6U, 0x68U,
/* STEP 206: classic READ block 139 */
/* classic READ block 139 */
0x27U, 0x07U, 0x0BU, 0x49U,
/* STEP 207: classic READ block 140 */
/* classic READ block 140 */
0xAFU, 0xB6U, 0x75U, 0x7BU,
/* STEP 208: classic READ block 141 */
/* classic READ block 141 */
0x90U, 0x5BU, 0x60U, 0x09U,
/* STEP 209: classic READ block 142 */
/* classic READ block 142 */
0x6BU, 0x07U, 0x72U, 0xADU,
/* STEP 210: classic READ block 143 */
/* classic READ block 143 */
0xFDU, 0x41U, 0xDEU, 0xE2U,
/* STEP 211: classic AUTH NT sector 33 */
/* classic AUTH NT sector 33 */
0x60U, 0x90U,
/* STEP 212: classic AUTH NR|AR sector 33 */
/* classic AUTH NR|AR sector 33 */
0x62U, 0xECU, 0x71U, 0x37U, 0xE4U, 0xD2U, 0x7BU, 0xA0U,
/* STEP 213: classic READ block 144 */
/* classic READ block 144 */
0xB6U, 0xF4U, 0xD3U, 0xABU,
/* STEP 214: classic READ block 145 */
/* classic READ block 145 */
0x8CU, 0x3EU, 0xD1U, 0x16U,
/* STEP 215: classic READ block 146 */
/* classic READ block 146 */
0x75U, 0xDFU, 0x46U, 0xE6U,
/* STEP 216: classic READ block 147 */
/* classic READ block 147 */
0x93U, 0x32U, 0x86U, 0x03U,
/* STEP 217: classic READ block 148 */
/* classic READ block 148 */
0xB8U, 0x3FU, 0x16U, 0xEDU,
/* STEP 218: classic READ block 149 */
/* classic READ block 149 */
0xEDU, 0xA9U, 0x1FU, 0x45U,
/* STEP 219: classic READ block 150 */
/* classic READ block 150 */
0xC9U, 0xC8U, 0x34U, 0xCAU,
/* STEP 220: classic READ block 151 */
/* classic READ block 151 */
0xCBU, 0xD5U, 0x2CU, 0xBCU,
/* STEP 221: classic READ block 152 */
/* classic READ block 152 */
0xB9U, 0x3EU, 0x14U, 0x28U,
/* STEP 222: classic READ block 153 */
/* classic READ block 153 */
0x48U, 0xF9U, 0xADU, 0x28U,
/* STEP 223: classic READ block 154 */
/* classic READ block 154 */
0xACU, 0xBBU, 0xC0U, 0x4FU,
/* STEP 224: classic READ block 155 */
/* classic READ block 155 */
0xBEU, 0xA9U, 0xDEU, 0x88U,
/* STEP 225: classic READ block 156 */
/* classic READ block 156 */
0xC8U, 0x70U, 0xA3U, 0x27U,
/* STEP 226: classic READ block 157 */
/* classic READ block 157 */
0x80U, 0x73U, 0xD8U, 0xCAU,
/* STEP 227: classic READ block 158 */
/* classic READ block 158 */
0xE9U, 0x73U, 0xE3U, 0x44U,
/* STEP 228: classic READ block 159 */
/* classic READ block 159 */
0x86U, 0xF2U, 0x00U, 0x0EU,
/* STEP 229: classic AUTH NT sector 34 */
/* classic AUTH NT sector 34 */
0x60U, 0xA0U,
/* STEP 230: classic AUTH NR|AR sector 34 */
/* classic AUTH NR|AR sector 34 */
0xAAU, 0xA1U, 0x2AU, 0x6DU, 0x8AU, 0x15U, 0x4DU, 0x35U,
/* STEP 231: classic READ block 160 */
/* classic READ block 160 */
0x0CU, 0x12U, 0x16U, 0x05U,
/* STEP 232: classic READ block 161 */
/* classic READ block 161 */
0x86U, 0x42U, 0x49U, 0x3EU,
/* STEP 233: classic READ block 162 */
/* classic READ block 162 */
0x55U, 0x89U, 0xA4U, 0x7FU,
/* STEP 234: classic READ block 163 */
/* classic READ block 163 */
0x95U, 0x14U, 0x0CU, 0x52U,
/* STEP 235: classic READ block 164 */
/* classic READ block 164 */
0xDAU, 0xF1U, 0x24U, 0xC4U,
/* STEP 236: classic READ block 165 */
/* classic READ block 165 */
0x98U, 0x78U, 0xABU, 0xD1U,
/* STEP 237: classic READ block 166 */
/* classic READ block 166 */
0x40U, 0xE1U, 0x55U, 0xCAU,
/* STEP 238: classic READ block 167 */
/* classic READ block 167 */
0x82U, 0x4BU, 0x57U, 0x51U,
/* STEP 239: classic READ block 168 */
/* classic READ block 168 */
0x3DU, 0x8DU, 0x1BU, 0x8EU,
/* STEP 240: classic READ block 169 */
/* classic READ block 169 */
0x5DU, 0xABU, 0xA5U, 0x09U,
/* STEP 241: classic READ block 170 */
/* classic READ block 170 */
0x52U, 0xBAU, 0x97U, 0x31U,
/* STEP 242: classic READ block 171 */
/* classic READ block 171 */
0x9BU, 0x63U, 0xBDU, 0xABU,
/* STEP 243: classic READ block 172 */
/* classic READ block 172 */
0x50U, 0x11U, 0x08U, 0xACU,
/* STEP 244: classic READ block 173 */
/* classic READ block 173 */
0x45U, 0x80U, 0xE5U, 0xC1U,
/* STEP 245: classic READ block 174 */
/* classic READ block 174 */
0x5BU, 0x16U, 0xAFU, 0x47U,
/* STEP 246: classic READ block 175 */
/* classic READ block 175 */
0x86U, 0x41U, 0x95U, 0xE7U,
/* STEP 247: classic AUTH NT sector 35 */
/* classic AUTH NT sector 35 */
0x60U, 0xB0U,
/* STEP 248: classic AUTH NR|AR sector 35 */
/* classic AUTH NR|AR sector 35 */
0xE0U, 0xFEU, 0x75U, 0xF5U, 0x93U, 0xC9U, 0x9AU, 0x1AU,
/* STEP 249: classic READ block 176 */
/* classic READ block 176 */
0x91U, 0xD5U, 0x77U, 0xB8U,
/* STEP 250: classic READ block 177 */
/* classic READ block 177 */
0x51U, 0xBDU, 0x74U, 0x94U,
/* STEP 251: classic READ block 178 */
/* classic READ block 178 */
0xB9U, 0x2AU, 0x82U, 0x24U,
/* STEP 252: classic READ block 179 */
/* classic READ block 179 */
0x7BU, 0x56U, 0x63U, 0x7EU,
/* STEP 253: classic READ block 180 */
/* classic READ block 180 */
0x39U, 0x18U, 0xA2U, 0x62U,
/* STEP 254: classic READ block 181 */
/* classic READ block 181 */
0x22U, 0x2DU, 0x95U, 0xEBU,
/* STEP 255: classic READ block 182 */
/* classic READ block 182 */
0x2CU, 0xDCU, 0x2FU, 0x34U,
/* STEP 256: classic READ block 183 */
/* classic READ block 183 */
0xC1U, 0x9AU, 0x06U, 0xD1U,
/* STEP 257: classic READ block 184 */
/* classic READ block 184 */
0x46U, 0x2DU, 0x9DU, 0xCCU,
/* STEP 258: classic READ block 185 */
/* classic READ block 185 */
0x48U, 0x5AU, 0xBEU, 0x84U,
/* STEP 259: classic READ block 186 */
/* classic READ block 186 */
0xB3U, 0x2DU, 0xA4U, 0xB5U,
/* STEP 260: classic READ block 187 */
/* classic READ block 187 */
0x48U, 0x53U, 0x3CU, 0x3AU,
/* STEP 261: classic READ block 188 */
/* classic READ block 188 */
0x1FU, 0x0CU, 0x80U, 0xD8U,
/* STEP 262: classic READ block 189 */
/* classic READ block 189 */
0x6BU, 0x2FU, 0xA2U, 0xBDU,
/* STEP 263: classic READ block 190 */
/* classic READ block 190 */
0x1AU, 0xE2U, 0x5CU, 0x1CU,
/* STEP 264: classic READ block 191 */
/* classic READ block 191 */
0xB0U, 0x32U, 0x6FU, 0xC1U,
/* STEP 265: classic AUTH NT sector 36 */
/* classic AUTH NT sector 36 */
0x60U, 0xC0U,
/* STEP 266: classic AUTH NR|AR sector 36 */
/* classic AUTH NR|AR sector 36 */
0xA2U, 0x7EU, 0x31U, 0xDDU, 0x07U, 0x68U, 0xC6U, 0xD3U,
/* STEP 267: classic READ block 192 */
/* classic READ block 192 */
0xF8U, 0x52U, 0x2DU, 0x64U,
/* STEP 268: classic READ block 193 */
/* classic READ block 193 */
0xE8U, 0x24U, 0xACU, 0x00U,
/* STEP 269: classic READ block 194 */
/* classic READ block 194 */
0x05U, 0x83U, 0x20U, 0x08U,
/* STEP 270: classic READ block 195 */
/* classic READ block 195 */
0x0DU, 0xD5U, 0x79U, 0x55U,
/* STEP 271: classic READ block 196 */
/* classic READ block 196 */
0xAEU, 0x33U, 0x43U, 0xF5U,
/* STEP 272: classic READ block 197 */
/* classic READ block 197 */
0x11U, 0x22U, 0x5CU, 0x4BU,
/* STEP 273: classic READ block 198 */
/* classic READ block 198 */
0x3BU, 0xEDU, 0xC8U, 0x8FU,
/* STEP 274: classic READ block 199 */
/* classic READ block 199 */
0xA0U, 0xF3U, 0x66U, 0xE3U,
/* STEP 275: classic READ block 200 */
/* classic READ block 200 */
0x36U, 0x0DU, 0xCCU, 0x7DU,
/* STEP 276: classic READ block 201 */
/* classic READ block 201 */
0xFFU, 0xFFU, 0xB7U, 0x1FU,
/* STEP 277: classic READ block 202 */
/* classic READ block 202 */
0xE3U, 0xC2U, 0x1FU, 0x6AU,
/* STEP 278: classic READ block 203 */
/* classic READ block 203 */
0x6BU, 0xD4U, 0xDFU, 0x08U,
/* STEP 279: classic READ block 204 */
/* classic READ block 204 */
0x41U, 0xEAU, 0x73U, 0xC6U,
/* STEP 280: classic READ block 205 */
/* classic READ block 205 */
0xD2U, 0xD6U, 0x45U, 0xD7U,
/* STEP 281: classic READ block 206 */
/* classic READ block 206 */
0x8BU, 0x72U, 0xCDU, 0x6FU,
/* STEP 282: classic READ block 207 */
/* classic READ block 207 */
0x95U, 0x43U, 0xD6U, 0x69U,
/* STEP 283: classic AUTH NT sector 37 */
/* classic AUTH NT sector 37 */
0x60U, 0xD0U,
/* STEP 284: classic AUTH NR|AR sector 37 */
/* classic AUTH NR|AR sector 37 */
0xC0U, 0xD0U, 0x9DU, 0x7EU, 0x0EU, 0xD9U, 0xD8U, 0xF8U,
/* STEP 285: classic READ block 208 */
/* classic READ block 208 */
0xA6U, 0xA2U, 0xA9U, 0x6AU,
/* STEP 286: classic READ block 209 */
/* classic READ block 209 */
0x8CU, 0xBEU, 0x42U, 0x87U,
/* STEP 287: classic READ block 210 */
/* classic READ block 210 */
0xCEU, 0x03U, 0xBCU, 0xCBU,
/* STEP 288: classic READ block 211 */
/* classic READ block 211 */
0x06U, 0x5AU, 0x06U, 0x4FU,
/* STEP 289: classic READ block 212 */
/* classic READ block 212 */
0xD1U, 0x8AU, 0x27U, 0xBCU,
/* STEP 290: classic READ block 213 */
/* classic READ block 213 */
0xBFU, 0x54U, 0x91U, 0x0AU,
/* STEP 291: classic READ block 214 */
/* classic READ block 214 */
0xB2U, 0x1DU, 0x79U, 0x52U,
/* STEP 292: classic READ block 215 */
/* classic READ block 215 */
0xA8U, 0xE1U, 0xA5U, 0xC1U,
/* STEP 293: classic READ block 216 */
/* classic READ block 216 */
0xF1U, 0x6EU, 0x9CU, 0x80U,
/* STEP 294: classic READ block 217 */
/* classic READ block 217 */
0x32U, 0x36U, 0x8AU, 0x67U,
/* STEP 295: classic READ block 218 */
/* classic READ block 218 */
0xC2U, 0xD8U, 0x52U, 0x0EU,
/* STEP 296: classic READ block 219 */
/* classic READ block 219 */
0x09U, 0xFAU, 0xB8U, 0x20U,
/* STEP 297: classic READ block 220 */
/* classic READ block 220 */
0x88U, 0x86U, 0x3AU, 0xF6U,
/* STEP 298: classic READ block 221 */
/* classic READ block 221 */
0x01U, 0x33U, 0x9DU, 0xC7U,
/* STEP 299: classic READ block 222 */
/* classic READ block 222 */
0x5EU, 0x4FU, 0x0CU, 0x4CU,
/* STEP 300: classic READ block 223 */
/* classic READ block 223 */
0xE0U, 0x71U, 0x10U, 0x1BU,
/* STEP 301: classic AUTH NT sector 38 */
/* classic AUTH NT sector 38 */
0x60U, 0xE0U,
/* STEP 302: classic AUTH NR|AR sector 38 */
/* classic AUTH NR|AR sector 38 */
0xA2U, 0xEEU, 0x9FU, 0x92U, 0xACU, 0x8AU, 0x15U, 0xDEU,
/* STEP 303: classic READ block 224 */
/* classic READ block 224 */
0x4AU, 0x1CU, 0xDBU, 0x27U,
/* STEP 304: classic READ block 225 */
/* classic READ block 225 */
0x2AU, 0x01U, 0xB9U, 0x1AU,
/* STEP 305: classic READ block 226 */
/* classic READ block 226 */
0xAFU, 0x69U, 0xCEU, 0x85U,
/* STEP 306: classic READ block 227 */
/* classic READ block 227 */
0x44U, 0xB8U, 0x1AU, 0x47U,
/* STEP 307: classic READ block 228 */
/* classic READ block 228 */
0xE5U, 0xBEU, 0x85U, 0xFFU,
/* STEP 308: classic READ block 229 */
/* classic READ block 229 */
0x59U, 0x03U, 0xF7U, 0x51U,
/* STEP 309: classic READ block 230 */
/* classic READ block 230 */
0xE5U, 0x7BU, 0x8EU, 0xFFU,
/* STEP 310: classic READ block 231 */
/* classic READ block 231 */
0x74U, 0x00U, 0x86U, 0xEBU,
/* STEP 311: classic READ block 232 */
/* classic READ block 232 */
0xB1U, 0xC3U, 0x2BU, 0xD7U,
/* STEP 312: classic READ block 233 */
/* classic READ block 233 */
0x00U, 0x07U, 0xBAU, 0xB2U,
/* STEP 313: classic READ block 234 */
/* classic READ block 234 */
0x95U, 0x1FU, 0x35U, 0x5FU,
/* STEP 314: classic READ block 235 */
/* classic READ block 235 */
0xDBU, 0xC6U, 0xC0U, 0xDAU,
/* STEP 315: classic READ block 236 */
/* classic READ block 236 */
0xABU, 0xBFU, 0x67U, 0x10U,
/* STEP 316: classic READ block 237 */
/* classic READ block 237 */
0xA8U, 0x64U, 0x1BU, 0x04U,
/* STEP 317: classic READ block 238 */
/* classic READ block 238 */
0x59U, 0xADU, 0xCDU, 0xA0U,
/* STEP 318: classic READ block 239 */
/* classic READ block 239 */
0x3DU, 0x20U, 0x3AU, 0x07U,
/* STEP 319: classic AUTH NT sector 39 */
/* classic AUTH NT sector 39 */
0x60U, 0xF0U,
/* STEP 320: classic AUTH NR|AR sector 39 */
/* classic AUTH NR|AR sector 39 */
0xC2U, 0x6EU, 0xEBU, 0xBCU, 0x57U, 0xB2U, 0xE4U, 0x6BU,
/* STEP 321: classic READ block 240 */
/* classic READ block 240 */
0x6FU, 0x6AU, 0xF3U, 0xB5U,
/* STEP 322: classic READ block 241 */
/* classic READ block 241 */
0xB9U, 0x82U, 0xC5U, 0xA2U,
/* STEP 323: classic READ block 242 */
/* classic READ block 242 */
0x05U, 0xBBU, 0xB0U, 0x28U,
/* STEP 324: classic READ block 243 */
/* classic READ block 243 */
0x1FU, 0xABU, 0x5DU, 0x4BU,
/* STEP 325: classic READ block 244 */
/* classic READ block 244 */
0x0FU, 0x29U, 0xB7U, 0xA6U,
/* STEP 326: classic READ block 245 */
/* classic READ block 245 */
0x1BU, 0x2EU, 0x44U, 0x09U,
/* STEP 327: classic READ block 246 */
/* classic READ block 246 */
0x53U, 0x25U, 0x40U, 0x93U,
/* STEP 328: classic READ block 247 */
/* classic READ block 247 */
0x86U, 0xA0U, 0xEBU, 0xD0U,
/* STEP 329: classic READ block 248 */
/* classic READ block 248 */
0xFCU, 0x98U, 0xECU, 0xC5U,
/* STEP 330: classic READ block 249 */
/* classic READ block 249 */
0xACU, 0x05U, 0xF9U, 0x56U,
/* STEP 331: classic READ block 250 */
/* classic READ block 250 */
0xB5U, 0x8FU, 0xACU, 0x63U,
/* STEP 332: classic READ block 251 */
/* classic READ block 251 */
0x9EU, 0xA5U, 0xDCU, 0x48U,
/* STEP 333: classic READ block 252 */
/* classic READ block 252 */
0x3DU, 0xEFU, 0x68U, 0x22U,
/* STEP 334: classic READ block 253 */
/* classic READ block 253 */
0x0EU, 0xA5U, 0x7AU, 0xDCU,
/* STEP 335: classic READ block 254 */
/* classic READ block 254 */
0xF6U, 0x1AU, 0x12U, 0xF5U,
/* STEP 336: classic READ block 255 */
/* classic READ block 255 */
0x07U, 0x53U, 0xF8U, 0xE0U,
