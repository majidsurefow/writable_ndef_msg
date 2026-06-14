#!/usr/bin/env python3
"""Convert Flipper .nfc (FlipperFormat) dumps to static test fixtures.

Offline tool — not invoked from CI. Twister links derived *.inc and *.bin only.

Usage:
  python3 scripts/nfc/flipper_nfc_to_fixture.py --help
  python3 scripts/nfc/flipper_nfc_to_fixture.py --all
  python3 scripts/nfc/flipper_nfc_to_fixture.py \\
    --input tests/fixtures/nfc/flipper/Ultralight_11.nfc \\
    --out-dir tests/fixtures/ultralight/

Outputs:
  <stem>_model.bin   — Tier A model golden (product serialize layout)
  <stem>_read.inc    — Tier B nfc_session_mock RX script (one STEP per poller TX)
  Optional tests/fixtures/store/<stem>.card.bin via --card-bin

HAL nfca_signal_* fixtures: timing + plain data only — no .card.bin (§14.11).

See tests/fixtures/nfc/flipper/README.md and docs/nfc/NFC_PROTOCOLS_COOKBOOK.md §14.11.
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

from nfc_persist_ids import (
    NFC_PERSIST_ID_CLASSIC,
    NFC_PERSIST_ID_FELICA,
    NFC_PERSIST_ID_SLIX,
    NFC_PERSIST_ID_ULTRALIGHT,
    NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
    NFC_STORE_ENTRY_FLAG_HAND_AUTHORED,
    PERSIST_HAND_FLAGS,
)
from mf_classic_crypto1 import Crypto1, cuid_from_uid, iso14443_crc_a
from ndef_to_card_bin import build_card_envelope

REPO_ROOT = Path(__file__).resolve().parents[2]
FLIPPER_DIR = REPO_ROOT / "tests" / "fixtures" / "nfc" / "flipper"
STORE_DIR = REPO_ROOT / "tests" / "fixtures" / "store"

UL_BLOB_FORMAT_VERSION = 0x01
UL_VARIANT_UL11 = 0x01
UL_VARIANT_UL21 = 0x02
UL_VARIANT_UNKNOWN = 0xFF

FELICA_BLOB_FORMAT_VERSION = 0x02
ISO15693_BLOB_FORMAT_VERSION = 0x01
SLIX_EXT_MAGIC = b"SL"
SLIX_EXT_VERSION = 0x01
HAL_SIGNAL_BLOB_VERSION = 0x01

CAP_DEFAULT = 0
CAP_ACCEPT_ALL = 1
CAP_MISSED = 2

PROTO_OUT_DIRS: dict[str, Path] = {
    "ultralight": REPO_ROOT / "tests" / "fixtures" / "ultralight",
    "felica": REPO_ROOT / "tests" / "fixtures" / "felica",
    "slix": REPO_ROOT / "tests" / "fixtures" / "slix",
    "hal": REPO_ROOT / "tests" / "fixtures" / "hal",
    "classic": REPO_ROOT / "tests" / "fixtures" / "classic",
}

CLASSIC_FORMAT_VERSION = 0x01
CLASSIC_TYPE_MINI = 0
CLASSIC_TYPE_1K = 1
CLASSIC_TYPE_4K = 2
CLASSIC_BLOCK_MASK_WORDS = 8
CLASSIC_DEFAULT_KEY = 0xFFFFFFFFFFFF
CLASSIC_TEST_NR = bytes([0x01, 0x02, 0x03, 0x04])


def parse_hex_bytes(value: str) -> bytes:
    parts = value.replace(",", " ").split()
    return bytes(int(p, 16) for p in parts)


def parse_flipper_nfc(path: Path) -> dict:
    text = path.read_text(encoding="utf-8", errors="replace").splitlines()
    meta: dict = {"_path": path}
    pages: list[tuple[int, bytes]] = []
    blocks: list[tuple[int, bytes]] = []

    for line in text:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, _, value = line.partition(":")
        key = key.strip()
        value = value.strip()

        page_m = re.fullmatch(r"Page (\d+)", key)
        if page_m:
            pages.append((int(page_m.group(1)), parse_hex_bytes(value)))
            continue
        block_m = re.fullmatch(r"Block (\d+)", key)
        if block_m:
            blocks.append((int(block_m.group(1)), parse_hex_bytes(value)))
            continue
        meta[key] = value

    if pages:
        meta["pages"] = dict(pages)
    if blocks:
        meta["blocks"] = dict(blocks)
    return meta


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


def read_inc_steps(steps: list[tuple[str, bytes]]) -> str:
    lines = ["/* Tier B nfc_session_mock RX script — one STEP per transceive */"]
    for idx, (label, payload) in enumerate(steps):
        lines.append(f"/* STEP {idx}: {label} */")
        lines.append(bytes_to_inc(payload, label).rstrip())
    return "\n".join(lines) + "\n"


def model_bin_to_inc(data: bytes) -> str:
    row: list[str] = []
    lines: list[str] = []
    for b in data:
        row.append(f"0x{b:02X}U")
        if len(row) == 8:
            lines.append(", ".join(row) + ",")
            row = []
    if row:
        lines.append(", ".join(row) + ",")
    return "\n".join(lines) + "\n"


def emit_ultralight_mock_header(stem: str, steps: list[tuple[str, bytes]], model: bytes,
                                 out_dir: Path) -> None:
    sym = stem.replace("-", "_")
    lines = [
        f"/* Auto-generated from Flipper {stem}.nfc — do not edit. */",
        "#ifndef ULTRALIGHT_FIXTURE_" + sym.upper() + "_H_",
        "#define ULTRALIGHT_FIXTURE_" + sym.upper() + "_H_",
        "",
        "#include \"nfc_session_mock.h\"",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include <zephyr/sys/util.h>",
        "",
        f"static const uint8_t ultralight_{sym}_model[] = {{",
        model_bin_to_inc(model).rstrip(),
        "};",
        "",
        f"#define ULTRALIGHT_{sym.upper()}_MODEL_LEN sizeof(ultralight_{sym}_model)",
        "",
    ]
    step_refs: list[str] = []
    for idx, (_label, payload) in enumerate(steps):
        arr = f"ultralight_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {arr}[] = {{")
        if payload:
            lines.append(model_bin_to_inc(payload).rstrip())
        lines.append("};")
        lines.append("")
        step_refs.append(
            f"\t{{ .rx = {arr}, .rx_len = sizeof({arr}), .err = 0 }},"
        )
    lines.append(f"static const nfc_session_mock_step_t ultralight_{sym}_read_steps[] = {{")
    lines.extend(step_refs)
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define ULTRALIGHT_{sym.upper()}_READ_STEP_COUNT "
        f"ARRAY_SIZE(ultralight_{sym}_read_steps)"
    )
    lines.append("")
    lines.append("#endif")
    lines.append("")
    out_path = out_dir / f"{stem}_mock.h"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out_path}", file=sys.stderr)


def ultralight_variant(meta: dict) -> int:
    device = meta.get("Device type", "")
    if "Ultralight 11" in device:
        return UL_VARIANT_UL11
    if "Ultralight 21" in device:
        return UL_VARIANT_UL21
    return UL_VARIANT_UNKNOWN


def build_ultralight_model(meta: dict) -> bytes:
    pages_map: dict[int, bytes] = meta.get("pages", {})
    pages_total = int(meta.get("Pages total", len(pages_map)))
    variant = ultralight_variant(meta)
    buf = bytearray()
    buf.append(UL_BLOB_FORMAT_VERSION)
    buf.append(variant)
    buf.extend(struct.pack("<H", pages_total))
    for page_num in range(pages_total):
        page = pages_map.get(page_num, b"\x00" * 4)
        if len(page) != 4:
            raise ValueError(f"page {page_num}: expected 4 bytes, got {len(page)}")
        buf.extend(page)

    version = parse_hex_bytes(meta.get("Mifare version", "")) if "Mifare version" in meta else b""
    has_version = 1 if len(version) == 8 and any(version) else 0
    buf.append(has_version)
    if has_version:
        buf.extend(version)

    signature = parse_hex_bytes(meta.get("Signature", "")) if "Signature" in meta else b""
    has_signature = 1 if len(signature) == 32 and any(signature) else 0
    buf.append(has_signature)
    if has_signature:
        buf.extend(signature)

    counters: list[int] = []
    tearings: list[int] = []
    for i in range(3):
        ck = f"Counter {i}"
        tk = f"Tearing {i}"
        if ck in meta:
            counters.append(int(meta[ck]))
        if tk in meta:
            tearings.append(int(meta[tk], 16))

    buf.append(len(counters))
    for counter in counters:
        buf.extend(struct.pack("<I", counter)[:3])
    buf.append(len(tearings))
    buf.extend(tearings)
    return bytes(buf)


def ultralight_locked_auth0(meta: dict) -> int | None:
    """Return AUTH0 page index when tag has password protection, else None."""
    pages_map: dict[int, bytes] = meta.get("pages", {})
    pages_total = int(meta.get("Pages total", len(pages_map)))
    if pages_total < 5:
        return None

    auth0_page = pages_total - 4
    access_page = pages_total - 3
    auth0_data = pages_map.get(auth0_page, b"\xff" * 4)
    access_data = pages_map.get(access_page, b"\x00" * 4)
    auth0 = auth0_data[3]
    prot = (access_data[0] & 0x01) != 0

    if auth0 == 0xFF:
        return None
    if auth0 >= pages_total:
        return None
    if prot or auth0 < pages_total:
        return auth0
    return None


def ultralight_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    pages_map: dict[int, bytes] = meta.get("pages", {})
    pages_total = int(meta.get("Pages total", len(pages_map)))
    steps: list[tuple[str, bytes]] = []

    version = parse_hex_bytes(meta.get("Mifare version", "")) if "Mifare version" in meta else b""
    signature = parse_hex_bytes(meta.get("Signature", "")) if "Signature" in meta else b""
    counters: list[int] = []
    tearings: list[int] = []
    for i in range(3):
        ck = f"Counter {i}"
        tk = f"Tearing {i}"
        if ck in meta:
            counters.append(int(meta[ck]))
        if tk in meta:
            tearings.append(int(meta[tk], 16))

    if len(version) == 8 and any(version):
        steps.append(("ultralight GET_VERSION", version))
    else:
        steps.append(("ultralight GET_VERSION fail", b""))
        page41 = b"".join(pages_map.get(p, b"\x00" * 4) for p in range(41, min(45, pages_total)))
        page41 = page41.ljust(16, b"\x00")
        steps.append(("ultralight READ page 41 probe", page41))
        if pages_total > 47:
            page47 = b"".join(pages_map.get(p, b"\x00" * 4) for p in range(47, min(51, pages_total)))
            page47 = page47.ljust(16, b"\x00")
            steps.append(("ultralight READ page 47 probe", page47))

    auth0 = ultralight_locked_auth0(meta)
    read_limit = pages_total
    if auth0 is not None:
        read_limit = auth0

    for start in range(0, read_limit, 4):
        chunk = b"".join(pages_map.get(p, b"\x00" * 4) for p in range(start, min(start + 4, pages_total)))
        if len(chunk) < 16:
            chunk = chunk.ljust(16, b"\x00")
        steps.append((f"ultralight READ page {start}", chunk))

    if auth0 is not None:
        return steps

    if len(version) == 8 and any(version):
        steps.append(("ultralight READ_SIG", signature.ljust(32, b"\x00")[:32]))
        for i in range(3):
            counter = counters[i] if i < len(counters) else 0
            steps.append((f"ultralight READ_CNT {i}", struct.pack("<I", counter)[:3]))
        for i in range(3):
            flag = tearings[i] if i < len(tearings) else 0
            steps.append((f"ultralight CHECK_TEARING {i}", bytes([flag])))

    return steps


def build_felica_model(meta: dict) -> bytes:
    uid = parse_hex_bytes(meta.get("Manufacture id", meta.get("UID", "")))
    pmm = parse_hex_bytes(meta.get("Manufacture parameter", ""))
    blocks_map: dict[int, bytes] = meta.get("blocks", {})
    blocks_total = int(meta.get("Blocks total", len(blocks_map)))
    blocks_read = int(meta.get("Blocks read", blocks_total))
    if len(uid) != 8 or len(pmm) != 8:
        raise ValueError("Felica: UID and PMm must be 8 bytes")

    buf = bytearray()
    buf.append(FELICA_BLOB_FORMAT_VERSION)
    buf.extend(uid)
    buf.extend(pmm)
    buf.extend(struct.pack("<HH", blocks_total, blocks_read))
    for block_num in range(blocks_total):
        raw = blocks_map.get(block_num, b"\x00" * 18)
        if len(raw) < 2:
            raw = b"\x00\x00" + raw
        sf1, sf2 = raw[0], raw[1]
        data = raw[2:18].ljust(16, b"\x00")
        buf.extend([sf1, sf2])
        buf.extend(data)
    return bytes(buf)


def felica_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    blocks_map: dict[int, bytes] = meta.get("blocks", {})
    blocks_total = int(meta.get("Blocks total", len(blocks_map)))
    steps: list[tuple[str, bytes]] = []
    uid = parse_hex_bytes(meta.get("Manufacture id", meta.get("UID", "")))
    pmm = parse_hex_bytes(meta.get("Manufacture parameter", ""))
    poll = uid[:8].ljust(8, b"\x00") + pmm[:8].ljust(8, b"\x00")
    steps.append(("felica poll response", poll))
    for block_num in range(blocks_total):
        raw = blocks_map.get(block_num, b"\x00" * 18)
        payload = raw[2:18].ljust(16, b"\x00") if len(raw) >= 2 else raw.ljust(16, b"\x00")
        steps.append((f"felica READ block {block_num}", payload))
    return steps


def build_iso15693_model(meta: dict) -> bytes:
    uid = parse_hex_bytes(meta.get("UID", ""))
    if len(uid) != 8:
        raise ValueError("ISO15693: UID must be 8 bytes")
    dsfid = int(meta.get("DSFID", "0"), 16) if "DSFID" in meta else 0
    afi = int(meta.get("AFI", "0"), 16) if "AFI" in meta else 0
    ic_ref = int(meta.get("IC Reference", "0"), 16) if "IC Reference" in meta else 0
    lock_dsfid = 1 if meta.get("Lock DSFID", "false").lower() == "true" else 0
    lock_afi = 1 if meta.get("Lock AFI", "false").lower() == "true" else 0
    block_count = int(meta.get("Block Count", "0"))
    block_size = int(meta.get("Block Size", "4"), 16)
    data = parse_hex_bytes(meta.get("Data Content", ""))
    security = parse_hex_bytes(meta.get("Security Status", ""))

    expected_data_len = block_count * block_size
    if len(data) < expected_data_len:
        data = data.ljust(expected_data_len, b"\x00")
    else:
        data = data[:expected_data_len]
    if len(security) < block_count:
        security = security.ljust(block_count, b"\x00")
    else:
        security = security[:block_count]

    buf = bytearray()
    buf.append(ISO15693_BLOB_FORMAT_VERSION)
    buf.extend(uid)
    buf.extend([dsfid, afi, ic_ref, lock_dsfid, lock_afi])
    buf.extend(struct.pack("<H", block_count))
    buf.append(block_size)
    buf.extend(data)
    buf.extend(security)
    return bytes(buf)


def slix_capabilities(meta: dict, stem: str) -> int:
    cap = meta.get("Capabilities", "Default")
    if cap == "AcceptAllPasswords" or "accept_all" in stem:
        return CAP_ACCEPT_ALL
    if cap == "Missed" or "missed" in stem:
        return CAP_MISSED
    return CAP_DEFAULT


def build_slix_model(meta: dict, stem: str) -> bytes:
    parent = build_iso15693_model(meta)
    buf = bytearray(parent)
    buf.extend(SLIX_EXT_MAGIC)
    buf.append(SLIX_EXT_VERSION)
    buf.append(slix_capabilities(meta, stem))
    for key in ("Password Read", "Password Write", "Password Privacy",
                "Password Destroy", "Password EAS"):
        pwd = parse_hex_bytes(meta.get(key, "00 00 00 00"))
        buf.extend(pwd.ljust(4, b"\x00")[:4])
    signature = parse_hex_bytes(meta.get("Signature", ""))
    buf.extend(signature.ljust(32, b"\x00")[:32])
    buf.append(1 if meta.get("Privacy Mode", "false").lower() == "true" else 0)
    buf.append(int(meta.get("Protection Pointer", "0"), 16) if "Protection Pointer" in meta else 0)
    buf.append(int(meta.get("Protection Condition", "0"), 16) if "Protection Condition" in meta else 0)
    buf.append(1 if meta.get("Lock EAS", "false").lower() == "true" else 0)
    buf.append(1 if meta.get("Lock PPL", "false").lower() == "true" else 0)
    return bytes(buf)


def iso15693_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    uid = parse_hex_bytes(meta.get("UID", ""))
    dsfid = int(meta.get("DSFID", "0"), 16) if "DSFID" in meta else 0
    block_count = int(meta.get("Block Count", "0"))
    block_size = int(meta.get("Block Size", "4"), 16)
    data = parse_hex_bytes(meta.get("Data Content", ""))
    steps = [("iso15693 inventory", bytes([0x00, dsfid]) + uid[::-1])]
    steps.append(("iso15693 GET_SYSTEM_INFO",
                  bytes([0x00]) + struct.pack("<H", block_count) + bytes([block_size])))
    for block_num in range(block_count):
        start = block_num * block_size
        payload = data[start:start + block_size].ljust(block_size, b"\x00")
        steps.append((f"iso15693 READ block {block_num}", bytes([0x00]) + payload))
    return steps


def slix_read_steps(meta: dict, stem: str) -> list[tuple[str, bytes]]:
    steps = iso15693_read_steps(meta)
    signature = parse_hex_bytes(meta.get("Signature", ""))
    steps.append(("slix GET_NXP_SYSTEM_INFO", bytes([0x00, 0x0F])))
    steps.append(("slix READ_SIGNATURE", signature.ljust(32, b"\x00")[:32]))
    cap = slix_capabilities(meta, stem)
    if cap == CAP_MISSED:
        steps.append(("slix CAP missed", b""))
    elif cap == CAP_ACCEPT_ALL:
        steps.append(("slix CAP accept all pass", b"\xAC"))
    else:
        steps.append(("slix CAP default", b"\x00"))
    return steps


def emit_protocol_mock_header(prefix: str, stem: str, steps: list[tuple[str, bytes]],
                              model: bytes, out_dir: Path) -> None:
    sym = stem.replace("-", "_")
    tag = prefix.upper()
    lines = [
        f"/* Auto-generated from Flipper {stem}.nfc — do not edit. */",
        f"#ifndef {tag}_FIXTURE_{sym.upper()}_H_",
        f"#define {tag}_FIXTURE_{sym.upper()}_H_",
        "",
        "#include \"nfc_session_mock.h\"",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include <zephyr/sys/util.h>",
        "",
        f"static const uint8_t {prefix}_{sym}_model[] = {{",
        model_bin_to_inc(model).rstrip(),
        "};",
        "",
        f"#define {tag}_{sym.upper()}_MODEL_LEN sizeof({prefix}_{sym}_model)",
        "",
    ]
    step_refs: list[str] = []
    for idx, (_label, payload) in enumerate(steps):
        arr = f"{prefix}_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {arr}[] = {{")
        if payload:
            lines.append(model_bin_to_inc(payload).rstrip())
        lines.append("};")
        lines.append("")
        step_refs.append(f"\t{{ .rx = {arr}, .rx_len = sizeof({arr}), .err = 0 }},")
    lines.append(f"static const nfc_session_mock_step_t {prefix}_{sym}_read_steps[] = {{")
    lines.extend(step_refs)
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define {tag}_{sym.upper()}_READ_STEP_COUNT "
        f"ARRAY_SIZE({prefix}_{sym}_read_steps)"
    )
    lines.append("")
    lines.append("#endif")
    lines.append("")
    out_path = out_dir / f"{stem}_mock.h"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out_path}", file=sys.stderr)


def build_hal_signal_model(meta: dict, stem: str) -> bytes:
    plain = parse_hex_bytes(meta.get("Plain data", ""))
    timings = [int(x) for x in meta.get("Timings", "").split()]
    variant = 1 if "long" in stem else 0
    buf = bytearray()
    buf.append(HAL_SIGNAL_BLOB_VERSION)
    buf.append(variant)
    buf.extend(struct.pack("<H", len(plain)))
    buf.extend(plain)
    buf.extend(struct.pack("<H", len(timings)))
    for t in timings:
        buf.extend(struct.pack("<H", t))
    return bytes(buf)


def hal_signal_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    plain = parse_hex_bytes(meta.get("Plain data", ""))
    timings_raw = [int(x) for x in meta.get("Timings", "").split()]
    steps: list[tuple[str, bytes]] = []
    if plain:
        steps.append(("nfca signal plain data", plain))
    if timings_raw:
        steps.append(("nfca signal timings", struct.pack(f"<{len(timings_raw)}H", *timings_raw)))
    return steps


def classic_type_from_meta(meta: dict) -> int:
    name = meta.get("Mifare Classic type", "1K")
    if name == "MINI":
        return CLASSIC_TYPE_MINI
    if name == "4K":
        return CLASSIC_TYPE_4K
    return CLASSIC_TYPE_1K


def classic_blocks_total(type_id: int) -> int:
    if type_id == CLASSIC_TYPE_MINI:
        return 20
    if type_id == CLASSIC_TYPE_4K:
        return 256
    return 64


def classic_sectors_total(type_id: int) -> int:
    if type_id == CLASSIC_TYPE_MINI:
        return 5
    if type_id == CLASSIC_TYPE_4K:
        return 40
    return 16


def classic_first_block_of_sector(sector: int) -> int:
    if sector < 32:
        return sector * 4
    return 32 * 4 + (sector - 32) * 16


def classic_blocks_in_sector(sector: int) -> int:
    return 16 if sector >= 32 else 4


def parse_classic_block_str(value: str) -> bytes | None:
    parts = value.replace(",", " ").split()
    out = bytearray(16)
    for i, part in enumerate(parts[:16]):
        if part == "??":
            return None
        out[i] = int(part, 16)
    return bytes(out)


def build_classic_model(meta: dict) -> bytes:
    uid = parse_hex_bytes(meta.get("UID", ""))
    atqa = parse_hex_bytes(meta.get("ATQA", "00 00"))
    sak = int(meta.get("SAK", "00"), 16)
    type_id = classic_type_from_meta(meta)
    blocks_map: dict[int, bytes] = {}
    blocks_total = classic_blocks_total(type_id)
    mask = [0] * CLASSIC_BLOCK_MASK_WORDS
    key_a_mask = 0
    key_b_mask = 0

    for block_num in range(blocks_total):
        key = f"Block {block_num}"
        if key not in meta:
            continue
        raw = parse_classic_block_str(meta[key])
        if raw is None:
            continue
        blocks_map[block_num] = raw
        mask[block_num // 32] |= 1 << (block_num % 32)
        if (block_num & 0x03) == 0x03 and block_num < 128:
            sector = block_num // 4
            key_a_mask |= 1 << sector
            key_b_mask |= 1 << sector
        elif block_num >= 128 and (block_num & 0x0F) == 0x0F:
            sector = 32 + (block_num - 128) // 16
            key_a_mask |= 1 << sector
            key_b_mask |= 1 << sector

    buf = bytearray()
    buf.append(CLASSIC_FORMAT_VERSION)
    buf.append(type_id)
    buf.append(len(uid))
    buf.extend(uid)
    buf.append(sak)
    buf.extend(atqa[:2].ljust(2, b"\x00"))
    for word in mask:
        buf.extend(struct.pack("<I", word))
    buf.extend(struct.pack("<Q", key_a_mask))
    buf.extend(struct.pack("<Q", key_b_mask))
    for block_num in range(blocks_total):
        buf.extend(blocks_map.get(block_num, b"\x00" * 16))
    return bytes(buf)


def _classic_auth_crypto_state(nt: bytes, cuid: int) -> Crypto1:
    crypto = Crypto1()
    nt_num = int.from_bytes(nt, "big")
    crypto.init(CLASSIC_DEFAULT_KEY)
    crypto.word(nt_num ^ cuid, 0)
    for b in CLASSIC_TEST_NR:
        crypto.byte(b, 0)
    nt_num = crypto.prng_successor(nt_num, 32)
    for _ in range(4):
        nt_num = crypto.prng_successor(nt_num, 8)
        crypto.byte(0, 0)
    crypto.word(0, 0)
    return crypto


def classic_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    uid = parse_hex_bytes(meta.get("UID", ""))
    type_id = classic_type_from_meta(meta)
    blocks_map: dict[int, bytes] = {}
    blocks_total = classic_blocks_total(type_id)
    sectors = classic_sectors_total(type_id)
    cuid = cuid_from_uid(uid)
    steps: list[tuple[str, bytes]] = []

    for block_num in range(blocks_total):
        key = f"Block {block_num}"
        if key in meta:
            raw = parse_classic_block_str(meta[key])
            if raw is not None:
                blocks_map[block_num] = raw

    if type_id == CLASSIC_TYPE_1K:
        steps.append(("classic detect 4k probe fail", b""))
        steps.append(("classic detect 1k probe nt", struct.pack(">I", 0x2000003E)))

    for sector in range(sectors):
        first = classic_first_block_of_sector(sector)
        nt = struct.pack(">I", 0x10000000 + sector)
        steps.append((f"classic AUTH NT sector {sector}", nt))

        nt_num = int.from_bytes(nt, "big")
        at_plain = struct.pack(">I", Crypto1.prng_successor(nt_num, 96))
        at_crypto = Crypto1()
        at_crypto.init(CLASSIC_DEFAULT_KEY)
        at_crypto.word(nt_num ^ cuid, 0)
        for b in CLASSIC_TEST_NR:
            at_crypto.byte(b, 0)
        at_enc = at_crypto.encrypt_bytes(at_plain)
        steps.append((f"classic AUTH AT sector {sector}", at_enc))

        crypto = _classic_auth_crypto_state(nt, cuid)

        for i in range(classic_blocks_in_sector(sector)):
            block_num = first + i
            if block_num >= blocks_total:
                break
            block = blocks_map.get(block_num, b"\x00" * 16)
            tx_plain = bytes([0x30, block_num])
            crc = iso14443_crc_a(tx_plain)
            tx_plain += struct.pack("<H", crc)
            crypto.encrypt_bytes(tx_plain)
            rx_plain = block + struct.pack("<H", iso14443_crc_a(block))
            rx_enc = crypto.encrypt_bytes(rx_plain)
            steps.append((f"classic READ block {block_num}", rx_enc))

    return steps


def emit_classic_mock_header(stem: str, steps: list[tuple[str, bytes]], model: bytes,
                              out_dir: Path) -> None:
    sym = stem.replace("-", "_")
    lines = [
        f"/* Auto-generated from Flipper {stem}.nfc — do not edit. */",
        "#ifndef CLASSIC_FIXTURE_" + sym.upper() + "_H_",
        "#define CLASSIC_FIXTURE_" + sym.upper() + "_H_",
        "",
        "#include \"nfc_session_mock.h\"",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include <zephyr/sys/util.h>",
        "",
        f"static const uint8_t classic_{sym}_model[] = {{",
        model_bin_to_inc(model).rstrip(),
        "};",
        "",
        f"#define CLASSIC_{sym.upper()}_MODEL_LEN sizeof(classic_{sym}_model)",
        "",
    ]
    step_refs: list[str] = []
    for idx, (_label, payload) in enumerate(steps):
        arr = f"classic_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {arr}[] = {{")
        if payload:
            lines.append(model_bin_to_inc(payload).rstrip())
        lines.append("};")
        lines.append("")
        step_refs.append(
            f"\t{{ .rx = {arr}, .rx_len = sizeof({arr}), .err = 0 }},"
        )
    lines.append(f"static const nfc_session_mock_step_t classic_{sym}_read_steps[] = {{")
    lines.extend(step_refs)
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define CLASSIC_{sym.upper()}_READ_STEP_COUNT "
        f"ARRAY_SIZE(classic_{sym}_read_steps)"
    )
    lines.append("")
    lines.append("#endif")
    lines.append("")
    out_path = out_dir / f"{stem}_mock.h"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out_path}", file=sys.stderr)


def classify(meta: dict, stem: str) -> str:
    filetype = meta.get("Filetype", "")
    if filetype == "Flipper NFC test":
        return "hal"
    device = meta.get("Device type", "")
    if device == "FeliCa":
        return "felica"
    if device == "SLIX":
        return "slix"
    if device in ("Mifare Ultralight 11", "Mifare Ultralight 21", "NTAG213",
                  "NTAG215", "NTAG216") or device == "NTAG/Ultralight":
        return "ultralight"
    if device == "Mifare Classic":
        return "classic"
    raise ValueError(f"unsupported device type: {device!r} in {stem}")


def persist_id_for(proto: str) -> int:
    if proto == "ultralight":
        return NFC_PERSIST_ID_ULTRALIGHT
    if proto == "felica":
        return NFC_PERSIST_ID_FELICA
    if proto == "slix":
        return NFC_PERSIST_ID_SLIX
    if proto == "classic":
        return NFC_PERSIST_ID_CLASSIC
    raise ValueError(f"no persist_id for proto {proto}")


def card_bin_allowed(proto: str) -> bool:
    return proto in ("ultralight", "felica", "slix", "classic")


def hand_flags_for(persist_id: int) -> int:
    base = PERSIST_HAND_FLAGS.get(persist_id, 0)
    return base | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED


def emit_fixture(meta: dict, out_dir: Path, stem: str, *, write_card_bin: bool) -> None:
    proto = classify(meta, stem)
    out_dir.mkdir(parents=True, exist_ok=True)

    if proto == "ultralight":
        model = build_ultralight_model(meta)
        steps = ultralight_read_steps(meta)
    elif proto == "felica":
        model = build_felica_model(meta)
        steps = felica_read_steps(meta)
    elif proto == "slix":
        model = build_slix_model(meta, stem)
        steps = slix_read_steps(meta, stem)
    elif proto == "hal":
        model = build_hal_signal_model(meta, stem)
        steps = hal_signal_read_steps(meta)
    elif proto == "classic":
        model = build_classic_model(meta)
        steps = classic_read_steps(meta)
    else:
        raise ValueError(f"unknown proto {proto}")

    model_path = out_dir / f"{stem}_model.bin"
    read_path = out_dir / f"{stem}_read.inc"
    model_path.write_bytes(model)
    read_path.write_text(read_inc_steps(steps), encoding="utf-8")
    if proto == "ultralight":
        emit_ultralight_mock_header(stem, steps, model, out_dir)
    elif proto == "classic":
        emit_classic_mock_header(stem, steps, model, out_dir)
    elif proto == "felica":
        emit_protocol_mock_header("felica", stem, steps, model, out_dir)
    elif proto == "slix":
        emit_protocol_mock_header("slix", stem, steps, model, out_dir)
    print(f"Wrote {model_path} ({len(model)} bytes)", file=sys.stderr)
    print(f"Wrote {read_path} ({len(steps)} steps)", file=sys.stderr)

    if write_card_bin and card_bin_allowed(proto):
        pid = persist_id_for(proto)
        flags = hand_flags_for(pid)
        emulation_complete = bool(flags & NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE)
        card = build_card_envelope(model, persist_id=pid, reader_capture=False,
                                   emulation_complete=emulation_complete)
        card_path = STORE_DIR / f"{stem}.card.bin"
        card_path.parent.mkdir(parents=True, exist_ok=True)
        card_path.write_bytes(card)
        card_inc = STORE_DIR / f"{stem}_card.inc"
        card_inc.write_text(model_bin_to_inc(card), encoding="utf-8")
        print(f"Wrote {card_path} ({len(card)} bytes)", file=sys.stderr)
        print(f"Wrote {card_inc}", file=sys.stderr)
    elif proto == "hal":
        print(f"Skipped .card.bin for HAL signal fixture {stem}", file=sys.stderr)


def process_file(path: Path, out_dir: Path | None, *, write_card_bin: bool) -> None:
    meta = parse_flipper_nfc(path)
    stem = path.stem
    proto = classify(meta, stem)
    target_dir = out_dir if out_dir is not None else PROTO_OUT_DIRS[proto]
    emit_fixture(meta, target_dir, stem, write_card_bin=write_card_bin)


def main() -> int:
    epilog = """
Examples:
  python3 scripts/nfc/flipper_nfc_to_fixture.py --all
  python3 scripts/nfc/flipper_nfc_to_fixture.py \\
    --input tests/fixtures/nfc/flipper/Ultralight_11.nfc \\
    --out-dir tests/fixtures/ultralight/ --card-bin

See docs/nfc/NFC_PROTOCOLS_COOKBOOK.md §14.11.
"""
    parser = argparse.ArgumentParser(
        description="Convert Flipper .nfc dumps to static test fixtures (offline; not CI).",
        epilog=epilog,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--input", type=Path, help="Path to .nfc file")
    parser.add_argument("--out-dir", type=Path,
                        help="Directory for derived fixtures (default: tests/fixtures/<proto>/)")
    parser.add_argument("--all", action="store_true",
                        help="Convert all 12 fixtures under tests/fixtures/nfc/flipper/")
    parser.add_argument("--card-bin", action="store_true",
                        help="Also emit tests/fixtures/store/<stem>.card.bin (not for HAL signals)")
    args = parser.parse_args()

    if args.all:
        nfc_files = sorted(FLIPPER_DIR.glob("*.nfc"))
        if not nfc_files:
            print(f"error: no .nfc files in {FLIPPER_DIR}", file=sys.stderr)
            return 1
        for path in nfc_files:
            try:
                process_file(path, args.out_dir, write_card_bin=args.card_bin)
            except ValueError as exc:
                print(f"error: {path.name}: {exc}", file=sys.stderr)
                return 1
        return 0

    if args.input is None:
        parser.error("--input or --all is required")
    if not args.input.is_file():
        print(f"error: not a file: {args.input}", file=sys.stderr)
        return 1
    try:
        process_file(args.input, args.out_dir, write_card_bin=args.card_bin)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
