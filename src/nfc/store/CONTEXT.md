# src/nfc/store — CONTEXT

**Layer:** L0 (engine) · **Lifecycle:** full

## Purpose

NFC card envelope: versioned TLV + CRC16 blob serialization, pluggable save/load backends (RAM, future NVS). Does NOT own protocol listeners (they provide `serialize`/`deserialize` vtables) or shell rendering (Phase A made default save inert).

## Key files

| File | Owns |
|------|------|
| `nfc_store.c` | Envelope format (magic/version/CRC), serialize/deserialize loop, quiescent check |
| `nfc_store.h` | Public API: `nfc_store_save`, `_load`, `_import_blob`, `_peek_entry_meta`, callbacks |
| `nfc_store_ram.c` | RAM slot backend: `nfc_store_ram_save_cb`, `_load_cb`, `nfc_store_ram_import` |
| `nfc_store_ram.h` | RAM backend API |
| `nfc_store_golden.c` | Compiled-in golden blobs for `nfc emulate golden` |
| `nfc_store_shell_cmds.c` | L2 shell adapter: `nfc store list/dump/import/save/load` |
| `nfc_persist_name.c` | `nfc_persist_id_name()` for human-readable protocol names |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_STORE` | `n` | Gate store core (`nfc_store.c`, `nfc_persist_name.c`) |
| `NFC_STORE_BLOB_SIZE` | 4096 | Staging buffer size |
| `NFC_STORE_MAX_TAG_LEN` | 16 | Max slot name length including NUL |
| `NFC_STORE_RAM` | `n` | Enable RAM slot backend |
| `NFC_STORE_RAM_SLOT_COUNT` | 4 | Number of RAM slots |
| `NFC_STORE_RAM_SHELL` | `y` if `SHELL` | Shell commands for RAM store |
| `NFC_STORE_GOLDENS` | `y` if `NFC_STORE_RAM` | Compiled-in golden blobs |

## Deps

- **Upstream (calls):** Protocol listeners (`nfc_service_t.serialize`/`deserialize`), Zephyr CRC (`crc16_ccitt`)
- **Downstream (called by):** `reader/` (poller clone), `nfc_stack/` (`_save`, `_load`), `nfc_applet_read`, `nfc_applet_emulate`

## Invariants & safety

- **Envelope format:** Magic `0x4E 0x46` ("NF"), version byte (V1=0x01, V2=0x02), 16-bit payload length, entries, CRC16-CCITT
- **Entry format (V2):** `[persist_id:1][flags:1][len:2][body:len]`; V1 lacks flags byte
- **Quiescent check:** `nfc_store_save`/`_load` → `-EBUSY` while `nfc_stack` is `STARTED` (listen active)
- **Default save inert:** `nfc_store_default_save()` returns `-ENODEV`—no shell symbols in `nfc_store.c` (Phase A fix)
- **Shell include gated:** `nfc_store_ram.c` includes `<shell/shell.h>` only inside `#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)` (Phase A fix)
- **Buffer bounds:** Staging/serialize buffers are `CONFIG_NFC_STORE_BLOB_SIZE`; overflow → `-ENOMEM` with `error_count` stat
- **CRC validation:** `nfc_store_read_envelope` and `nfc_store_validate_blob_buf` reject mismatched CRC → `-EBADMSG`

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `nfc_reader.store` | E | `NFC_STORE` | Roundtrip serialize→save→load→deserialize for 9 protocols |
| `nfc_reader.store_ram` | E | `NFC_STORE_RAM` | RAM backend slot management |
| `test_store_import.c` | E | `NFC_STORE` | `nfc_store_import_blob` validates CRC, rejects corrupt |

## Shell

L2 adapter: `nfc_store_shell_cmds.c` under `NFC_STORE_RAM_SHELL`. Exposes `nfc store list`, `dump <slot>`, `import <slot> <hex>`, `import golden <name>`, `save <slot>`, `load <slot>`. The `@@NFCDUMP@@` hex-dump save callback is registered by `nfc_applet_shell_cmds.c`, not here—the store core is headless.
