# src/nfc/reader — CONTEXT

**Layer:** L0 (engine) · **Lifecycle:** full

## Purpose

Reader discovery engine: async tag scan on `nfc_stack_wq`, session transceive delegate, poller registry walk for detect/clone. Does NOT own listen orchestration (delegated to `nfc_stack/`) or envelope persistence (delegated to `store/`).

## Key files

| File | Owns |
|------|------|
| `nfc_reader_engine.c` | Scan/read/clone work handlers, session state, `-EBUSY` guards |
| `nfc_reader_engine.h` | Public API: `nfc_reader_scan_start`, `_read_start`, `_session_transceive` |
| `nfc_reader_poller_registry.c` | Protocol-to-poller dispatch table, detect/clone hooks |
| `nfc_reader_poller_registry.h` | `nfc_reader_poller_entry_t` table type, `nfc_reader_pollers_run` |
| `nfc_reader_shell_cmds.c` | L2 shell adapter: `nfc reader scan`, `nfc reader detect`, top-level `nfc` registration |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_READER` | `n` | Gate entire reader engine |
| `NFC_READER_SHELL` | `y` if `SHELL` | Shell commands (`nfc reader scan/detect`) |
| `NFC_READER_SCAN_TIMEOUT_MS` | 10000 | Default discovery timeout |

## Deps

- **Upstream (calls):** `nfc_transport` (HAL), `nfc_stack_wq` (run/), protocols via poller registry
- **Downstream (called by):** `nfc_applet_read`, `nfc_applet_verify`, `nfc_applet_scan`

## Invariants & safety

- **Session exclusion:** `scan_busy`, `read_busy`, `clone_busy` atomics guard single-flight; concurrent scan/read/clone → `-EBUSY`
- **Buffers:** Tag name copied to static `s_read_tag[CONFIG_NFC_STORE_MAX_TAG_LEN]`; length validated before strncpy
- **Work queue:** All blocking HAL calls (`discover_wait`, `transceive`) run on `nfc_stack_wq`, never ISR
- **Session lifetime:** `nfc_reader_session_end()` calls `nfc_transport_discover_stop()` + clears `s_session`; pollers must not hold stale session pointers

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `nfc_reader.store` | E | `NFC_READER`, `NFC_STORE` | 9-protocol roundtrip via poller registry (QEMU) |
| `nfc_reader.store_ram` | E | + `NFC_STORE_RAM` | RAM slot save/load roundtrip |
| `test_poller_registry.c` | E | `NFC_READER` | Poller detect/clone hook dispatch |
| `test_verify_diff.c` | E | `NFC_READER` | Diff logic for `nfc_applet_verify` |

## Shell

L2 adapter: `nfc_reader_shell_cmds.c` under `NFC_READER_SHELL`. Registers top-level `nfc` command + `nfc reader scan`, `nfc reader detect`. Product commands (`nfc read`, `nfc check`, `nfc loop`) are extern-linked from `nfc_applet_shell_cmds.c`. No business logic duplicated.
