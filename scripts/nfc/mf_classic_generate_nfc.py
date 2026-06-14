#!/usr/bin/env python3
"""Generate deterministic Flipper-format MfClassic_1K_4b.nfc (generator golden).

Mirrors flipperzero/lib/nfc/helpers/nfc_data_generator.c nfc_generate_mf_classic_1k_4b_uid
with fixed UID 04 DE AD BE for reproducible Tier A/B fixtures.
"""

from __future__ import annotations

import argparse
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = REPO_ROOT / "tests" / "fixtures" / "nfc" / "flipper" / "MfClassic_1K_4b.nfc"

UID = bytes([0x04, 0xDE, 0xAD, 0xBE])
ATQA = (0x00, 0x04)
SAK = 0x08
BLOCKS_1K = 64
SECTORS_1K = 16


def sector_trailer_block(sector: int) -> int:
    return sector * 4 + 3


def format_block_hex(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)


def block0_bytes() -> bytes:
    blk = bytearray(16)
    blk[0:4] = UID
    blk[5] = SAK
    blk[6] = ATQA[0]
    blk[7] = ATQA[1]
    for i in range(8, 16):
        blk[i] = 0xFF
    return bytes(blk)


def sector_trailer_bytes() -> bytes:
    return bytes([
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x07, 0x80, 0x69,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    ])


def build_nfc_text() -> str:
    lines = [
        "Filetype: Flipper NFC device",
        "Version: 3",
        "Device type: Mifare Classic",
        "UID: " + format_block_hex(UID),
        f"ATQA: {ATQA[0]:02X} {ATQA[1]:02X}",
        f"SAK: {SAK:02X}",
        "# Mifare Classic specific data",
        "Mifare Classic type: 1K",
        "Data format version: 2",
        "# Mifare Classic blocks, '??' means unknown data",
        f"Block 0: {format_block_hex(block0_bytes())}",
    ]
    for block in range(1, BLOCKS_1K):
        if block == sector_trailer_block(block // 4):
            data = sector_trailer_bytes()
        else:
            data = bytes(16)
        lines.append(f"Block {block}: {format_block_hex(data)}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate deterministic MfClassic_1K_4b.nfc")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(build_nfc_text(), encoding="utf-8")
    print(f"Wrote {args.out}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
