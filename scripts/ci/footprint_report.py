#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Collect flash/RAM sizes from Twister zephyr.stat files."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def parse_stat(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        parts = line.split()
        if len(parts) >= 2:
            data[parts[0]] = parts[1]
    return data


def collect(twister_out: Path) -> list[dict]:
    report: list[dict] = []
    for stat in sorted(twister_out.rglob("zephyr.stat")):
        rel = stat.relative_to(twister_out)
        parts = rel.parts
        test_id = parts[0] if parts else str(rel)
        report.append(
            {
                "test_id": test_id,
                "platform": parts[1] if len(parts) > 1 else "",
                "stat_file": str(rel),
                **parse_stat(stat),
            }
        )
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract footprint data from twister-out/")
    parser.add_argument(
        "twister_out",
        nargs="?",
        default="twister-out",
        help="Twister output directory (default: twister-out)",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="footprint.json",
        help="Output JSON path (default: footprint.json)",
    )
    args = parser.parse_args()

    twister_out = Path(args.twister_out)
    if not twister_out.is_dir():
        print(f"::error::Twister output directory not found: {twister_out}", file=sys.stderr)
        return 1

    report = collect(twister_out)
    if not report:
        print(f"::warning::No zephyr.stat files found under {twister_out}", file=sys.stderr)

    payload = json.dumps(report, indent=2)
    Path(args.output).write_text(payload, encoding="utf-8")
    print(payload)
    return 0


if __name__ == "__main__":
    sys.exit(main())
