# NFC Harmonization Findings

Rollup of findings from program phases P1–P7.  
Format: `severity · file:line · category(D.4) · description · fix status`

---

## Phase A — Applets + Shell Decoupling (LANDED)

**Status:** DONE 2026-06-14  
**Commits:** 32b9ad1 → ca54ea7  
**Baseline:** 97/97 twister tests on `qemu_cortex_m3`

### Locked artifacts
- L1 applet API: `nfc_applet_service.h` — no `struct shell *`
- L2 shell: `*_shell_cmds.c` only
- Commands: `scan start/stop`, `read`, `emulate`/`golden`, `loop`, `check` (DK)
- DROPPED: `reader clone`, product `verify`

---

## P1 — Kconfig Truth Audit

**Status:** DONE  
**Date:** 2026-06-14  
**Exit gate:** `west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild` → **PASS** (2/2 configs, 97/97 cases)

### KCONFIG_TRUTH_FINDINGS

#### 1. Profile → Symbol Table (verified via `.config` dumps)

| Profile | What it implies |
|---------|-----------------|
| `NFC_PROFILE_READER` | `NFC_READER`, `NFC_STORE`, `NFC_STORE_RAM`, `NFC_APPLETS`, **all 9 protocols** (NDEF, Ultralight, Classic, FeliCa, ISO15693-3, SLIX, DESFire, EMV, Aliro) |
| `NFC_PROFILE_CARD_EMULATION` | `NFC_LISTEN_STACK` (→ selects `NFC_FRAMING`, `NFC_ROUTER`, `NFC_PROTOCOL_NDEF`), `NFC_STORE`, `NFC_STORE_RAM`, **5 emulatable protocols** (NDEF, Ultralight, DESFire, EMV, Aliro) |
| `NFC_PROFILE_LAB` | nothing — HAL scaffold only (roles compile, no engine/store/applets/protocols) |

**Verified builds (qemu_cortex_m3):**

| Overlay(s) | Profile | Symbols confirmed |
|------------|---------|-------------------|
| `overlay-pn7160-stack.conf` | READER | 9 protocols, `NFC_READER=y`, `NFC_STORE=y`, `NFC_STORE_RAM=y`, `NFC_APPLETS=y`, `NFC_ROLE_LISTEN=n` (no listen stack) |
| `overlay-pn7160-stack.conf;overlay-pn7160-listen.conf` | READER + listen | all above + `NFC_LISTEN_STACK=y`, `NFC_FRAMING=y`, `NFC_ROUTER=y`, `NFC_ROLE_LISTEN=y` |
| `overlay-pn7160-hal.conf` | LAB | `NFC_ROLE_READER=y` but `NFC_READER` absent; no store/applets/protocols |

#### 2. Overlay → Profile Mapping

| Overlay file | Profile set | Additional explicit symbols |
|--------------|-------------|----------------------------|
| `overlay-pn7160-stack.conf` | `NFC_PROFILE_READER=y` | `NFC_ROLE_LISTEN=n`, board deps (I2C/GPIO/EMUL/PN7160) |
| `overlay-pn7160-listen.conf` | (layers on reader) | `NFC_ROLE_LISTEN=y`, `NFC_LISTEN_STACK=y` |
| `overlay-pn7160-hal.conf` | `NFC_PROFILE_LAB=y` | `NFC_ROLE_READER=y`, `# CONFIG_NFC_READER is not set` |
| `overlay-nfct-stack.conf` | `NFC_PROFILE_CARD_EMULATION=y` | `NFC_T4T_NRFXLIB=y`, `NFC_HAL_BACKEND_NRFX=y`, `# CONFIG_NFC_ROLE_READER is not set` |

**Imply chains match overlays:** one profile line enables the expected orchestrators, store, and protocols. Overlays reduced to profile + board deps + explicit role overrides.

#### 3. `nfc_reader` Test Divergence (document for P5 — DO NOT FIX HERE)

| Finding | File | Category | Severity | Fix phase |
|---------|------|----------|----------|-----------|
| All 9 protocol `.c` files compiled **unconditionally** | `tests/unit/nfc_reader/CMakeLists.txt:27-52` | D (Kconfig↔CMake) | divergence | P5 |
| `prj.conf` force-enables all 9 protocols regardless of scenario | `tests/unit/nfc_reader/prj.conf:6-14` | D | divergence | P5 |
| Test `Kconfig` **duplicates** the production symbol tree (155+ lines) instead of `rsource`-ing it | `tests/unit/nfc_reader/Kconfig:7-155` | D | divergence | P5 (Phase 3 per KCONFIG_ARCH §6) |
| `NFC_ROLE_LISTEN=y` stub present unconditionally | `tests/unit/nfc_reader/Kconfig:133-135` | D | minor | P5 |
| Capacity defaults differ (test: smaller for RAM; prod: larger) | `prj.conf` vs `src/nfc/protocols/*/Kconfig` | D | acceptable | n/a |

**Root cause:** the test suite was written before profile `imply` chains landed. It compiles sources directly to prove store roundtrip without linking the HAL. This works but **diverges from profile builds** — the test proves code that a real overlay might not compile. Example: a CE-only profile (`NFC_PROFILE_CARD_EMULATION`) does not imply Classic/FeliCa/SLIX/ISO15693-3, yet `nfc_reader` tests them anyway.

**P5 fix plan (from Master Plan B.5):**
1. Wrap each protocol block in CMakeLists.txt under `if(CONFIG_NFC_PROTOCOL_X)`.
2. Split `prj.conf` into profile-scoped fragments (`prj_reader.conf`, `prj_ce.conf`).
3. Add twister scenarios tied to profiles.
4. (Deferred to KCONFIG_ARCH Phase 3) Replace duplicated test Kconfig with `rsource "../../../src/nfc/Kconfig"`.

#### 4. Kconfig Inconsistencies Found

| Finding | Category | Severity | Notes |
|---------|----------|----------|-------|
| None critical | — | — | Imply chains are correct; `select` (CRC from NFC_STORE, NET_BUF from NFC_ROLE_LISTEN, FRAMING/ROUTER/NDEF from NFC_LISTEN_STACK) match documented architecture |

#### 5. Profile → Compiles → May-Test Mapping (per Master Plan Part B)

| Profile | Compiles (production) | Tests that may run |
|---------|----------------------|-------------------|
| `NFC_PROFILE_READER` | reader engine, store+RAM, applets, **all 9 poller protocols**; **no listen stack** | protocol Tier A+B (all 9), `nfc_reader` Tier E (store+store_ram), applet L1 headless (future P5/P8) |
| `NFC_PROFILE_CARD_EMULATION` | listen stack (framing+router), store+RAM, applets, **emulate subset** (5 protocols) | Tier A+C for emulate subset, virtual loopback (emulatable only), `nfc_reader` Tier E |
| `NFC_PROFILE_LAB` | HAL + role scaffold only | module/HAL tier only (`pn7160_tml`, `nfc_apdu_asm`); no protocol/store/applet tiers |

**Test truth gap:** `nfc_reader` currently runs all 9 protocols for both scenarios (store, store_ram). A reader-profile-only scenario should be added in P5; a CE-profile scenario should skip clone-only protocols.

### Verification summary

```
$ west twister -T tests/unit/nfc_reader -t ci_unit -p qemu_cortex_m3 --no-sysbuild
INFO - 2 of 2 executed test configurations passed (100.00%)
INFO - 97 of 97 executed test cases passed (100.00%)
```

**Exit gate: PASS.** No code changes required for P1 (read-only audit).

---

## P2 — HAL + APDU Layer CONTEXT.md

**Status:** PENDING

---

## P3 — Reader + Stack + Run + Store CONTEXT.md

**Status:** PENDING

---

## P4 — Protocol CONTEXT.md + Parity Matrix

**Status:** PENDING

---

## P4b — Applets CONTEXT.md + L1 Verification

**Status:** PENDING

---

## P5 — Test Gating Fix

**Status:** PENDING

---

## P6 — Full QEMU Green + CI + Doc Drift

**Status:** PENDING

---

## P7 — Architecture Assembly

**Status:** PENDING

---

## Summary

| Phase | Status | Key metric |
|-------|--------|------------|
| A | DONE | 97/97 baseline |
| P1 | DONE | 97/97, imply chains verified |
| P2 | PENDING | — |
| P3 | PENDING | — |
| P4 | PENDING | — |
| P4b | PENDING | — |
| P5 | PENDING | — |
| P6 | PENDING | — |
| P7 | PENDING | — |
