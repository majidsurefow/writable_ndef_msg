#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Doxygen Coverage Diff Tool

Compares Doxygen documentation coverage between two coverxygen JSON reports (reference and
comparison branches) and identifies newly introduced undocumented API symbols.
"""

import argparse
import json
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Symbol:
    file: str
    line: int
    name: str
    kind: str
    documented: bool


def extract_symbols(data: dict, strip_prefix: str = None) -> list[Symbol]:
    symbols = []
    for path, sym_list in data.get("files", {}).items():
        if strip_prefix and path.startswith(strip_prefix):
            path = path.removeprefix(strip_prefix).lstrip("/")
        if not isinstance(sym_list, list):
            continue
        symbols.extend(
            Symbol(
                path,
                s.get("line", 0),
                s.get("symbol", ""),
                s.get("kind", ""),
                s.get("documented", False),
            )
            for s in sym_list
        )
    return symbols


def find_undocumented_new(ref: list[Symbol], comp: list[Symbol]) -> list[Symbol]:
    ref_undoc = Counter((s.name, s.kind) for s in ref if not s.documented)

    result = []
    for s in comp:
        if s.documented:
            continue
        key = (s.name, s.kind)
        if ref_undoc[key] > 0:
            ref_undoc[key] -= 1
        else:
            result.append(s)

    return sorted(result, key=lambda s: (s.file, s.line))


def split_by_warn_paths(
    symbols: list[Symbol], warn_paths: list[str]
) -> tuple[list[Symbol], list[Symbol]]:
    if not warn_paths:
        return symbols, []

    errors = []
    warnings = []
    for sym in symbols:
        if any(sym.file.startswith(prefix) for prefix in warn_paths):
            warnings.append(sym)
        else:
            errors.append(sym)
    return errors, warnings


def print_github_annotations(
    symbols: list[Symbol], level: str = "error", max_annotations: int = 50
) -> None:
    for sym in symbols[:max_annotations]:
        print(
            f"::{level} file={sym.file},line={sym.line},"
            f"title=Missing Doxygen documentation::{sym.kind} '{sym.name}'"
            " is missing Doxygen comments."
        )

    if len(symbols) > max_annotations:
        print(
            f"::warning::... and {len(symbols) - max_annotations} more undocumented symbols "
            f"(showing first {max_annotations})"
        )


def _write_symbol_table(output_file, symbols: list[Symbol]) -> None:
    output_file.write("| File | Line | Kind | Symbol |\n|------|------|------|--------|\n")
    for sym in symbols:
        output_file.write(f"| `{sym.file}` | {sym.line} | {sym.kind} | `{sym.name}` |\n")


def write_summary(errors: list[Symbol], warnings: list[Symbol], summary_file: str) -> None:
    try:
        with open(summary_file, "a", encoding="utf-8") as f:
            f.write("\n## Doxygen Coverage Check Results\n\n")

            if not errors and not warnings:
                f.write("All new API symbols are properly documented.\n")
                return

            if errors:
                f.write(f"**{len(errors)} new API symbol(s) with missing documentation.**\n\n")
                _write_symbol_table(f, errors)
                f.write("\n")

            if warnings:
                f.write(
                    f"**{len(warnings)} new API symbol(s) with missing documentation "
                    "(warning-only paths).**\n\n"
                )
                _write_symbol_table(f, warnings)
                f.write("\n")

            f.write(
                "\nAdd Doxygen `@brief` comments to the listed symbols, "
                "or exclude internal symbols from the public headers.\n"
            )
    except OSError as e:
        print(f"::warning::Failed to write summary file '{summary_file}': {e}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compare Doxygen coverage between base and PR branches",
        allow_abbrev=False,
    )
    parser.add_argument("--reference", required=True, help="Path to reference coverage JSON")
    parser.add_argument("--comparison", required=True, help="Path to comparison coverage JSON")
    parser.add_argument(
        "--summary-file", help="Path to write markdown summary (e.g., $GITHUB_STEP_SUMMARY)"
    )
    parser.add_argument(
        "--strip-reference-prefix", help="Path prefix to strip from reference file paths"
    )
    parser.add_argument(
        "--strip-comparison-prefix", help="Path prefix to strip from comparison file paths"
    )
    parser.add_argument(
        "--warn-paths",
        nargs="*",
        default=[],
        help="Path prefixes where undocumented symbols are treated as warnings",
    )
    args = parser.parse_args()

    for name, path in [("Reference", args.reference), ("Comparison", args.comparison)]:
        if not Path(path).is_file():
            print(f"::error::{name} coverage file not found: {path}")
            return 1

    try:
        ref_data = json.loads(Path(args.reference).read_text(encoding="utf-8"))
        comp_data = json.loads(Path(args.comparison).read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        print(f"::error::Failed to parse coverage JSON: {e}")
        return 1

    ref_symbols = extract_symbols(ref_data, args.strip_reference_prefix)
    comp_symbols = extract_symbols(comp_data, args.strip_comparison_prefix)

    print(f"Reference: {len(ref_symbols)} symbols")
    print(f"Comparison: {len(comp_symbols)} symbols")

    undocumented_new = find_undocumented_new(ref_symbols, comp_symbols)
    print(f"New undocumented symbols: {len(undocumented_new)}")

    errors, warnings = split_by_warn_paths(undocumented_new, args.warn_paths)
    print(f" Errors: {len(errors)}, Warnings: {len(warnings)}")

    if errors:
        print_github_annotations(errors, level="error")
    if warnings:
        print_github_annotations(warnings, level="warning")

    if args.summary_file:
        write_summary(errors, warnings, args.summary_file)

    if errors:
        print(f"\nFound {len(errors)} new API symbol(s) without documentation (errors).")
        if warnings:
            print(f"Additionally, {len(warnings)} symbol(s) in warning-only paths.")
        return 1

    if warnings:
        print(f"\nFound {len(warnings)} new API symbol(s) without documentation (warnings only).")

    print("\nAll new API symbols are properly documented (or in warning-only paths).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
