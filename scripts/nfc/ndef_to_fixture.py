#!/usr/bin/env python3
"""Generate NDEF CI fixtures: Tier A .bin and Tier E .card.bin.

NDEF has no Flipper protocols/ndef/ folder — goldens come from cookbook §5.1,
NXP RW_NDEF_T4T, and tests/fixtures/ndef/ (not flipper_nfc_to_fixture.py).

Usage:
  python3 scripts/nfc/ndef_to_fixture.py
  python3 scripts/nfc/ndef_to_fixture.py --variant uri_5byte
  python3 scripts/nfc/ndef_to_fixture.py --variant all

Outputs (variant empty):
  tests/fixtures/ndef/empty.bin
  tests/fixtures/ndef/empty.inc
  tests/fixtures/store/ndef_empty.card.bin
  tests/fixtures/store/ndef_empty_card.inc
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from ndef_to_card_bin import build_card_envelope

REPO_ROOT = Path(__file__).resolve().parents[2]
NDEF_FIXTURE_DIR = REPO_ROOT / "tests" / "fixtures" / "ndef"
STORE_FIXTURE_DIR = REPO_ROOT / "tests" / "fixtures" / "store"

NDEF_CC_LEN = 15
NDEF_FORMAT_VERSION = 0x01

URI_PAYLOAD = bytes([0xD1, 0x01, 0x03, 0x55, 0x01])
CHUNK_NLEN = 300  # 0x012C — matches ndef_fixture.h NDEF_FIXTURE_CHUNK_NLEN


def build_model(ndef_file: bytes) -> bytes:
    if len(ndef_file) < 2:
        raise ValueError("ndef_file must include NLEN field")
    return bytes([NDEF_FORMAT_VERSION]) + bytes(NDEF_CC_LEN) + ndef_file


def build_chunk_payload() -> bytes:
    return bytes(i & 0xFF for i in range(CHUNK_NLEN))


VARIANTS: dict[str, dict[str, object]] = {
    "empty": {
        "model_bin": "empty.bin",
        "model_inc": "empty.inc",
        "model_comment": "Golden empty NDEF blob: format v1, zero CC, NLEN=0 (18 bytes).",
        "card_bin": "ndef_empty.card.bin",
        "card_inc": "ndef_empty_card.inc",
        "card_comment": "Tier E golden: nfc_store envelope for empty NDEF.",
        "build_model": lambda: build_model(bytes([0x00, 0x00])),
    },
    "uri_5byte": {
        "model_bin": "uri_5byte.bin",
        "model_inc": "uri_5byte.inc",
        "model_comment": "Golden URI NDEF blob: format v1, zero CC, NLEN=5 (23 bytes).",
        "card_bin": "ndef_uri_5byte.card.bin",
        "card_inc": "ndef_uri_5byte_card.inc",
        "card_comment": "Tier E golden: nfc_store envelope for URI NDEF (5-byte payload).",
        "build_model": lambda: build_model(bytes([0x00, len(URI_PAYLOAD)]) + URI_PAYLOAD),
    },
    "chunk_255": {
        "model_bin": "chunk_255.bin",
        "model_inc": "chunk_255.inc",
        "model_comment": "Golden chunked NDEF blob: format v1, zero CC, NLEN=300 (318 bytes).",
        "card_bin": "ndef_chunk_255.card.bin",
        "card_inc": "ndef_chunk_255_card.inc",
        "card_comment": "Tier E golden: nfc_store envelope for chunked NDEF (NLEN=300).",
        "build_model": lambda: build_model(bytes([0x01, 0x2C]) + build_chunk_payload()),
    },
}


def bytes_to_inc(data: bytes, comment: str) -> str:
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


def emit_variant(name: str) -> None:
    spec = VARIANTS[name]
    build_fn = spec["build_model"]
    assert callable(build_fn)
    model: bytes = build_fn()

    NDEF_FIXTURE_DIR.mkdir(parents=True, exist_ok=True)
    STORE_FIXTURE_DIR.mkdir(parents=True, exist_ok=True)

    model_bin = NDEF_FIXTURE_DIR / str(spec["model_bin"])
    model_inc = NDEF_FIXTURE_DIR / str(spec["model_inc"])
    card_bin = STORE_FIXTURE_DIR / str(spec["card_bin"])
    card_inc = STORE_FIXTURE_DIR / str(spec["card_inc"])

    model_bin.write_bytes(model)
    model_inc.write_text(bytes_to_inc(model, str(spec["model_comment"])), encoding="utf-8")

    card = build_card_envelope(model)
    card_bin.write_bytes(card)
    card_inc.write_text(bytes_to_inc(card, str(spec["card_comment"])), encoding="utf-8")

    print(f"Wrote {model_bin} ({len(model)} bytes)", file=sys.stderr)
    print(f"Wrote {model_inc}", file=sys.stderr)
    print(f"Wrote {card_bin} ({len(card)} bytes)", file=sys.stderr)
    print(f"Wrote {card_inc}", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description="Regenerate NDEF Tier A + Tier E fixtures.")
    parser.add_argument(
        "--variant",
        default="empty",
        choices=["empty", "uri_5byte", "chunk_255", "all"],
        help="Fixture variant (default empty; all regenerates every variant)",
    )
    args = parser.parse_args()

    if args.variant == "all":
        for name in ("empty", "uri_5byte", "chunk_255"):
            emit_variant(name)
    else:
        emit_variant(args.variant)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
