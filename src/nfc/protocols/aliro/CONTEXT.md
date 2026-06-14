# protocols/aliro — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full · **Status: SHIPPED**

## Purpose

Aliro credential protocol: data model (public key, endpoint configuration, auth transcript), serialize/deserialize, poller (reader capture), and ISO-DEP listener (credential emulation). **Partially emulatable**—public keys + config can be emulated; private key never leaves PSA key storage.

## Key files

| File | Owns |
|------|------|
| `aliro.c` | Data model: `aliro_data_t`, pubkey, endpoint config, serialize/deserialize |
| `aliro.h` | Public types, model API, capacity symbols, crypto size constants |
| `aliro_vectors.h` | Deterministic wire vectors (SELECT/AUTH0/AUTH1/EXCHANGE) when unverified |
| `aliro_poller.c` | Reader poller: SELECT → AUTH0 → AUTH1 → EXCHANGE observe |
| `aliro_poller.h` | Poller API: `aliro_poller_read` |
| `aliro_listener.c` | ISO-DEP listener: AUTH0/AUTH1/EXCHANGE deferred crypto work |
| `aliro_listener.h` | Listener API: `aliro_listener_load`, `aliro_listener_on_apdu` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_ALIRO` | n | Gate model + poller + listener |
| `NFC_ALIRO_PROTOCOL_VERIFIED` | n | **n (default):** deterministic vectors in `aliro_vectors.h` for CI/QEMU. **y:** PSA ECDH/HKDF/ECDSA/AES-GCM against Aliro 0.9.4 field order (HIL sign-off) |
| `NFC_ALIRO_MAX_TRANSCRIPT` | 512 | Max public AUTH transcript bytes |

## Crypto path (production vs CI)

| Build | AUTH0/AUTH1/EXCHANGE crypto | Source |
|-------|----------------------------|--------|
| `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=n` (default) | Deterministic test vectors | `aliro_vectors.h`; mirrored by `tests/common/nfc_aliro_crypto_shim.c` |
| `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=y` | PSA Crypto (CRACEN on nRF54) | Direct PSA API; credential key in `NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE` |

Private key is never serialized. Unverified builds ship full AUTH chain + EXCHANGE FSM with vector parity; verified builds gate on spec field-order sign-off.

## Deps

- **Upstream (calls):** PSA Crypto when `NFC_ALIRO_PROTOCOL_VERIFIED=y`
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_router` (AID dispatch—two AIDs), `nfc_store`

## Invariants & safety

- **Buffers:** Transcript size validated against `NFC_ALIRO_MAX_TRANSCRIPT`
- **PSA keys:** Private key stored in PSA persistent storage; never serialized to blob
- **Error propagation:** Crypto failure → `-EINVAL`; key not provisioned → `-ENOKEY`

## Roles

- **Poller:** `aliro_poller.c` (Tier B) — reader ISO-DEP SELECT + AUTH chain + EXCHANGE observe
- **Listener:** `aliro_listener.c` (Tier C) — ISO-DEP credential emulation through DONE

## Wire/spec

- Aliro specification v0.9.4 (CCC/CSA); NFC expedited transaction
- APDU catalog: [`docs/nfc/archive/waves/wave5-aliro.md`](../../../docs/nfc/archive/waves/wave5-aliro.md) §8
- **Emulatable:** Yes (public key + config; private key via PSA, never in blob)
- Step-up AID (`A0 00 00 08 58 01 01 02 00`) → SW `6A81` (function not supported)

## Capacity symbols

| CONFIG_NFC_ALIRO_* | Default | Bound it protects |
|--------------------|---------|-------------------|
| `MAX_TRANSCRIPT` | 512 | AUTH transcript buffer |

## Fixtures ↔ goldens

- **Flipper:** No Flipper Aliro fixture (proprietary — permanent N/A)
- **Synthetic:** `tests/fixtures/aliro/` — `AUTH0/AUTH1/EXCHANGE_*.inc`, `Aliro_mock.h`
- **Store envelope:** `tests/fixtures/store/Aliro.card.bin`, `Aliro_card.inc` via `scripts/nfc/protocol_to_card_bin.py --aliro`
- **Protocol copy:** `tests/fixtures/aliro/Aliro.card.bin`
- **Loopback:** `test_virtual_loopback_aliro` + `test_virtual_loopback_aliro_auth_chain` with `aliro_compare_auth_chain`

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_aliro.model` | A (model) | `NFC_PROTOCOL_ALIRO` | Serialize/deserialize, pubkey format |
| `sample.nfc.unit.nfc_aliro.poller` | B (poller) | `+NFC_ALIRO_TEST_TIER_POLLER` | SELECT + AUTH0/AUTH1/EXCHANGE TX + transcript |
| `sample.nfc.unit.nfc_aliro.listener` | C (listener) | `+NFC_ALIRO_TEST_TIER_LISTENER` | SELECT, step-up 6A81, AUTH0/AUTH1/EXCHANGE FSM |
| `nfc_reader_aliro_store` | E (store) | `NFC_PROTOCOL_ALIRO` | Golden `.card.bin` load/save |
| `nfc_reader_loopback.test_virtual_loopback_aliro*` | E+ | `CONFIG_NFC_TEST_LOOPBACK` | Deep compare poller↔listener AUTH+EXCHANGE |

**Tier counts:** 3 configs, 32+ cases (model 4 + poller 4 + listener 15+).
**Twister dir:** `tests/unit/nfc_aliro`

## Live HIL

- **Tag/credential:** Aliro-provisioned device
- **Golden:** Reader-captured `.card` or provisioned credential
- **Overlay:** `overlay-pn7160-stack.conf` (reader); `+overlay-pn7160-listen.conf` (emulate)
- **Build:** Set `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=y` for PSA crypto against real reader
- **Shell sequence:**
  ```
  nfc read tag1 → nfc stack stop → nfc emulate tag1
  nfc_transport stats  # apdu_assembled > 0
  ```
- **PASS criteria:** ISO-DEP AUTH0/AUTH1/EXCHANGE transcript exchanged; listener reaches DONE

## Door-lock reference

**Golden reference:** NCS door-lock repo (`ncs-door-lock-and-access-control`) — **not Flipper** (no Aliro dumps). B validates **reader role** via `lib/aliro/` + RFAL; **card role** via external Test Harness only (no in-repo NFCT listener).

| Tier | Status | Proof |
|------|--------|-------|
| **CI SHIPPED** | **PASS** | [`docs/nfc/ALIRO_DOOR_LOCK_SHIP_CHECK.md`](../../../docs/nfc/ALIRO_DOOR_LOCK_SHIP_CHECK.md) — 31/31 `nfc_aliro` + Aliro store/loopback in `nfc_reader` |
| **Door-lock interop VERIFIED** | **PENDING** | PSA (`CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=y`), LOAD_CERT, step-up, kpersistent, Test Harness PICS — see ship-check §VERIFIED backlog |

Expedited SELECT/AUTH0/AUTH1/EXCHANGE wire shapes match B/spec 0.9.4; deferred crypto timing matches B `aliro_work` pattern. Gaps (LOAD_CERT, kpersistent, full step-up, PSA default-off) are documented — not CI blockers.

## NCS Door-Lock / Access-Control Parity

Aliro listener implements the **same protocol** as the NCS `aliro/` tree (Aliro 0.9.4 expedited transaction), but as a **reimplemented protocol module** under the NFC stack architecture (see `docs/nfc/archive/waves/wave5-aliro.md`). Key differences:

| Aspect | NCS `aliro/` tree | This protocol module |
|--------|-------------------|---------------------|
| Language | C++ | C (MISRA-compliant) |
| Crypto | C++ PSA wrappers | PSA when verified; `aliro_vectors.h` when unverified |
| Key storage | kpersistent_manager | PSA persistent keys (`NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE`) |
| Work queue | `aliro_work/` C++ | `nfc_stack_wq` (HAL-owned, C) |
| Build gate | `DOOR_LOCK_ALIRO_*` | `NFC_PROTOCOL_ALIRO` |
| License | GPL-derived reference | Clean-room reimplementation |
| Golden sources | NCS integration captures | wave5 synthetic fixtures; not Flipper |

**NCS parity deltas (read-only audit):** AUTH0/AUTH1/EXCHANGE deferred crypto timing matches NCS `aliro_work` pattern; expedited SELECT uses `00 00` prefix without `90 00` trailer (virtual loopback / PICC path).

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. Aliro via `nfc read`, `nfc emulate`, `nfc loop`.
