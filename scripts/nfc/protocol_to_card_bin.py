#!/usr/bin/env python3
"""Build Tier E .card.bin goldens for desfire / emv / aliro protocol mocks.

Usage (repo root):
  python3 scripts/nfc/protocol_to_card_bin.py --all
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from ndef_to_card_bin import build_card_envelope, crc16_ccitt  # noqa: E402
from nfc_persist_ids import (  # noqa: E402
    NFC_PERSIST_ID_ALIRO,
    NFC_PERSIST_ID_DESFIRE,
    NFC_PERSIST_ID_EMV,
)

STORE_DIR = Path(__file__).resolve().parents[2] / "tests" / "fixtures" / "store"

DESFIRE_MODEL = bytes([
    0x01, 0x04, 0x01, 0x01, 0x00, 0x18, 0x05, 0x01, 0x04, 0x01, 0x01, 0x00,
    0x18, 0x05, 0x01, 0x04, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x24, 0x60, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0B, 0x01, 0x01, 0x02, 0x03, 0x0B, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x0E, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x48, 0x45, 0x4C, 0x4C,
    0x4F, 0x20, 0x44, 0x45, 0x53, 0x46, 0x49, 0x52, 0x45, 0x21, 0x21,
])

EMV_SERVICE_APP_AID = bytes([0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10])
EMV_PAN = bytes([0x99] * 8)
EMV_EXPIRY = bytes([0x99, 0x12, 0x31])
EMV_NAME = b"TEST/CARD" + (b" " * 17)
EMV_TRACK2 = bytes([
    0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99,
    0xD9, 0x12, 0x31, 0x00, 0x00, 0x00, 0x00, 0x0F,
])


def model_bin_to_inc(data: bytes, comment: str) -> str:
    lines = [f"/* {comment} */"]
    row: list[str] = []
    for b in data:
        row.append(f"0x{b:02X}U")
        if len(row) == 8:
            lines.append(", ".join(row) + ",")
            row = []
    if row:
        lines.append(", ".join(row) + ",")
    return "\n".join(lines) + "\n"


def emv_build_record0(track2_len: int, track2: bytes) -> bytes:
    """Mirror emv_build_record0() + emv_tlv_close() in emv.c."""
    record = bytearray([0x70, 0x00])
    start = len(record)
    record.append(0x57)
    record.append(track2_len)
    record.extend(track2[:track2_len])
    record[start] = len(record) - start - 1
    return bytes(record)


def build_emv_default_model() -> bytes:
    track2_len = len(EMV_TRACK2)
    record0 = emv_build_record0(track2_len, EMV_TRACK2)
    track2_padded = EMV_TRACK2 + bytes(19 - len(EMV_TRACK2))
    app_aid = EMV_SERVICE_APP_AID + bytes(16 - len(EMV_SERVICE_APP_AID))
    body = bytearray()
    body.append(0x01)
    body.append(16)
    body.extend(EMV_PAN)
    body.extend(EMV_EXPIRY)
    body.extend(EMV_NAME)
    body.append(track2_len)
    body.extend(track2_padded)
    body.extend(bytes([0x00, 0x00]))
    body.extend(bytes([0x08, 0x01, 0x01, 0x00]))
    body.append(len(EMV_SERVICE_APP_AID))
    body.extend(app_aid)
    body.append(1)
    body.append(len(record0))
    body.extend(record0)
    return bytes(body)


def build_aliro_default_model() -> bytes:
    body = bytearray()
    body.append(0x01)
    body.extend(struct.pack("<H", 0))
    body.append(0)
    body.append(9)
    pubkey = bytearray(65)
    pubkey[0] = 0x04
    body.extend(pubkey)
    transcript = bytes([0xAA, 0xBB])
    body.extend(struct.pack("<H", len(transcript)))
    body.extend(transcript)
    return bytes(body)


def emit_fixture(stem: str, model: bytes, persist_id: int, comment: str) -> None:
    card = build_card_envelope(model, persist_id=persist_id, reader_capture=False)
    card_bin = STORE_DIR / f"{stem}.card.bin"
    card_inc = STORE_DIR / f"{stem}_card.inc"
    card_bin.write_bytes(card)
    card_inc.write_text(model_bin_to_inc(card, comment), encoding="utf-8")
    print(f"Wrote {card_bin} ({len(card)} bytes)", file=sys.stderr)
    print(f"Wrote {card_inc}", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description="Regenerate desfire/emv/aliro store goldens.")
    parser.add_argument("--all", action="store_true", help="Emit all protocol card goldens")
    parser.add_argument("--desfire", action="store_true")
    parser.add_argument("--emv", action="store_true")
    parser.add_argument("--aliro", action="store_true")
    args = parser.parse_args()

    if not (args.all or args.desfire or args.emv or args.aliro):
        args.all = True

    STORE_DIR.mkdir(parents=True, exist_ok=True)

    if args.all or args.desfire:
        emit_fixture("Desfire", DESFIRE_MODEL, NFC_PERSIST_ID_DESFIRE,
                     "Tier E golden: nfc_store envelope for DESFire mock.")
    if args.all or args.emv:
        emit_fixture("Emv", build_emv_default_model(), NFC_PERSIST_ID_EMV,
                     "Tier E golden: nfc_store envelope for EMV default card.")
    if args.all or args.aliro:
        emit_fixture("Aliro", build_aliro_default_model(), NFC_PERSIST_ID_ALIRO,
                     "Tier E golden: nfc_store envelope for Aliro mock.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
