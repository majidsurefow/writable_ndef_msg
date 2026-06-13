#!/usr/bin/env python3
"""Build nfc_store .card.bin envelope from an NDEF Tier A model blob.

Offline tool — mirrors nfc_store.c header + TLV entry + CRC16-CCITT layout.
Not invoked from CI; Twister links committed tests/fixtures/store/*.card.bin.

Usage:
  python3 scripts/nfc/ndef_to_card_bin.py --model tests/fixtures/ndef/empty.bin \\
    --output tests/fixtures/store/ndef_empty.card.bin
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

NFC_STORE_BLOB_MAGIC_0 = 0x4E
NFC_STORE_BLOB_MAGIC_1 = 0x46
NFC_STORE_BLOB_VERSION = 0x02
NFC_STORE_BLOB_HDR_SIZE = 6
NFC_STORE_ENTRY_OVERHEAD = 4
NFC_PERSIST_ID_NDEF = 0x01
NFC_STORE_ENTRY_FLAG_READER_CAPTURED = 1 << 0
NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE = 1 << 2


def crc16_ccitt(data: bytes, init: int = 0xFFFF) -> int:
    """Zephyr crc16_ccitt(seed, buf) — zephyr/subsys/crc/crc16_sw.c."""
    seed = init
    for byte in data:
        e = (seed ^ byte) & 0xFF
        f = e ^ ((e << 4) & 0xFF)
        seed = ((seed >> 8) ^ (f << 8) ^ (f << 3) ^ (f >> 4)) & 0xFFFF
    return seed


def build_card_envelope(model_body: bytes, persist_id: int = NFC_PERSIST_ID_NDEF,
                        reader_capture: bool = True) -> bytes:
    flags = NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE
    if reader_capture:
        flags |= NFC_STORE_ENTRY_FLAG_READER_CAPTURED
    else:
        flags |= 1 << 1  # NFC_STORE_ENTRY_FLAG_HAND_AUTHORED

    entry_len = NFC_STORE_ENTRY_OVERHEAD + len(model_body)
    payload_len = entry_len

    buf = bytearray()
    buf.extend([NFC_STORE_BLOB_MAGIC_0, NFC_STORE_BLOB_MAGIC_1, NFC_STORE_BLOB_VERSION, 0x00])
    buf.extend(struct.pack("<H", payload_len))
    buf.extend([persist_id, flags])
    buf.extend(struct.pack("<H", len(model_body)))
    buf.extend(model_body)

    crc = crc16_ccitt(buf)
    buf.extend(struct.pack("<H", crc))
    return bytes(buf)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Wrap NDEF model blob in nfc_store .card.bin TLV envelope.",
    )
    parser.add_argument("--model", type=Path, required=True,
                        help="Tier A model binary (e.g. tests/fixtures/ndef/empty.bin)")
    parser.add_argument("--output", type=Path, required=True,
                        help="Output .card.bin path")
    parser.add_argument("--persist-id", type=lambda x: int(x, 0), default=NFC_PERSIST_ID_NDEF,
                        help="NFC_PERSIST_ID_* value (default 0x01 NDEF)")
    args = parser.parse_args()

    if not args.model.is_file():
        print(f"error: not a file: {args.model}", file=sys.stderr)
        return 1

    model_body = args.model.read_bytes()
    if len(model_body) == 0:
        print("error: empty model blob", file=sys.stderr)
        return 1

    envelope = build_card_envelope(model_body, persist_id=args.persist_id)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(envelope)
    print(f"Wrote {args.output} ({len(envelope)} bytes)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
