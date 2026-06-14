# src/nfc/router — CONTEXT

**Layer:** L0 (listen path) · **Lifecycle:** full

## Purpose

ISO-DEP SELECT-by-AID dispatch router for the listen path. Maintains a table of registered `nfc_service_t` vtables keyed by AID; on SELECT APDU, activates the matching service and forwards subsequent APDUs until deselect or field-off. Does NOT own APDU parsing (delegated to `framing/`) or service implementation (protocol-specific).

## Key files

| File | Owns |
|------|------|
| `aid_router.c` | `aid_router_init/shutdown/register/clear/dispatch/field_off`, AID lookup, stats |
| `aid_router.h` | Public API: init, register, dispatch, get_config/stats/state |
| `service.h` | `nfc_service_t` vtable (on_select/on_apdu/on_deselect/on_field_off/serialize/deserialize) + persist IDs |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_ROUTER` | `n` | Gate AID router (depends on `NFC_STACK` + `NFC_ROLE_LISTEN`) |
| `NFC_ROUTER_MAX_AIDS` | 8 | Maximum registered AID entries (1–32) |

## Deps

- **Upstream (calls down):** `framing/apdu_types.h` (parsed APDU view), `hal/nfc_transport.h` (`send_response`)
- **Downstream (callback up):** Protocol services implement `nfc_service_t` (NDEF, DESFire, EMV, Aliro, Ultralight)

## Invariants & safety

- **Table bounds:** `s_table[CONFIG_NFC_ROUTER_MAX_AIDS]` static array; register returns `-ENOMEM` + bumps `register_table_full_count` when full
- **AID validation:** `aid_router_register` rejects NULL, `aid_len == 0`, or `aid_len > NFC_AID_MAX_LEN (16)` → `-EINVAL`
- **Dispatch rules:** SELECT-by-AID (`CLA=00 INS=A4 P1=04`) activates service; unmatched → `6A82 (File Not Found)`; non-SELECT with no active service → `6986 (No EF selected)`
- **Deselect:** On service switch, calls `on_deselect` on previous service before `on_select` on new
- **Field-off:** `aid_router_field_off` calls active service's `on_field_off` and clears `s_active_svc`
- **Stats:** `aid_router_stats_t` tracks `select_matched_count`, `select_unmatched_count`, `apdu_routed_count`, `apdu_no_service_count`, `field_off_count`, `error_count`
- **State:** `AID_ROUTER_STATE_UNINITIALIZED/INITIALIZED`; returns `-ENODEV` when not initialized

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| (implicit via protocol Tier C) | C | `NFC_ROUTER=y` | SELECT dispatch, field-off |

*Note:* Router is exercised through virtual loopback in emulatable protocol tests (NDEF, DESFire, EMV, Aliro). Direct unit test suite deferred to P8 test harmonization.

## Shell

No shell adapter — router is internal to the listen stack. Dispatch events are indirectly observable via protocol service stats and `nfc_transport stats`.
