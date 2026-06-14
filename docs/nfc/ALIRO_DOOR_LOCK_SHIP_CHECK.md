# Aliro Door-Lock Ship Check

**Date:** 2026-06-14  
**Repo A (NFC stack):** `/Users/majidfaroud/writable_ndef_msg` · branch `nfc-stack` · HEAD `597248254b3f356e023e479329d634f1afe74861`  
**Repo B (door-lock reference):** `/Users/majidfaroud/ncs-door-lock-and-access-control` · reader role via `lib/aliro/` + RFAL; card role via external Test Harness only  
**Gate format:** [`PROTOCOL_DONE_WORKFLOW.md`](PROTOCOL_DONE_WORKFLOW.md) (Phase R→V→D), golden reference = door-lock (not Flipper)

---

## Ship verdict

| Tier | Verdict | Evidence |
|------|---------|----------|
| **CI SHIPPED** | **PASS** | `nfc_aliro` 31/31 + `nfc_reader` 139/139 (Aliro store ×2, loopback ×2); profile builds include `aliro_poller.c` + `aliro_listener.c` |
| **Door-lock interop VERIFIED** | **PENDING** | PSA crypto (`CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=y`), LOAD_CERT, step-up, kpersistent — backlog; see §Cross-repo parity |

---

## Reader role (PN7160 / poller — parity with B `libaliro` expedited NFC path)

| Gate | Status | Evidence |
|------|--------|----------|
| Expedited AID matches B / spec 0.9.4 | **PASS** | A: `aliro_expedited_aid` `A0 00 00 08 58 01 01 01 00` — `src/nfc/protocols/aliro/aliro.c:13-15`; B: `wave5-aliro.md` §1.4 / door-lock PICS |
| Step-up AID recognized (reader detect) | **PASS** | A poller uses expedited AID only (`aliro_poller.c:85-86`); step-up not selected on reader path (matches B expedited-only poller scope) |
| SELECT APDU shape (ISO-DEP) | **PASS** | `00 A4 04 00 09 <expedited AID>` — `aliro_poller.c:85-86`, `106-107` |
| AUTH0 TX shape (CLA/INS/Lc/data) | **PASS** | `80 80 00 00 51` + 81 B (nonce 16 + pubkey 65) — `aliro_vectors.h:47-58`; INS `0x80` per `aliro.h:26` |
| AUTH1 TX shape | **PASS** | `80 81 00 00 40` + 64 B signature — `aliro_vectors.h:72-80`; INS `0x81` |
| EXCHANGE TX shape | **PASS** | `80 82 00 00 1D` + 29 B (GCM nonce+pt+tag) — `aliro_vectors.h:91-99`; INS `0x82` |
| SELECT response (expedited PICC) | **PASS** | `00 00 \|\| nonce(16)` — no `90 00` trailer — `aliro_vectors.h:39-44`; poller accepts — `aliro_poller.c:25-28` |
| Poller FSM completes on vector card | **PASS** | 4 TX (SELECT→AUTH0→AUTH1→EXCHANGE) — `test_poller.c:32-40`; mock `Aliro_mock.h:41-46` |
| Store roundtrip | **PASS** | `test_store_aliro_roundtrip.c:63-97` — golden `Aliro.card.bin` |
| E+ loopback + `aliro_compare_auth_chain` | **PASS** | `test_virtual_loopback.c:1028-1081` — transcript + pubkey deep compare |
| Tier B unit tests | **PASS** | `sample.nfc.unit.nfc_aliro.poller` — 8/8 cases (Twister 2026-06-14) |
| Tier A model tests | **PASS** | `sample.nfc.unit.nfc_aliro.model` — 4/4 cases |

---

## Card role (NFCT / listener — A has this; B validates reader side only via Test Harness)

| Gate | Status | Evidence |
|------|--------|----------|
| `overlay-nfct-stack.conf` + `NFC_PROFILE_CARD_EMULATION` builds with Aliro listener | **PASS** | `overlay-nfct-stack.conf:19-23`; `.p9_profiles.log` NFCT CE build links `aliro_listener.c` (obj 51/340) |
| Listener registered under CE profile | **PASS** | `src/nfc/Kconfig:56` `imply NFC_PROTOCOL_ALIRO`; `confs/layer3-protocols.conf` |
| Listener FSM: SELECT → AUTH0 → AUTH1 → EXCHANGE | **PASS** | `aliro_listener.c:124-145` (SELECT), `168-186` (AUTH0/AUTH1 work deferral), `189-210` (EXCHANGE) |
| Unverified build uses vectors (`verified=n`) | **PASS** | `#if !IS_ENABLED(CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED)` — `aliro_listener.c:50-57`, `65-75`, `89-106` |
| Step-up SELECT → `6A81` | **PASS** | `aliro_listener.c:139-141`; `test_listener.c:51-56` |
| Tier C listener tests | **PASS** | `sample.nfc.unit.nfc_aliro.listener` — 19/19 cases |
| B card-role in-repo | **N/A** | B has no NFCT credential listener; Test Harness emulates User Device (`docs/testing/setting_up_test_harness.rst`) |

**Product note:** Card emulation is **Repo A's NFCT product path** (`overlay-nfct-stack.conf`). Repo B validates **reader side only** against Test Harness (synthetic User Device).

---

## Cross-repo parity table (A vs B door-lock)

| Aliro phase / capability | A (NFC stack) | B (door-lock) | Parity |
|--------------------------|---------------|---------------|--------|
| Expedited AID + SELECT/AUTH0/AUTH1/EXCHANGE wire | Poller + listener FSM, vector/PSA-gated | `lib/aliro/` + `AccessManager` via RFAL (`app/src/aliro/platform/nfc/`) | **MATCH** (expedited standard) |
| Deferred crypto work queue | `nfc_transport_submit_work` → `nfc_stack_wq` | `AliroWorkSubmit` → `aliro_work/` (`app/src/aliro/aliro_work/aliro_work.cpp`) | **MATCH** (pattern) |
| Step-up phase (`…01 02 00` AID) | Listener stub `6A81`; poller N/A | `kFeatureStepUpPhaseSupported` — full stack (`lib/aliro/include/aliro/aliro.h:29`) | **PARTIAL** |
| LOAD_CERT (reader cert after AUTH0) | Not implemented | Provisioned cert path (`docs/testing/testing_certificate_reader.rst`) | **GAP** |
| Kpersistent / expedited-fast | Not implemented | `KpersistentManager` (`app/src/aliro/kpersistent_manager/`) + `kKpersistentRangeBegin` (`storage/psa_key_ids.h:32-34`) | **GAP** |
| PSA crypto (ECDH/HKDF/ECDSA/AES-GCM) | Opt-in `CONFIG_NFC_ALIRO_PROTOCOL_VERIFIED=y`; default vectors | `interface_impl/crypto.cpp` + `platform/crypto/utils.h` | **PARTIAL** |
| Key storage | `NFC_ALIRO_PSA_KEY_CREDENTIAL_PRIVATE` (Kconfig range) | `DOOR_LOCK_ALIRO_PSA_KEY_ID_RANGE_BASE` + kpersistent slots (`storage/psa_key_ids.h`) | **PARTIAL** |
| BLE/UWB ranging | N/A (NFC-only module) | `CONFIG_NCS_ALIRO_BLE_UWB` | **N/A** |
| Test Harness PICS conformance | Not run in CI | `docs/testing/verification_and_testing.rst` | **GAP** (VERIFIED tier) |

### Verification tiers

| Tier | Scope | Status |
|------|-------|--------|
| **CI SHIPPED** | Synthetic vectors, QEMU unit + virtual loopback, profile compile | **PASS** — 31/31 `nfc_aliro` + Aliro cases in `nfc_reader` |
| **Door-lock interop VERIFIED** | PSA verified build + PN7160 reader ↔ Test Harness card + optional NFCT HIL | **PENDING** — not required for CI SHIPPED; documented backlog below |

---

## VERIFIED tier backlog (pointers to B — do not port unless blocking twister)

| Item | B reference | A status |
|------|-------------|----------|
| PSA wire crypto | `app/src/aliro/interface_impl/crypto.cpp` | Stub returns `6985` when `VERIFIED=y` without PSA port — `aliro_listener.c:52-56` |
| Kpersistent manager | `app/src/aliro/kpersistent_manager/kpersistent_manager.h` | Not in A |
| LOAD_CERT + cert chain | `docs/testing/testing_certificate_reader.rst` | Not in A |
| Step-up full FSM | `lib/aliro/include/aliro/aliro.h` (`kFeatureStepUpPhaseSupported`) | Stub `6A81` only |
| Test Harness PICS | `docs/testing/verification_and_testing.rst` | Manual HIL |

---

## Twister & profile verification (2026-06-14)

```bash
export PATH="/opt/nordic/ncs/toolchains/ef4fc6722e/bin:$PATH"
export ZEPHYR_BASE="/opt/nordic/ncs/v3.2.4/zephyr"
export ZEPHYR_SDK_INSTALL_DIR="/opt/nordic/ncs/toolchains/ef4fc6722e/opt/zephyr-sdk"

west twister -T tests/unit/nfc_aliro -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v
# → 3/3 configs, 31/31 cases PASS

west twister -T tests/unit/nfc_reader -p qemu_cortex_m3 --no-sysbuild -v
# → 3/3 configs, 139/139 cases PASS (includes aliro store ×2, loopback ×2)
```

Profile builds (not re-run this gate — prior `.p9_profiles.log` green, `exit=0`):

| Profile | Overlay | Aliro objects |
|---------|---------|---------------|
| PN7160 reader | `overlay-pn7160-stack.conf` | `aliro.c`, `aliro_poller.c`, `aliro_listener.c` |
| NFCT CE | `overlay-nfct-stack.conf` | `aliro.c`, `aliro_listener.c` (+ poller for store/applets) |

---

## Related docs

- [`src/nfc/protocols/aliro/CONTEXT.md`](../../src/nfc/protocols/aliro/CONTEXT.md) — protocol CONTEXT + door-lock reference
- [`docs/nfc/archive/waves/wave5-aliro.md`](archive/waves/wave5-aliro.md) — APDU catalog §8
- [`docs/nfc/PROTOCOL_SHIP_STATUS.md`](PROTOCOL_SHIP_STATUS.md) — program ship matrix
