# src/nfc/nfc_stack — CONTEXT

**Layer:** L0 (engine) · **Lifecycle:** full

## Purpose

NFC listen stack orchestrator: initializes framing/router/store/transport, registers protocol AIDs, manages listen session state machine. Does NOT own poll/reader discovery (delegated to `reader/`) or the work queue itself (delegated to `run/`).

## Key files

| File | Owns |
|------|------|
| `nfc_stack.c` | Init/start/stop/shutdown, AID registration, APDU→framing dispatch, stats |
| `nfc_stack.h` | Public API: `nfc_stack_init`, `_start`, `_stop`, `_load`, `_save`, state enum |
| `nfc_stack_shell_cmds.c` | L2 shell adapter: `nfc stack init/start/stop/load` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_LISTEN_STACK` | `n` | Gate entire listen orchestrator; `select`s `NFC_FRAMING`, `NFC_ROUTER`, `NFC_PROTOCOL_NDEF` |
| `NFC_STACK_DEFAULT_PROFILE` | 1 | Default `nfc_profile_t` (1 = NDEF) |
| `NFC_LISTEN_STACK_SHELL` | `y` if `SHELL` | Shell commands (`nfc stack init/start/stop/load`) |

## Deps

- **Upstream (calls):** `apdu_assembler` (framing/), `aid_router` (router/), `nfc_transport` (hal/), `nfc_store` (store/)
- **Downstream (called by):** `nfc_applet_emulate`, `nfc_applet_loop`

## Invariants & safety

- **State machine:** `NFC_STACK_STATE_{UNINITIALIZED, INITIALIZED, STARTED, STOPPED, ERROR}`; start requires INITIALIZED or STOPPED; `-ENODEV` otherwise
- **Poll/listen exclusion:** Load/save → `-EBUSY` while `STARTED`; caller must `stop` before serialize
- **UID validation:** `nfc_stack_start(uid)` rejects invalid UID lengths (must be 4/7/10)
- **Error propagation:** Init failure cascades with `goto fail_*` cleanup; stats track `error_count`, `last_error_code`
- **Active tag tracking:** `s_active_tag` holds the loaded slot name for dirty-commit callbacks

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `nfc_reader.store_ram` | E | `NFC_LISTEN_STACK`, `NFC_STORE_RAM` | `nfc_stack_save`/`_load` roundtrip |
| Protocol loopback tests | C | `NFC_LISTEN_STACK` | NDEF/DESFire/EMV/Aliro listen via framing+router |

## Shell

L2 adapter: `nfc_stack_shell_cmds.c` under `NFC_LISTEN_STACK_SHELL`. Exposes `nfc stack init`, `start [uid_hex]`, `stop`, `load <tag>`. On listen-only builds (no `NFC_READER_SHELL`), registers the top-level `nfc` command itself. No business logic duplicated.
