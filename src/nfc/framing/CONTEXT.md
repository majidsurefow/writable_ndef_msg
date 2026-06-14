# src/nfc/framing — CONTEXT

**Layer:** L0 (listen path) · **Lifecycle:** full

## Purpose

Inbound C-APDU parsing and dispatch shim for the listen path. Receives complete APDU net_bufs from `nfc_transport`, validates ISO 7816-4 structure (CLA/INS/P1/P2/Lc/Le), and forwards to `router/aid_router`. Does NOT own AID matching or service dispatch (delegated to `router/`).

## Key files

| File | Owns |
|------|------|
| `apdu_assembler.c` | `apdu_assembler_init/shutdown`, transport callback impl, APDU parse, stats |
| `apdu_assembler.h` | Public API: init, get_ops, get_config, get_stats, state |
| `apdu_types.h` | Parsed `nfc_apdu_t` struct, SW constants (`NFC_SW_*`), INS codes |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_FRAMING` | `n` | Gate framing layer (depends on `NFC_STACK` + `NFC_ROLE_LISTEN`) |
| `NFC_APDU_EXTENDED_SUPPORT` | `n` | Accept extended-length C-APDUs (3-byte Lc/Le) |

## Deps

- **Upstream (calls down):** `hal/nfc_transport.h` — registers `nfc_transport_ops_t` callbacks
- **Downstream (callback up):** `router/aid_router.h` — `aid_router_dispatch`, `aid_router_field_off`

## Invariants & safety

- **Parse bounds:** `apdu_parse()` rejects < 4 bytes → `APDU_PARSE_REJECT_TOO_SHORT`; length mismatch → `APDU_PARSE_REJECT_LENGTH_MISMATCH`; extended if disabled → `APDU_PARSE_REJECT_EXTENDED_DISABLED`
- **Overflow protection:** Parse failure → `6700 (Wrong Length)` SW response + `apdu_parse_reject_count` stat; no assert
- **Buffer ownership:** `on_apdu` callback owns `net_buf` ref; always `net_buf_unref` after dispatch or reject
- **Stats:** `apdu_assembler_stats_t` tracks `apdu_rx_count`, `apdu_parse_ok_count`, `apdu_parse_reject_count`, `field_on/off_count`, `error_count`
- **State:** `APDU_ASSEMBLER_STATE_UNINITIALIZED/INITIALIZED`; drops APDUs when not initialized

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| (covered by `nfc_apdu_asm` for pure helpers) | A | `NFC_ROLE_LISTEN` | Fragment fits + drop decision |

*Note:* Full APDU parse integration is exercised via virtual loopback in higher-layer protocol tests (NDEF, DESFire, EMV, Aliro Tier C).

## Shell

No shell adapter — framing is internal to the listen stack. Field/APDU events are observable via `nfc_transport stats` (HAL shell).
