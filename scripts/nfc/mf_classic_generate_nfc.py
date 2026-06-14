#!/usr/bin/env python3
"""Generate deterministic Flipper-format MIFARE Classic .nfc fixtures.

Mirrors flipperzero/lib/nfc/helpers/nfc_data_generator.c layout with fixed UIDs
for reproducible Tier A/B/C/E+ tests (1K/4K/Mini × 4b/7b UID where applicable).
"""

from __future__ import annotations

import argparse
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT_DIR = REPO_ROOT / "tests" / "fixtures" / "nfc" / "flipper"

VARIANTS: dict[str, dict] = {
    "1K_4b": {
        "uid": bytes([0x04, 0xDE, 0xAD, 0xBE]),
        "atqa": (0x00, 0x04),
        "sak": 0x08,
        "type_name": "1K",
        "blocks": 64,
    },
    "1K_7b": {
        "uid": bytes([0x04, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE]),
        "atqa": (0x44, 0x00),
        "sak": 0x08,
        "type_name": "1K",
        "blocks": 64,
    },
    "4K_4b": {
        "uid": bytes([0x04, 0x4B, 0x4B, 0x4B]),
        "atqa": (0x02, 0x00),
        "sak": 0x18,
        "type_name": "4K",
        "blocks": 256,
    },
    "Mini_4b": {
        "uid": bytes([0x04, 0xDE, 0xAD, 0xCA]),
        "atqa": (0x08, 0x00),
        "sak": 0x09,
        "type_name": "MINI",
        "blocks": 20,
    },
}


def format_block_hex(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)


def sector_trailer_block(sector: int) -> int:
    return sector * 4 + 3


def sector_trailer_bytes() -> bytes:
    return bytes([
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0x07, 0x80, 0x69,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    ])


def uid_bcc(uid_part: bytes) -> int:
    bcc = 0
    for b in uid_part:
        bcc ^= b
    return bcc


def block0_bytes(uid: bytes, sak: int, atqa: tuple[int, int]) -> bytes:
    blk = bytearray(16)
    if len(uid) == 7:
        blk[0] = 0x88
        blk[1:5] = uid[:4]
        blk[5] = uid_bcc(uid[:4])
        blk[6] = sak
        blk[7] = atqa[0]
        blk[8] = atqa[1]
        for i in range(9, 16):
            blk[i] = 0xFF
    else:
        blk[0:4] = uid
        blk[5] = sak
        blk[6] = atqa[0]
        blk[7] = atqa[1]
        for i in range(8, 16):
            blk[i] = 0xFF
    return bytes(blk)


def block1_bytes(uid: bytes) -> bytes | None:
    if len(uid) != 7:
        return None
    blk = bytearray(16)
    blk[0:3] = uid[4:7]
    blk[3] = uid_bcc(uid[4:7])
    for i in range(4, 16):
        blk[i] = 0xFF
    return bytes(blk)


def build_nfc_text(variant: dict) -> str:
    uid = variant["uid"]
    atqa = variant["atqa"]
    sak = variant["sak"]
    type_name = variant["type_name"]
    blocks_total = variant["blocks"]

    lines = [
        "Filetype: Flipper NFC device",
        "Version: 3",
        "Device type: Mifare Classic",
        "UID: " + format_block_hex(uid),
        f"ATQA: {atqa[0]:02X} {atqa[1]:02X}",
        f"SAK: {sak:02X}",
        "# Mifare Classic specific data",
        f"Mifare Classic type: {type_name}",
        "Data format version: 2",
        "# Mifare Classic blocks, '??' means unknown data",
        f"Block 0: {format_block_hex(block0_bytes(uid, sak, atqa))}",
    ]

    block1 = block1_bytes(uid)
    if block1 is not None:
        lines.append(f"Block 1: {format_block_hex(block1)}")

    start = 2 if block1 is not None else 1
    for block in range(start, blocks_total):
        if block == sector_trailer_block(block // 4):
            data = sector_trailer_bytes()
        else:
            data = bytes(16)
        lines.append(f"Block {block}: {format_block_hex(data)}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate deterministic MIFARE Classic .nfc")
    parser.add_argument(
        "--variant",
        choices=["all", *VARIANTS.keys()],
        default="all",
        help="Which variant to emit (default: all)",
    )
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR)
    args = parser.parse_args()

    keys = VARIANTS.keys() if args.variant == "all" else [args.variant]
    args.out_dir.mkdir(parents=True, exist_ok=True)

    for key in keys:
        out_path = args.out_dir / f"MfClassic_{key}.nfc"
        out_path.write_text(build_nfc_text(VARIANTS[key]), encoding="utf-8")
        print(f"Wrote {out_path}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
