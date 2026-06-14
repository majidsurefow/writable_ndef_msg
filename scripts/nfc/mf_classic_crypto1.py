"""Crypto1 helpers for deterministic Classic fixture generation (Flipper crypto1.c)."""

from __future__ import annotations

LF_POLY_ODD = 0x29CE5C
LF_POLY_EVEN = 0x870804


def _swapendian(x: int) -> int:
    x &= 0xFFFFFFFF
    x = ((x >> 8) & 0x00FF00FF) | ((x & 0x00FF00FF) << 8)
    x = (x >> 16) | ((x & 0xFFFF) << 16)
    return x & 0xFFFFFFFF


def _bebit(x: int, n: int) -> int:
    return (x >> (n ^ 24)) & 1


def _even_parity32(x: int) -> int:
    x &= 0xFFFFFFFF
    x ^= x >> 16
    x ^= x >> 8
    return (0x6996 >> (x & 0x0F)) & 1


def _odd_parity8(x: int) -> int:
    x &= 0xFF
    x ^= x >> 4
    x ^= x >> 2
    x ^= x >> 1
    return x & 1


class Crypto1:
    def __init__(self) -> None:
        self.odd = 0
        self.even = 0

    def init(self, key: int) -> None:
        self.odd = 0
        self.even = 0
        i = 47
        while i > 0:
            self.odd = (self.odd << 1) | ((key >> ((i - 1) ^ 7)) & 1)
            self.even = (self.even << 1) | ((key >> (i ^ 7)) & 1)
            i -= 2

    @staticmethod
    def _filter(inp: int) -> int:
        out = 0
        out = (0xF22C0 >> (inp & 0xF)) & 16
        out |= (0x6C9C0 >> ((inp >> 4) & 0xF)) & 8
        out |= (0x3C8B0 >> ((inp >> 8) & 0xF)) & 4
        out |= (0x1E458 >> ((inp >> 12) & 0xF)) & 2
        out |= (0x0D938 >> ((inp >> 16) & 0xF)) & 1
        return (0xEC57E80A >> out) & 1

    def bit(self, inp: int, is_encrypted: int) -> int:
        out = self._filter(self.odd)
        feed = out & int(bool(is_encrypted))
        feed ^= int(bool(inp))
        feed ^= LF_POLY_ODD & self.odd
        feed ^= LF_POLY_EVEN & self.even
        self.even = (self.even << 1) | _even_parity32(feed)
        self.odd, self.even = self.even, self.odd
        return out

    def byte(self, inp: int, is_encrypted: int) -> int:
        out = 0
        for i in range(8):
            out |= self.bit((inp >> i) & 1, is_encrypted) << i
        return out & 0xFF

    def word(self, inp: int, is_encrypted: int) -> int:
        out = 0
        for i in range(32):
            out |= self.bit(_bebit(inp, i), is_encrypted) << (24 ^ i)
        return out & 0xFFFFFFFF

    @staticmethod
    def prng_successor(x: int, n: int) -> int:
        x = _swapendian(x)
        for _ in range(n):
            x = (x >> 1) | (
                (x >> 16 ^ x >> 18 ^ x >> 19 ^ x >> 21) << 31
            ) & 0xFFFFFFFF
        return _swapendian(x)

    def encrypt_bytes(self, plain: bytes, keystream: bytes | None = None) -> bytes:
        out = bytearray()
        for i, pb in enumerate(plain):
            ks = keystream[i] if keystream else 0
            out.append(self.byte(ks, 0) ^ pb)
        return bytes(out)

    def decrypt_bytes(self, enc: bytes) -> bytes:
        out = bytearray()
        for eb in enc:
            out.append(self.byte(0, 0) ^ eb)
        return bytes(out)

    def encrypt_reader_nonce(
        self, key: int, cuid: int, nt: bytes, nr: bytes
    ) -> bytes:
        nt_num = int.from_bytes(nt, "big")
        self.init(key)
        self.word(nt_num ^ cuid, 0)
        out = bytearray()
        for b in nr:
            enc = self.byte(b, 0) ^ b
            out.append(enc)
        nt_num = self.prng_successor(nt_num, 32)
        for _ in range(4):
            nt_num = self.prng_successor(nt_num, 8)
            enc = self.byte(0, 0) ^ (nt_num & 0xFF)
            out.append(enc)
        return bytes(out)


def iso14443_crc_a(data: bytes) -> int:
    crc = 0x6363
    for byte in data:
        byte ^= crc & 0xFF
        byte ^= (byte << 4) & 0xFF
        crc = (crc >> 8) ^ (byte << 8) ^ (byte << 3) ^ (byte >> 4)
        crc &= 0xFFFF
    return crc


def cuid_from_uid(uid: bytes) -> int:
    if len(uid) == 7:
        uid = uid[3:]
    return int.from_bytes(uid[:4], "big")
