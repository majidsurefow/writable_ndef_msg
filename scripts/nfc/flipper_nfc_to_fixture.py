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
    NFC_PERSIST_ID_DESFIRE,
    NFC_PERSIST_ID_FELICA,
    NFC_PERSIST_ID_SLIX,
    NFC_PERSIST_ID_ULTRALIGHT,
    NFC_STORE_ENTRY_FLAG_EMULATION_COMPLETE,
    NFC_STORE_ENTRY_FLAG_HAND_AUTHORED,
    PERSIST_HAND_FLAGS,
)
from mf_classic_crypto1 import Crypto1, cuid_from_uid, iso14443_crc_a
from ndef_to_card_bin import build_card_envelope

try:
    from ultralight_3des import auth_probe_response, encrypt, decrypt, shift_data
except ImportError:
    auth_probe_response = None

UL_TEST_RNDA = bytes(range(8))
UL_TEST_RNDB = bytes([0x10 + i for i in range(8)])
UL_TEST_KEY = bytes(16)
_UL_AUTH_CONT_TX = b""

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

ISO15693_INV_FLAGS = 0x26
ISO15693_CMD_FLAGS = 0x02
ISO15693_ADDR_FLAGS = 0x22
ISO15693_CMD_INVENTORY = 0x01
ISO15693_CMD_READ = 0x20
ISO15693_CMD_GET_SYS_INFO = 0x2B
ISO15693_CMD_GET_BLOCKS_SEC = 0x2C
ISO15693_BLOCKS_PER_QUERY = 32
SLIX_CMD_GET_NXP_SYSINFO = 0xAB
SLIX_CMD_READ_SIGNATURE = 0xBD
SLIX_NXP_MFG_CODE = 0x04

ISO15693_SYSINFO_FLAG_DSFID = 0x01
ISO15693_SYSINFO_FLAG_AFI = 0x10
ISO15693_SYSINFO_FLAG_MEMORY = 0x40
ISO15693_SYSINFO_FLAG_IC_REF = 0x80

PROTO_OUT_DIRS: dict[str, Path] = {
    "ultralight": REPO_ROOT / "tests" / "fixtures" / "ultralight",
    "felica": REPO_ROOT / "tests" / "fixtures" / "felica",
    "slix": REPO_ROOT / "tests" / "fixtures" / "slix",
    "iso15693_3": REPO_ROOT / "tests" / "fixtures" / "iso15693_3",
    "hal": REPO_ROOT / "tests" / "fixtures" / "hal",
    "classic": REPO_ROOT / "tests" / "fixtures" / "classic",
    "desfire": REPO_ROOT / "tests" / "fixtures" / "desfire",
}

DESFIRE_FORMAT_VERSION = 0x01
DESFIRE_SW1 = 0x91
DESFIRE_STATUS_OK = 0x00
DESFIRE_STATUS_AF = 0xAF

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


def tx_inc_steps(steps: list[tuple[str, bytes]]) -> str:
    lines = ["/* Tier B poller TX golden — one STEP per transceive */"]
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
    for idx, (label, payload) in enumerate(steps):
        arr = f"ultralight_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {arr}[] = {{")
        if payload:
            lines.append(model_bin_to_inc(payload).rstrip())
        lines.append("};")
        lines.append("")
        tx_arr = f"ultralight_{sym}_step{idx}_tx"
        tx_payload = ultralight_build_tx(label)
        lines.append(f"static const uint8_t {tx_arr}[] = {{")
        if tx_payload:
            lines.append(model_bin_to_inc(tx_payload).rstrip())
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
    lines.append(f"static const uint8_t *const ultralight_{sym}_read_tx_steps[] = {{")
    for idx, (_label, _payload) in enumerate(steps):
        lines.append(f"\tultralight_{sym}_step{idx}_tx,")
    lines.append("};")
    lines.append("")
    lines.append(f"static const size_t ultralight_{sym}_read_tx_lens[] = {{")
    for idx, (_label, _payload) in enumerate(steps):
        lines.append(f"\tsizeof(ultralight_{sym}_step{idx}_tx),")
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define ULTRALIGHT_{sym.upper()}_READ_TX_STEP_COUNT "
        f"ARRAY_SIZE(ultralight_{sym}_read_tx_steps)"
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


def ultralight_is_mful_c(meta: dict) -> bool:
    device = meta.get("Device type", "")
    ntag_type = meta.get("NTAG/Ultralight type", "")
    return "Ultralight C" in device or "Ultralight C" in ntag_type


def ultralight_build_tx(label: str) -> bytes:
    if label in ("ultralight GET_VERSION", "ultralight GET_VERSION fail"):
        return bytes([0x60])
    if label == "ultralight AUTH test":
        return bytes([0x1A, 0x00])
    if label == "ultralight AUTH start":
        return bytes([0x1A, 0x00])
    if label == "ultralight AUTH cont":
        return _UL_AUTH_CONT_TX if _UL_AUTH_CONT_TX else bytes([0xAF]) + bytes(16)
    if label == "ultralight READ_SIG":
        return bytes([0x3C, 0x00])
    if label.startswith("ultralight READ_CNT "):
        return bytes([0x39, int(label.rsplit(" ", 1)[-1])])
    if label.startswith("ultralight CHECK_TEARING "):
        return bytes([0x3E, int(label.rsplit(" ", 1)[-1])])
    if label.startswith("ultralight PWD_AUTH "):
        parts = label.split()
        if len(parts) >= 6:
            pwd = bytes(int(p, 16) for p in parts[2:6])
            return bytes([0x1B]) + pwd
        return bytes([0x1B, 0xFF, 0xFF, 0xFF, 0xFF])
    if label.startswith("ultralight READ page "):
        parts = label.split()
        if parts[-1] == "fail":
            page = int(parts[-2])
        elif parts[-1] == "probe":
            page = 41
        else:
            page = int(parts[-1])
        return bytes([0x30, page])
    return b""


def ultralight_auth_rx_steps() -> list[tuple[str, bytes]]:
    global _UL_AUTH_CONT_TX
    if auth_probe_response is None:
        return []
    probe = auth_probe_response(UL_TEST_KEY, UL_TEST_RNDB)
    enc_rnd_b = probe[1:]
    iv0 = b"\x00" * 8
    rnd_b = decrypt(UL_TEST_KEY, iv0, enc_rnd_b)
    shifted_b = shift_data(rnd_b)
    output = bytearray(16)
    output[:8] = UL_TEST_RNDA
    output[8:] = shifted_b
    enc_out = encrypt(UL_TEST_KEY, enc_rnd_b, bytes(output))
    _UL_AUTH_CONT_TX = bytes([0xAF]) + enc_out
    shifted_a = shift_data(bytes(UL_TEST_RNDA))
    enc_resp = encrypt(UL_TEST_KEY, enc_out[8:16], shifted_a)
    finish = bytes([0x00]) + enc_resp
    return [
        ("ultralight AUTH test", probe),
        ("ultralight AUTH start", probe),
        ("ultralight AUTH cont", finish),
    ]


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
    if auth0 < 0x03:
        return None
    if auth0 >= pages_total:
        return None
    if prot or auth0 < pages_total:
        return auth0
    return None


def ultralight_insert_pwd_auth_step(steps: list[tuple[str, bytes]], meta: dict,
                                    pages_total: int) -> list[tuple[str, bytes]]:
    auth0 = ultralight_locked_auth0(meta)
    if auth0 is None:
        return steps

    auth0_page = pages_total - 4
    pack_page = pages_total - 1
    pwd_page = pages_total - 2
    pages_map: dict[int, bytes] = meta.get("pages", {})
    pack_bytes = pages_map.get(pack_page, b"\x00\x00")[:2]
    pwd_bytes = pages_map.get(pwd_page, b"\x00\x00\x00\x00")[:4]
    if len(pack_bytes) < 2:
        pack_bytes = pack_bytes.ljust(2, b"\x00")
    if len(pwd_bytes) < 4:
        pwd_bytes = pwd_bytes.ljust(4, b"\x00")
    pwd_label = "ultralight PWD_AUTH " + " ".join(f"{b:02x}" for b in pwd_bytes)

    out: list[tuple[str, bytes]] = []
    inserted = False
    for label, rx in steps:
        if (not inserted) and label.startswith("ultralight READ page "):
            page = int(label.split()[-1])
            if page > auth0_page:
                out.append((pwd_label, pack_bytes))
                inserted = True
        out.append((label, rx))
    return out


def ultralight_read_steps(meta: dict, stem: str) -> list[tuple[str, bytes]]:
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
        if ultralight_is_mful_c(meta):
            steps.extend(ultralight_auth_rx_steps())
        else:
            steps.append(("ultralight AUTH test", b"\x00"))
            page41 = b"".join(pages_map.get(p, b"\x00" * 4) for p in range(41, min(45, pages_total)))
            page41 = page41.ljust(16, b"\x00")
            steps.append(("ultralight READ page 41 probe", page41))

    auth0 = ultralight_locked_auth0(meta)
    read_limit = int(meta.get("Pages total", len(pages_map)))
    if "locked" in stem.lower():
        if auth0 is not None:
            read_limit = auth0
        else:
            read_limit = int(meta.get("Pages read", read_limit))

    for start in range(0, read_limit, 4):
        chunk = b"".join(pages_map.get(p, b"\x00" * 4) for p in range(start, min(start + 4, pages_total)))
        if len(chunk) < 16:
            chunk = chunk.ljust(16, b"\x00")
        steps.append((f"ultralight READ page {start}", chunk))

    if auth0 is not None and "locked" in stem.lower():
        steps.append((f"ultralight READ page {auth0} fail", b"\x00"))
        return steps

    if len(version) == 8 and any(version):
        steps.append(("ultralight READ_SIG", signature.ljust(32, b"\x00")[:32]))
        for i in range(3):
            counter = counters[i] if i < len(counters) else 0
            steps.append((f"ultralight READ_CNT {i}", struct.pack("<I", counter)[:3]))
        for i in range(3):
            flag = tearings[i] if i < len(tearings) else 0
            steps.append((f"ultralight CHECK_TEARING {i}", bytes([flag])))

    if "locked" not in stem.lower():
        steps = ultralight_insert_pwd_auth_step(steps, meta, pages_total)

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


FELICA_BLOCKS_LITE_TOTAL = 28
FELICA_BLOCK_INDEX_REG = 0x0E
FELICA_BLOCK_INDEX_RC = 0x80
FELICA_BLOCK_INDEX_MC = 0x88
FELICA_BLOCK_INDEX_WCNT = 0x90
FELICA_BLOCK_INDEX_STATE = 0x92
FELICA_BLOCK_INDEX_CRC_CHECK = 0xA0
FELICA_SERVICE_RO_ACCESS = 0x000B
FELICA_CMD_POLL = 0x00
FELICA_CMD_READ = 0x06
FELICA_CMD_POLL_RESP = 0x01
FELICA_CMD_READ_RESP = 0x07


def felica_crc_calculate(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return ((crc << 8) | (crc >> 8)) & 0xFFFF


def felica_crc_append(data: bytes) -> bytes:
    crc = felica_crc_calculate(data)
    return data + struct.pack("<H", crc)


def felica_lite_wire_block_indices() -> list[int]:
    indices: list[int] = []
    idx = 0
    for _ in range(FELICA_BLOCKS_LITE_TOTAL):
        indices.append(idx)
        idx += 1
        if idx == FELICA_BLOCK_INDEX_REG + 1:
            idx = FELICA_BLOCK_INDEX_RC
        elif idx == FELICA_BLOCK_INDEX_MC + 1:
            idx = FELICA_BLOCK_INDEX_WCNT
        elif idx == FELICA_BLOCK_INDEX_STATE + 1:
            idx = FELICA_BLOCK_INDEX_CRC_CHECK
    return indices


def felica_build_poll_tx() -> bytes:
    body = bytes([0x06, FELICA_CMD_POLL, 0xFF, 0xFF, 0x00, 0x00])
    return felica_crc_append(body)


def felica_build_poll_rx(idm: bytes, pmm: bytes) -> bytes:
    body = bytes([0x12, FELICA_CMD_POLL_RESP]) + idm[:8].ljust(8, b"\x00") + pmm[:8].ljust(8, b"\x00")
    return felica_crc_append(body)


def felica_build_read_tx(idm: bytes, block_num: int) -> bytes:
    cmd = bytes([FELICA_CMD_READ]) + idm[:8].ljust(8, b"\x00") + bytes([0x01, 0x0B, 0x00, 0x01])
    block_list = bytes([0x80, block_num & 0xFF])
    total = len(cmd) + 1 + len(block_list)
    body = bytes([total]) + cmd + block_list
    return felica_crc_append(body)


def felica_build_read_rx(idm: bytes, sf1: int, sf2: int, data: bytes) -> bytes:
    payload = bytes([FELICA_CMD_READ_RESP]) + idm[:8].ljust(8, b"\x00") + bytes([sf1, sf2, 0x01])
    payload += data.ljust(16, b"\x00")[:16]
    body = bytes([len(payload) + 1]) + payload
    return felica_crc_append(body)


def felica_framed_read_steps(meta: dict) -> list[tuple[str, bytes, bytes]]:
    blocks_map: dict[int, bytes] = meta.get("blocks", {})
    blocks_total = int(meta.get("Blocks total", len(blocks_map)))
    uid = parse_hex_bytes(meta.get("Manufacture id", meta.get("UID", "")))
    pmm = parse_hex_bytes(meta.get("Manufacture parameter", ""))
    idm = uid[:8].ljust(8, b"\x00")
    steps: list[tuple[str, bytes, bytes]] = []

    steps.append(("felica poll", felica_build_poll_tx(), felica_build_poll_rx(idm, pmm)))

    wire_blocks = felica_lite_wire_block_indices()
    for dump_idx, wire_block in enumerate(wire_blocks[:blocks_total]):
        raw = blocks_map.get(dump_idx, b"\x00" * 18)
        sf1, sf2 = raw[0], raw[1] if len(raw) >= 2 else 0
        data = raw[2:18].ljust(16, b"\x00") if len(raw) >= 2 else raw.ljust(16, b"\x00")
        tx = felica_build_read_tx(idm, wire_block)
        rx = felica_build_read_rx(idm, sf1, sf2, data)
        steps.append((f"felica READ block {wire_block} (dump {dump_idx})", tx, rx))

    return steps


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


def iso15693_crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0x8408
            else:
                crc >>= 1
    return (~crc) & 0xFFFF


def iso15693_append_crc(payload: bytes) -> bytes:
    crc = iso15693_crc16(payload)
    return payload + bytes([crc & 0xFF, (crc >> 8) & 0xFF])


def slix_type_from_uid(uid: bytes) -> int:
    if len(uid) != 8 or uid[0] != 0xE0 or uid[1] != SLIX_NXP_MFG_CODE:
        return -1
    mapping = {0x01: 0, 0x02: 1, 0x03: 2, 0x04: 3}
    return mapping.get(uid[2], 0)


def slix_type_has_nxp_sysinfo(slix_type: int) -> bool:
    return slix_type == 3


def slix_type_has_signature(slix_type: int) -> bool:
    return slix_type == 3


def iso15693_framed_steps(meta: dict) -> list[tuple[str, bytes, bytes]]:
    uid = parse_hex_bytes(meta.get("UID", ""))
    dsfid = int(meta.get("DSFID", "0"), 16) if "DSFID" in meta else 0
    afi = int(meta.get("AFI", "0"), 16) if "AFI" in meta else 0
    ic_ref = int(meta.get("IC Reference", "0"), 16) if "IC Reference" in meta else 0
    block_count = int(meta.get("Block Count", "0"))
    block_size = int(meta.get("Block Size", "4"), 16)
    data = parse_hex_bytes(meta.get("Data Content", ""))
    security = parse_hex_bytes(meta.get("Security Status", ""))

    if len(security) < block_count:
        security = security.ljust(block_count, b"\x00")
    else:
        security = security[:block_count]

    steps: list[tuple[str, bytes, bytes]] = []
    steps.append((
        "iso15693 inventory",
        bytes([ISO15693_INV_FLAGS, ISO15693_CMD_INVENTORY]),
        bytes([0x00, dsfid]) + uid[::-1],
    ))

    info_flags = ISO15693_SYSINFO_FLAG_DSFID | ISO15693_SYSINFO_FLAG_AFI
    info_flags |= ISO15693_SYSINFO_FLAG_MEMORY | ISO15693_SYSINFO_FLAG_IC_REF
    sysinfo_rx = bytearray([0x00, info_flags, dsfid, afi, block_count - 1, block_size - 1, ic_ref])
    steps.append((
        "iso15693 GET_SYSTEM_INFO",
        bytes([ISO15693_CMD_FLAGS, ISO15693_CMD_GET_SYS_INFO]),
        bytes(sysinfo_rx),
    ))

    for block_num in range(block_count):
        start = block_num * block_size
        payload = data[start:start + block_size].ljust(block_size, b"\x00")
        steps.append((
            f"iso15693 READ block {block_num}",
            bytes([ISO15693_CMD_FLAGS, ISO15693_CMD_READ, block_num]),
            bytes([0x00]) + payload,
        ))

    for start in range(0, block_count, ISO15693_BLOCKS_PER_QUERY):
        batch = min(block_count - start, ISO15693_BLOCKS_PER_QUERY)
        steps.append((
            f"iso15693 GET_BLOCKS_SECURITY start {start}",
            bytes([ISO15693_CMD_FLAGS, ISO15693_CMD_GET_BLOCKS_SEC, start, batch - 1]),
            bytes([0x00]) + security[start:start + batch],
        ))

    return steps


def iso15693_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    return [(label, rx) for label, _tx, rx in iso15693_framed_steps(meta)]


def slix_framed_steps(meta: dict, stem: str) -> list[tuple[str, bytes, bytes]]:
    steps = iso15693_framed_steps(meta)
    uid = parse_hex_bytes(meta.get("UID", ""))
    slix_type = slix_type_from_uid(uid)
    signature = parse_hex_bytes(meta.get("Signature", ""))
    protection_pointer = int(meta.get("Protection Pointer", "0"), 16) if "Protection Pointer" in meta else 0
    protection_condition = int(meta.get("Protection Condition", "0"), 16) if "Protection Condition" in meta else 0

    if slix_type_has_nxp_sysinfo(slix_type):
        tx = bytes([ISO15693_ADDR_FLAGS, SLIX_CMD_GET_NXP_SYSINFO, SLIX_NXP_MFG_CODE]) + uid[::-1]
        steps.append((
            "slix GET_NXP_SYSTEM_INFO",
            tx,
            bytes([0x00, protection_pointer, protection_condition]),
        ))

    if slix_type_has_signature(slix_type):
        tx = bytes([ISO15693_ADDR_FLAGS, SLIX_CMD_READ_SIGNATURE, SLIX_NXP_MFG_CODE]) + uid[::-1]
        steps.append((
            "slix READ_SIGNATURE",
            tx,
            signature.ljust(32, b"\x00")[:32],
        ))

    return steps


def slix_read_steps(meta: dict, stem: str) -> list[tuple[str, bytes]]:
    return [(label, rx) for label, _tx, rx in slix_framed_steps(meta, stem)]


def framed_steps_to_rx(steps: list[tuple[str, bytes, bytes]]) -> list[tuple[str, bytes]]:
    return [(label, rx) for label, _tx, rx in steps]


def emit_framed_mock_header(prefix: str, stem: str, framed: list[tuple[str, bytes, bytes]],
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
    for idx, (_label, tx_pre, rx_pre) in enumerate(framed):
        tx_wire = iso15693_append_crc(tx_pre)
        rx_wire = iso15693_append_crc(rx_pre)
        tx_arr = f"{prefix}_{sym}_step{idx}_tx"
        rx_arr = f"{prefix}_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {tx_arr}[] = {{")
        lines.append(model_bin_to_inc(tx_wire).rstrip())
        lines.append("};")
        lines.append("")
        lines.append(f"static const uint8_t {rx_arr}[] = {{")
        if rx_wire:
            lines.append(model_bin_to_inc(rx_wire).rstrip())
        lines.append("};")
        lines.append("")
        step_refs.append(
            f"\t{{ .rx = {rx_arr}, .rx_len = sizeof({rx_arr}), .err = 0 }},"
        )
    lines.append(f"static const nfc_session_mock_step_t {prefix}_{sym}_read_steps[] = {{")
    lines.extend(step_refs)
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define {tag}_{sym.upper()}_READ_STEP_COUNT "
        f"ARRAY_SIZE({prefix}_{sym}_read_steps)"
    )
    lines.append("")
    for idx, (_label, tx_pre, _rx_pre) in enumerate(framed):
        tx_wire = iso15693_append_crc(tx_pre)
        lines.append(
            f"#define {tag}_{sym.upper()}_STEP{idx}_TX_LEN {len(tx_wire)}U"
        )
    lines.append("")
    lines.append("#endif")
    lines.append("")
    out_path = out_dir / f"{stem}_mock.h"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out_path}", file=sys.stderr)


def emit_felica_framed_mock_header(stem: str, steps: list[tuple[str, bytes, bytes]],
                                   model: bytes, out_dir: Path) -> None:
    sym = stem.replace("-", "_")
    lines = [
        f"/* Auto-generated from Flipper {stem}.nfc — do not edit. */",
        f"#ifndef FELICA_FIXTURE_{sym.upper()}_H_",
        f"#define FELICA_FIXTURE_{sym.upper()}_H_",
        "",
        "#include \"nfc_session_mock.h\"",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include <zephyr/sys/util.h>",
        "",
        f"static const uint8_t felica_{sym}_model[] = {{",
        model_bin_to_inc(model).rstrip(),
        "};",
        "",
        f"#define FELICA_{sym.upper()}_MODEL_LEN sizeof(felica_{sym}_model)",
        "",
    ]
    step_refs: list[str] = []
    for idx, (_label, tx_payload, rx_payload) in enumerate(steps):
        tx_arr = f"felica_{sym}_step{idx}_tx"
        rx_arr = f"felica_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {tx_arr}[] = {{")
        if tx_payload:
            lines.append(model_bin_to_inc(tx_payload).rstrip())
        lines.append("};")
        lines.append("")
        lines.append(f"static const uint8_t {rx_arr}[] = {{")
        if rx_payload:
            lines.append(model_bin_to_inc(rx_payload).rstrip())
        lines.append("};")
        lines.append("")
        step_refs.append(f"\t{{ .rx = {rx_arr}, .rx_len = sizeof({rx_arr}), .err = 0 }},")
    lines.append(f"static const nfc_session_mock_step_t felica_{sym}_read_steps[] = {{")
    lines.extend(step_refs)
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define FELICA_{sym.upper()}_READ_STEP_COUNT "
        f"ARRAY_SIZE(felica_{sym}_read_steps)"
    )
    lines.append("")
    lines.append(f"static const uint8_t *const felica_{sym}_read_tx_steps[] = {{")
    for idx in range(len(steps)):
        lines.append(f"\tfelica_{sym}_step{idx}_tx,")
    lines.append("};")
    lines.append("")
    lines.append(f"static const size_t felica_{sym}_read_tx_lens[] = {{")
    for idx, (_label, tx_payload, _rx_payload) in enumerate(steps):
        lines.append(f"\tsizeof(felica_{sym}_step{idx}_tx),")
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define FELICA_{sym.upper()}_READ_TX_STEP_COUNT "
        f"ARRAY_SIZE(felica_{sym}_read_tx_steps)"
    )
    lines.append("")
    lines.append("#endif")
    lines.append("")
    out_path = out_dir / f"{stem}_mock.h"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out_path}", file=sys.stderr)


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

    for block_num, raw in meta.get("blocks", {}).items():
        if (not isinstance(block_num, int)) or (block_num < 0) or (block_num >= blocks_total):
            continue
        if isinstance(raw, str):
            raw = parse_classic_block_str(raw)
        if raw is None:
            continue
        blocks_map[block_num] = raw[:16].ljust(16, b"\x00")
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


def classic_framed_steps(meta: dict) -> list[tuple[str, bytes, bytes]]:
    uid = parse_hex_bytes(meta.get("UID", ""))
    type_id = classic_type_from_meta(meta)
    blocks_total = classic_blocks_total(type_id)
    sectors = classic_sectors_total(type_id)
    cuid = cuid_from_uid(uid)
    framed: list[tuple[str, bytes, bytes]] = []
    blocks_map: dict[int, bytes] = {}

    for block_num, raw in meta.get("blocks", {}).items():
        if (not isinstance(block_num, int)) or (block_num < 0) or (block_num >= blocks_total):
            continue
        if isinstance(raw, str):
            raw = parse_classic_block_str(raw)
        if raw is not None:
            blocks_map[block_num] = raw[:16].ljust(16, b"\x00")

    if type_id == CLASSIC_TYPE_4K:
        framed.append(("classic detect 4k probe nt", bytes([0x60, 254]),
                        struct.pack(">I", 0x200000FE)))
    elif type_id == CLASSIC_TYPE_1K:
        framed.append(("classic detect 4k probe fail", bytes([0x60, 254]), b""))
        framed.append(("classic detect 1k probe nt", bytes([0x60, 62]),
                        struct.pack(">I", 0x2000003E)))
    elif type_id == CLASSIC_TYPE_MINI:
        framed.append(("classic detect 4k probe fail", bytes([0x60, 254]), b""))
        framed.append(("classic detect 1k probe fail", bytes([0x60, 62]), b""))

    for sector in range(sectors):
        first = classic_first_block_of_sector(sector)
        nt = struct.pack(">I", 0x10000000 + sector)
        framed.append((f"classic AUTH NT sector {sector}", bytes([0x60, first]), nt))

        nt_num = int.from_bytes(nt, "big")
        at_plain = struct.pack(">I", Crypto1.prng_successor(nt_num, 96))
        at_crypto = Crypto1()
        at_crypto.init(CLASSIC_DEFAULT_KEY)
        at_crypto.word(nt_num ^ cuid, 0)
        for b in CLASSIC_TEST_NR:
            at_crypto.byte(b, 0)
        at_enc = at_crypto.encrypt_bytes(at_plain)

        tx_auth = Crypto1().encrypt_reader_nonce(
            CLASSIC_DEFAULT_KEY, cuid, nt, CLASSIC_TEST_NR
        )
        framed.append((f"classic AUTH NR|AR sector {sector}", tx_auth, at_enc))

        crypto = _classic_auth_crypto_state(nt, cuid)

        for i in range(classic_blocks_in_sector(sector)):
            block_num = first + i
            if block_num >= blocks_total:
                break
            block = blocks_map.get(block_num, b"\x00" * 16)
            tx_plain = bytes([0x30, block_num])
            crc = iso14443_crc_a(tx_plain)
            tx_plain += struct.pack("<H", crc)
            tx_enc = crypto.encrypt_bytes(tx_plain)
            rx_plain = block + struct.pack("<H", iso14443_crc_a(block))
            rx_enc = crypto.encrypt_bytes(rx_plain)
            framed.append((f"classic READ block {block_num}", tx_enc, rx_enc))

    return framed


def classic_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    return [(label, rx) for label, _tx, rx in classic_framed_steps(meta)]


def classic_read_tx_steps(meta: dict) -> list[tuple[str, bytes]]:
    return [(label, tx) for label, tx, _rx in classic_framed_steps(meta)]


def emit_classic_mock_header(stem: str, steps: list[tuple[str, bytes]], model: bytes,
                              out_dir: Path,
                              tx_steps: list[tuple[str, bytes]] | None = None) -> None:
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
    if tx_steps:
        lines.append(f"#define CLASSIC_{sym.upper()}_TX_STEP_COUNT {len(tx_steps)}U")
        lines.append("")
        tx_ptrs: list[str] = []
        tx_lens: list[str] = []
        for idx, (_label, payload) in enumerate(tx_steps):
            tx_arr = f"classic_{sym}_step{idx}_tx"
            lines.append(f"static const uint8_t {tx_arr}[] = {{")
            if payload:
                lines.append(model_bin_to_inc(payload).rstrip())
            lines.append("};")
            lines.append("")
            lines.append(f"#define CLASSIC_{sym.upper()}_STEP{idx}_TX_LEN {len(payload)}U")
            tx_ptrs.append(tx_arr)
            tx_lens.append(f"CLASSIC_{sym.upper()}_STEP{idx}_TX_LEN")
        lines.append(
            f"static const uint8_t *const classic_{sym}_tx_steps"
            f"[CLASSIC_{sym.upper()}_TX_STEP_COUNT] = {{"
        )
        lines.append(",\n".join(f"\t{ptr}" for ptr in tx_ptrs))
        lines.append("};")
        lines.append("")
        lines.append(
            f"static const size_t classic_{sym}_tx_lens"
            f"[CLASSIC_{sym.upper()}_TX_STEP_COUNT] = {{"
        )
        lines.append(",\n".join(f"\t{length}" for length in tx_lens))
        lines.append("};")
        lines.append("")
    lines.append("#endif")
    lines.append("")
    out_path = out_dir / f"{stem}_mock.h"
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {out_path}", file=sys.stderr)


def desfire_key_settings(meta: dict, prefix: str) -> int:
    ks = 0
    if meta.get(f"{prefix} Config Changeable", "false") == "true":
        ks |= 0x01
    if meta.get(f"{prefix} Free Create Delete", "false") == "true":
        ks |= 0x02
    if meta.get(f"{prefix} Key Changeable", "false") == "true":
        ks |= 0x04
    if meta.get(f"{prefix} Free Directory List", "false") == "true":
        ks |= 0x08
    return ks


def desfire_app_prefix(meta: dict, aid: bytes) -> str:
    return f"Application {aid[0]:02X}{aid[1]:02X}{aid[2]:02X}"


def desfire_resp(payload: bytes, sw2: int = DESFIRE_STATUS_OK) -> bytes:
    return payload + bytes([DESFIRE_SW1, sw2])


def build_desfire_model(meta: dict) -> bytes:
    ver = parse_hex_bytes(meta["PICC Version"])
    if len(ver) < 28:
        raise ValueError(f"PICC Version must be at least 28 bytes, got {len(ver)}")
    ver = ver[:28]
    free_mem = int(meta["PICC Free Memory"])
    body = bytearray()
    body.append(DESFIRE_FORMAT_VERSION)
    body.extend(ver[:7])
    body.extend(ver[7:14])
    body.extend(ver[14:21])
    body.extend(ver[21:26])
    body.extend(ver[26:28])
    body.extend(struct.pack("<I", free_mem))
    body.extend(bytes(16))
    body.append(desfire_key_settings(meta, "PICC"))

    app_count = int(meta.get("Application Count", "0"))
    body.append(app_count)
    app_ids = parse_hex_bytes(meta.get("Application IDs", ""))
    for ai in range(app_count):
        aid = app_ids[ai * 3:(ai + 1) * 3]
        prefix = desfire_app_prefix(meta, aid)
        body.extend(aid)
        body.append(desfire_key_settings(meta, prefix))
        key_count = int(meta.get(f"{prefix} Max Keys", "0"))
        body.append(key_count)
        file_ids = parse_hex_bytes(meta.get(f"{prefix} File IDs", ""))
        body.append(len(file_ids))
        for fid in file_ids:
            fp = f"{prefix} File {fid}"
            ftype = int(meta.get(f"{fp} Type", "0"))
            fcomm = int(meta.get(f"{fp} Communication Settings", "0"))
            access = parse_hex_bytes(meta.get(f"{fp} Access Rights", "EE EE"))
            fdata = parse_hex_bytes(meta.get(fp, ""))
            body.append(fid)
            fs = bytearray(8)
            fs[0] = ftype
            fs[1] = fcomm
            fs[2] = access[0]
            fs[3] = access[1]
            if ftype in (0, 1):
                size = int(meta.get(f"{fp} Size", str(len(fdata))))
                fs[4] = size & 0xFF
                fs[5] = (size >> 8) & 0xFF
                fs[6] = (size >> 16) & 0xFF
            elif ftype == 2:
                if len(fdata) >= 4:
                    fs[4:8] = fdata[:4]
            else:
                rec_size = int(meta.get(f"{fp} Size", str(len(fdata) if fdata else 1)))
                fs[4] = rec_size & 0xFF
                fs[5] = (rec_size >> 8) & 0xFF
                fs[6] = (rec_size >> 16) & 0xFF
            body.extend(fs)
            body.extend(struct.pack("<H", len(fdata)))
            body.extend(fdata)
    return bytes(body)


def desfire_version_frames(meta: dict) -> list[bytes]:
    ver = parse_hex_bytes(meta["PICC Version"])[:28]
    return [
        ver[:7] + bytes([DESFIRE_SW1, DESFIRE_STATUS_AF]),
        ver[7:14] + bytes([DESFIRE_SW1, DESFIRE_STATUS_AF]),
        ver[14:] + bytes([DESFIRE_SW1, DESFIRE_STATUS_OK]),
    ]


def desfire_read_steps(meta: dict) -> list[tuple[str, bytes]]:
    steps: list[tuple[str, bytes]] = [
        ("iso select desfire aid", bytes([0x90, 0x00])),
        ("get key version", desfire_resp(bytes([0x00]))),
    ]
    for idx, frame in enumerate(desfire_version_frames(meta)):
        steps.append((f"get version frame {idx}", frame))
    free_mem = int(meta["PICC Free Memory"])
    steps.append(("get free memory", desfire_resp(struct.pack("<I", free_mem))))
    master_ks = desfire_key_settings(meta, "PICC")
    picc_max_keys = int(meta.get("PICC Max Keys", "1"))
    steps.append(("get picc key settings", desfire_resp(bytes([master_ks, picc_max_keys]))))

    app_ids = parse_hex_bytes(meta.get("Application IDs", ""))
    if app_ids:
        steps.append(("get application ids", desfire_resp(app_ids)))

    app_count = int(meta.get("Application Count", "0"))
    for ai in range(app_count):
        aid = app_ids[ai * 3:(ai + 1) * 3]
        prefix = desfire_app_prefix(meta, aid)
        aid_hex = f"{aid[0]:02X}{aid[1]:02X}{aid[2]:02X}"
        steps.append((f"select app {aid_hex}", desfire_resp(b"")))
        app_ks = desfire_key_settings(meta, prefix)
        app_max = int(meta.get(f"{prefix} Max Keys", "0"))
        steps.append((f"app {aid_hex} key settings", desfire_resp(bytes([app_ks, app_max]))))
        file_ids = parse_hex_bytes(meta.get(f"{prefix} File IDs", ""))
        steps.append((f"app {aid_hex} file ids", desfire_resp(bytes(file_ids))))
        for fid in file_ids:
            fp = f"{prefix} File {fid}"
            ftype = int(meta.get(f"{fp} Type", "0"))
            fcomm = int(meta.get(f"{fp} Communication Settings", "0"))
            access = parse_hex_bytes(meta.get(f"{fp} Access Rights", "EE EE"))
            size = int(meta.get(f"{fp} Size", "0"))
            fs_payload = bytes([
                ftype, fcomm, access[0], access[1],
                size & 0xFF, (size >> 8) & 0xFF, (size >> 16) & 0xFF,
            ])
            steps.append((f"file {fid} settings", desfire_resp(fs_payload)))
            fdata = parse_hex_bytes(meta.get(fp, ""))
            if ftype == 2:
                steps.append((f"file {fid} get value", desfire_resp(fdata[:4])))
            elif ftype in (3, 4):
                steps.append((f"file {fid} read records", desfire_resp(fdata)))
            else:
                steps.append((f"file {fid} read data", desfire_resp(fdata)))
    return steps


def emit_desfire_mock_header(stem: str, steps: list[tuple[str, bytes]], model: bytes,
                             out_dir: Path) -> None:
    sym = "Desfire" if stem == "MfDesfire_EV1_sample" else stem.replace("-", "_")
    lines = [
        f"/* Auto-generated from Flipper {stem}.nfc — do not edit. */",
        f"#ifndef TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_",
        f"#define TESTS_FIXTURES_DESFIRE_DESFIRE_MOCK_H_",
        "",
        "#include \"nfc_session_mock.h\"",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include <zephyr/sys/util.h>",
        "",
        f"static const uint8_t desfire_{sym}_model[] = {{",
        model_bin_to_inc(model).rstrip(),
        "};",
        "",
        f"#define DESFIRE_{sym.upper()}_MODEL_LEN ((size_t)sizeof(desfire_{sym}_model))",
        "",
    ]
    step_refs: list[str] = []
    for idx, (_label, payload) in enumerate(steps):
        arr = f"desfire_{sym}_step{idx}_rx"
        lines.append(f"static const uint8_t {arr}[] = {{")
        if payload:
            lines.append(model_bin_to_inc(payload).rstrip())
        lines.append("};")
        lines.append("")
        step_refs.append(f"\t{{.rx = {arr}, .rx_len = sizeof({arr}), .err = 0}},")
    lines.append(f"static const nfc_session_mock_step_t desfire_{sym}_read_steps[] = {{")
    lines.extend(step_refs)
    lines.append("};")
    lines.append("")
    lines.append(
        f"#define DESFIRE_{sym.upper()}_READ_STEP_COUNT "
        f"ARRAY_SIZE(desfire_{sym}_read_steps)"
    )
    lines.append("")
    lines.append("#endif")
    lines.append("")
    mock_name = "Desfire_mock.h" if stem == "MfDesfire_EV1_sample" else f"{stem}_mock.h"
    out_path = out_dir / mock_name
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
    if device == "Mifare DESFire":
        return "desfire"
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
    if proto == "desfire":
        return NFC_PERSIST_ID_DESFIRE
    raise ValueError(f"no persist_id for proto {proto}")


def card_bin_allowed(proto: str) -> bool:
    return proto in ("ultralight", "felica", "slix", "classic", "desfire")


def hand_flags_for(persist_id: int) -> int:
    base = PERSIST_HAND_FLAGS.get(persist_id, 0)
    return base | NFC_STORE_ENTRY_FLAG_HAND_AUTHORED


def emit_fixture(meta: dict, out_dir: Path, stem: str, *, write_card_bin: bool) -> None:
    proto = classify(meta, stem)
    out_dir.mkdir(parents=True, exist_ok=True)

    if proto == "ultralight":
        model = build_ultralight_model(meta)
        steps = ultralight_read_steps(meta, stem)
    elif proto == "felica":
        model = build_felica_model(meta)
        framed = felica_framed_read_steps(meta)
        steps = [(label, rx) for label, _tx, rx in framed]
        _felica_framed = framed
    elif proto == "slix":
        model = build_slix_model(meta, stem)
        framed = slix_framed_steps(meta, stem)
        steps = [(label, rx) for label, _tx, rx in framed]
        _slix_framed = framed
    elif proto == "hal":
        model = build_hal_signal_model(meta, stem)
        steps = hal_signal_read_steps(meta)
    elif proto == "classic":
        model = build_classic_model(meta)
        steps = classic_read_steps(meta)
        tx_steps = classic_read_tx_steps(meta)
    elif proto == "desfire":
        model = build_desfire_model(meta)
        steps = desfire_read_steps(meta)
    else:
        raise ValueError(f"unknown proto {proto}")

    model_path = out_dir / f"{stem}_model.bin"
    read_path = out_dir / f"{stem}_read.inc"
    model_path.write_bytes(model)
    read_path.write_text(read_inc_steps(steps), encoding="utf-8")
    if proto == "classic":
        tx_path = out_dir / f"{stem}_tx.inc"
        tx_path.write_text(tx_inc_steps(tx_steps), encoding="utf-8")
        print(f"Wrote {tx_path} ({len(tx_steps)} steps)", file=sys.stderr)
    if proto == "ultralight":
        emit_ultralight_mock_header(stem, steps, model, out_dir)
    elif proto == "classic":
        emit_classic_mock_header(stem, steps, model, out_dir, tx_steps)
    elif proto == "felica":
        emit_felica_framed_mock_header(stem, _felica_framed, model, out_dir)
        framed_inc = out_dir / f"{stem}_framed.inc"
        framed_lines = ["/* Tier B framed FeliCa TX/RX — one STEP per transceive */"]
        for idx, (label, tx, rx) in enumerate(_felica_framed):
            framed_lines.append(f"/* STEP {idx}: {label} */")
            framed_lines.append("/* TX */")
            framed_lines.append(bytes_to_inc(tx, f"{label} TX").rstrip())
            framed_lines.append("/* RX */")
            framed_lines.append(bytes_to_inc(rx, f"{label} RX").rstrip())
        framed_inc.write_text("\n".join(framed_lines) + "\n", encoding="utf-8")
        print(f"Wrote {framed_inc}", file=sys.stderr)
    elif proto == "slix":
        emit_framed_mock_header("slix", stem, _slix_framed, model, out_dir)
        if stem == "Slix_cap_default":
            parent_model = build_iso15693_model(meta)
            parent_framed = iso15693_framed_steps(meta)
            parent_dir = PROTO_OUT_DIRS["iso15693_3"]
            parent_dir.mkdir(parents=True, exist_ok=True)
            emit_framed_mock_header("iso15693", stem, parent_framed, parent_model, parent_dir)
            parent_read = parent_dir / f"{stem}_read.inc"
            parent_read.write_text(read_inc_steps(framed_steps_to_rx(parent_framed)),
                                   encoding="utf-8")
            parent_model_path = parent_dir / f"{stem}_model.bin"
            parent_model_path.write_bytes(parent_model)
            print(f"Wrote {parent_model_path} ({len(parent_model)} bytes)", file=sys.stderr)
            print(f"Wrote {parent_read} ({len(parent_framed)} steps)", file=sys.stderr)
    elif proto == "desfire":
        emit_desfire_mock_header(stem, steps, model, out_dir)
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
        fixture_card = out_dir / f"{stem}.card.bin"
        fixture_card.write_bytes(card)
        print(f"Wrote {card_path} ({len(card)} bytes)", file=sys.stderr)
        print(f"Wrote {fixture_card} ({len(card)} bytes)", file=sys.stderr)
        print(f"Wrote {card_inc}", file=sys.stderr)
        if proto == "desfire" and stem == "MfDesfire_EV1_sample":
            alias_bin = STORE_DIR / "Desfire.card.bin"
            alias_inc = STORE_DIR / "Desfire_card.inc"
            alias_bin.write_bytes(card)
            alias_inc.write_text(
                "/* Tier E golden: nfc_store envelope for DESFire mock. */\n"
                + model_bin_to_inc(card),
                encoding="utf-8",
            )
            print(f"Wrote {alias_bin} ({len(card)} bytes)", file=sys.stderr)
            print(f"Wrote {alias_inc}", file=sys.stderr)
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
