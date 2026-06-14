#!/usr/bin/env python3
"""3DES-2KEY CBC helpers matching Flipper mf_ultralight (mbedtls DES3 2-key mode)."""

from __future__ import annotations

from Crypto.Cipher import DES


def shift_data(block: bytes) -> bytes:
    data = bytearray(block)
    buf = data[0]
    for i in range(1, len(data)):
        data[i - 1] = data[i]
    data[-1] = buf
    return bytes(data)


def _des_ecb(key8: bytes, block: bytes, decrypt: bool) -> bytes:
    cipher = DES.new(key8, DES.MODE_ECB)
    if decrypt:
        return cipher.decrypt(block)
    return cipher.encrypt(block)


def _tdes2_block(key16: bytes, block: bytes, decrypt: bool) -> bytes:
    k1, k2 = key16[:8], key16[8:16]
    if not decrypt:
        block = _des_ecb(k1, block, False)
        block = _des_ecb(k2, block, True)
        return _des_ecb(k1, block, False)
    block = _des_ecb(k1, block, True)
    block = _des_ecb(k2, block, False)
    return _des_ecb(k1, block, True)


def encrypt(key16: bytes, iv: bytes, data: bytes) -> bytes:
    out = bytearray()
    prev = iv
    for off in range(0, len(data), 8):
        block = bytes(a ^ b for a, b in zip(data[off : off + 8], prev))
        block = _tdes2_block(key16, block, False)
        out.extend(block)
        prev = block
    return bytes(out)


def decrypt(key16: bytes, iv: bytes, data: bytes) -> bytes:
    out = bytearray()
    prev = iv
    for off in range(0, len(data), 8):
        block = data[off : off + 8]
        plain = _tdes2_block(key16, block, True)
        out.extend(bytes(a ^ b for a, b in zip(plain, prev)))
        prev = block
    return bytes(out)


def auth_probe_response(key16: bytes, rnd_b: bytes) -> bytes:
    enc = encrypt(key16, b"\x00" * 8, rnd_b)
    return bytes([0xAF]) + enc
