# protocols/aliro — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

Aliro credential protocol: data model (public key, endpoint configuration, auth transcript), serialize/deserialize, poller (reader capture), and ISO-DEP listener (credential emulation). **Partially emulatable**—public keys + config can be emulated; private key never leaves PSA key storage.

## Key files

| File | Owns |
|------|------|
| `aliro.c` | Data model: `aliro_t` struct, pubkey, endpoint config, serialize/deserialize |
| `aliro.h` | Public types, model API, capacity symbols, crypto size constants |
| `aliro_poller.c` | Reader poller: Aliro AUTH sequence capture |
| `aliro_poller.h` | Poller API: `aliro_poller_read` |
| `aliro_listener.c` | ISO-DEP listener: responds to AUTH0/AUTH1/EXCHANGE APDUs |
| `aliro_listener.h` | Listener API: `aliro_listener_load`, `aliro_listener_on_apdu` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_ALIRO` | n | Gate model + poller + listener |
| `NFC_ALIRO_PROTOCOL_VERIFIED` | n | Gate hardware-facing AUTH field order checks |
| `NFC_ALIRO_MAX_TRANSCRIPT` | 512 | Max public AUTH transcript bytes |

## Deps

- **Upstream (calls):** PSA Crypto (for key storage, ECDH, ECDSA)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_router` (AID dispatch—two AIDs), `nfc_store`

## Invariants & safety

- **Buffers:** Transcript size validated against `NFC_ALIRO_MAX_TRANSCRIPT`
- **PSA keys:** Private key stored in PSA persistent storage; never serialized to blob
- **Error propagation:** Crypto failure → `-EINVAL`; key not provisioned → `-ENOKEY`

## Roles

- **Poller:** `aliro_poller.c` (Tier B) — reader ISO-DEP
- **Listener:** `aliro_listener.c` (Tier C) — ISO-DEP credential emulation

## Wire/spec

- Aliro specification v0.9.4 (CCC/CSA); NFC expedited transaction
- **Emulatable:** Yes (public key + config; private key via PSA, never in blob)

## Capacity symbols

| CONFIG_NFC_ALIRO_* | Default | Bound it protects |
|--------------------|---------|-------------------|
| `MAX_TRANSCRIPT` | 512 | AUTH transcript buffer |

## Fixtures ↔ goldens

- **Flipper:** No Flipper Aliro fixture (proprietary)
- **Generated:** Reader-captured `.card` (pubkey + config only)
- **Loopback:** `tests/common/nfc_virtual_loopback` (poller↔listener)

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_aliro.model` | A (model) | `NFC_PROTOCOL_ALIRO` | Serialize/deserialize, pubkey format |
| `sample.nfc.unit.nfc_aliro.poller` | B (poller) | `+NFC_ALIRO_TEST_TIER_POLLER` | Poller AUTH capture sequence |
| `sample.nfc.unit.nfc_aliro.listener` | C (listener) | `+NFC_ALIRO_TEST_TIER_LISTENER` | Listener AUTH0/AUTH1/EXCHANGE responses |

**Tier counts:** 3 configs, 19 cases (model + poller + listener).
**Twister dir:** `tests/unit/nfc_aliro`

## Live HIL

- **Tag/credential:** Aliro-provisioned device
- **Golden:** Reader-captured `.card` or provisioned credential
- **Overlay:** `overlay-pn7160-stack.conf` (reader); `+overlay-pn7160-listen.conf` (emulate)
- **Shell sequence:**
  ```
  nfc read tag1 → nfc stack stop → nfc emulate tag1
  nfc_transport stats  # apdu_assembled > 0
  ```
- **PASS criteria:** ISO-DEP AUTH transcript exchanged; listener responds to AUTH0/AUTH1

## NCS Door-Lock / Access-Control Parity

Aliro listener implements the **same protocol** as the NCS `aliro/` tree (Aliro 0.9.4 expedited transaction), but as a **reimplemented protocol module** under the NFC stack architecture (see `docs/nfc/archive/waves/wave5-aliro.md`). Key differences:

| Aspect | NCS `aliro/` tree | This protocol module |
|--------|-------------------|---------------------|
| Language | C++ | C (MISRA-compliant) |
| Crypto | C++ PSA wrappers | Direct PSA Crypto API |
| Key storage | kpersistent_manager | PSA persistent keys (`NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE`) |
| Work queue | `aliro_work/` C++ | `nfc_stack_wq` (HAL-owned, C) |
| Build gate | `DOOR_LOCK_ALIRO_*` | `NFC_PROTOCOL_ALIRO` |
| License | GPL-derived reference | Clean-room reimplementation |

**Intent parity:** Both implement the Aliro 0.9.4 NFC expedited AUTH flow (AUTH0 → AUTH1 → optional EXCHANGE). This module is **not a GPL port**—it is a behavioral reference implementation for the NFC stack's listener architecture.

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. Aliro via `nfc read`, `nfc emulate`, `nfc loop`.
