# src/nfc/hal — CONTEXT

**Layer:** L0 (HAL abstraction) · **Lifecycle:** full

## Purpose

Backend-neutral NFC transport HAL: provides a unified API (`nfc_transport_*`) for both PN7160 (reader+listen) and NRFX NFCT (listen-only) backends. Owns the inbound APDU net_buf pool and assembly helpers for ISR→WQ handoff in the listen path. Does NOT own APDU parsing or routing (delegated to `framing/` + `router/`).

## Key files

| File | Owns |
|------|------|
| `nfc_transport.h` | Public transport API: init, start/stop, discover, tag_transceive, send_response, callbacks |
| `nfc_transport_pn7160.c` | PN7160 backend adapter (reader poll + listen) |
| `nfc_transport_nrfx.c` | NFCT nrfxlib backend adapter (listen-only; poll returns `-ENOTSUP`) |
| `nfc_apdu_pool.c` / `.h` | ISR-safe FIXED net_buf pool (`NFC_APDU_POOL_COUNT` × `NFC_APDU_BUF_SIZE`) |
| `nfc_apdu_asm.c` / `.h` | Pure helpers: `nfc_apdu_frag_fits`, `nfc_apdu_asm_on_frag` (oversize drop decision) |
| `nfc_transport_shell_cmds.c` | Shell adapter (L2 — `NFC_HAL_SHELL`) |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_HAL_BACKEND_PN7160` | auto if PN7160 | Select PN7160 backend; sets `NFC_HAL_BACKEND_HAS_READER` |
| `NFC_HAL_BACKEND_NRFX` | auto if NFCT | Select NFCT backend (listen-only) |
| `NFC_ROLE_READER` | `y` if HAS_READER | Enable poll sub-API (`discover_start/stop/wait`, `tag_transceive`) |
| `NFC_ROLE_LISTEN` | `y` | Enable listen sub-API; selects `NET_BUF` |
| `NFC_APDU_BUF_SIZE` | 512 | Data bytes per APDU net_buf |
| `NFC_APDU_POOL_COUNT` | 4 | Number of APDU net_bufs (ISR assembly) |
| `NFC_HAL_SHELL` | `y` if SHELL | Shell commands (`nfc_transport config/stats/state`) |

## Deps

- **Upstream (calls):** `modules/nfc_pn7160` (PN7160 backend) or nrfxlib NFC T4T (NFCT backend)
- **Downstream (callback up):** `nfc_transport_ops_t` callbacks → `framing/apdu_assembler` → `router/aid_router`

## Invariants & safety

- **Buffer ownership:** `nfc_apdu_pool` is FIXED (ISR-safe); callee owns `net_buf` ref after `on_apdu` callback; caller must `net_buf_unref` after dispatch
- **Overflow protection:** `nfc_apdu_frag_fits` + `nfc_apdu_asm_on_frag` check fragment fit before `net_buf_add_mem`; oversize → `NFC_APDU_ASM_DROP_OVERSIZE` + `apdu_oversized_count` stat
- **Stats counters:** `nfc_transport_stats_t` tracks `frag_dropped_no_buffer`, `apdu_oversized_count`, `apdu_dropped_no_consumer`, `error_count` (named counters per CONVENTIONS §6)
- **Session:** `nfc_transport_state_t` enum (UNINITIALIZED/INITIALIZED/STARTED/STOPPED); `-EBUSY` implied when ops called in wrong state
- **Response limit:** `NFC_TRANSPORT_MAX_RESPONSE_LEN = 255` (PN7160 TML); NFCT backend `BUILD_ASSERT` equivalence

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_apdu_asm` | A (model) | `NFC_ROLE_LISTEN=y` | `frag_fits` boundary cases, oversize drop decision (4 cases) |

## Shell

L2 adapter: `nfc_transport_shell_cmds.c` under `NFC_HAL_SHELL`. Top-level `nfc_transport` command with subcommands: `config` (print FWI), `stats` (HIL acceptance counters), `state` (roles/tier/tech). No business logic — each reads via `nfc_transport_get_*` and prints.
