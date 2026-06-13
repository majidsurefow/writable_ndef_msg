#!/usr/bin/env python3
"""Convert Flipper .nfc (FlipperFormat) dumps to static test fixtures.

Offline tool — not invoked from CI. Twister links derived *.inc and *.bin only.

Usage:
  python3 scripts/nfc/flipper_nfc_to_fixture.py --help
  python3 scripts/nfc/flipper_nfc_to_fixture.py \\
    --input tests/fixtures/nfc/flipper/Ultralight_11.nfc \\
    --out-dir tests/fixtures/ultralight/

Outputs (when fully implemented):
  <stem>.bin   — Tier A model golden (serialize round-trip)
  <stem>.inc   — Tier B nfc_session_mock TX/RX script
  (optional) <stem>_listener.inc — Tier C APDU sequences

Stub today: writes <stem>.flipper_meta.txt summary only.

See tests/fixtures/nfc/flipper/README.md and docs/nfc/NFC_PROTOCOLS_COOKBOOK.md §14.11.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


def parse_flipper_nfc(path: Path) -> dict[str, str | list[str]]:
    """Parse minimal FlipperFormat key/value lines (stub)."""
    text = path.read_text(encoding="utf-8", errors="replace").splitlines()
    meta: dict[str, str | list[str]] = {}
    for line in text:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, _, value = line.partition(":")
        key = key.strip()
        value = value.strip()
        if key.startswith("Page "):
            pages = meta.setdefault("pages", [])
            assert isinstance(pages, list)
            pages.append(f"{key}: {value}")
        else:
            meta[key] = value
    return meta


def emit_fixtures(meta: dict[str, str | list[str]], out_dir: Path, stem: str) -> None:
    """Write placeholder outputs until full protocol mappers land."""
    out_dir.mkdir(parents=True, exist_ok=True)
    summary = out_dir / f"{stem}.flipper_meta.txt"
    lines = [f"{k}: {v}" if not isinstance(v, list) else f"{k}: ({len(v)} entries)" for k, v in meta.items()]
    summary.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote stub summary: {summary}", file=sys.stderr)
    print(
        "Full .inc/.bin generation not implemented yet — extend this script per protocol.",
        file=sys.stderr,
    )


def main() -> int:
    epilog = """
Examples:
  python3 scripts/nfc/flipper_nfc_to_fixture.py --help
  python3 scripts/nfc/flipper_nfc_to_fixture.py \\
    --input tests/fixtures/nfc/flipper/Ultralight_11.nfc \\
    --out-dir tests/fixtures/ultralight/

Outputs (when fully implemented):
  <stem>.bin   — Tier A model golden
  <stem>.inc   — Tier B nfc_session_mock script
  (optional) <stem>_listener.inc — Tier C APDU sequences

Stub today: writes <stem>.flipper_meta.txt only.
See tests/fixtures/nfc/flipper/README.md and docs/nfc/NFC_PROTOCOLS_COOKBOOK.md §14.11.
"""
    parser = argparse.ArgumentParser(
        description="Convert Flipper .nfc dumps to static test fixtures (offline; not CI).",
        epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--input", type=Path, required=True, help="Path to .nfc file")
    parser.add_argument(
        "--out-dir",
        type=Path,
        required=True,
        help="Directory for derived fixtures (e.g. tests/fixtures/ultralight)",
    )
    args = parser.parse_args()
    if not args.input.is_file():
        print(f"error: not a file: {args.input}", file=sys.stderr)
        return 1
    meta = parse_flipper_nfc(args.input)
    emit_fixtures(meta, args.out_dir, args.input.stem)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
