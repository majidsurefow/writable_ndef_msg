#!/usr/bin/env python3
"""Generate NDEF CI fixtures: Tier A .bin and Tier E .card.bin.

NDEF has no Flipper protocols/ndef/ folder — goldens come from cookbook §5.1,
NXP RW_NDEF_T4T, and tests/fixtures/ndef/ (not flipper_nfc_to_fixture.py).

Usage:
  python3 scripts/nfc/ndef_to_fixture.py
  python3 scripts/nfc/ndef_to_fixture.py --variant empty

Outputs (default variant empty):
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

# Empty NDEF model: format v1, zero CC, NLEN=0 (18 bytes) — cookbook §5.1 / Tier A golden.
EMPTY_MODEL = bytes([
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
])


def bytes_to_inc(data: bytes, comment: str) -> str:
    lines = [f"/* {comment} */"]
    row: list[str] = []
    for i, b in enumerate(data):
        row.append(f"0x{b:02X}U")
        if len(row) == 8:
            lines.append(", ".join(row) + ",")
            row = []
    if row:
        lines.append(", ".join(row) + ",")
    return "\n".join(lines) + "\n"


def emit_empty_fixtures() -> None:
    NDEF_FIXTURE_DIR.mkdir(parents=True, exist_ok=True)
    STORE_FIXTURE_DIR.mkdir(parents=True, exist_ok=True)

    (NDEF_FIXTURE_DIR / "empty.bin").write_bytes(EMPTY_MODEL)
    (NDEF_FIXTURE_DIR / "empty.inc").write_text(
        bytes_to_inc(EMPTY_MODEL, "Golden empty NDEF blob: format v1, zero CC, NLEN=0 (18 bytes)."),
        encoding="utf-8",
    )

    card = build_card_envelope(EMPTY_MODEL)
    (STORE_FIXTURE_DIR / "ndef_empty.card.bin").write_bytes(card)
    (STORE_FIXTURE_DIR / "ndef_empty_card.inc").write_text(
        bytes_to_inc(card, "Tier E golden: nfc_store envelope for empty NDEF (30 bytes)."),
        encoding="utf-8",
    )

    print(f"Wrote {NDEF_FIXTURE_DIR / 'empty.bin'} ({len(EMPTY_MODEL)} bytes)", file=sys.stderr)
    print(f"Wrote {NDEF_FIXTURE_DIR / 'empty.inc'}", file=sys.stderr)
    print(f"Wrote {STORE_FIXTURE_DIR / 'ndef_empty.card.bin'} ({len(card)} bytes)", file=sys.stderr)
    print(f"Wrote {STORE_FIXTURE_DIR / 'ndef_empty_card.inc'}", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description="Regenerate NDEF Tier A + Tier E fixtures.")
    parser.add_argument("--variant", default="empty", choices=["empty"],
                        help="Fixture variant (only empty today)")
    args = parser.parse_args()

    if args.variant != "empty":
        print(f"error: unsupported variant: {args.variant}", file=sys.stderr)
        return 1

    emit_empty_fixtures()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
