# NFC applets and testing

**Status:** LOCKED — product shell names, test tiers, and fixture policy  
**Normative detail:** [NFC_PROTOCOLS_COOKBOOK.md](NFC_PROTOCOLS_COOKBOOK.md) §7.1 (applets), §14 (tiers A–E), [§14.11](NFC_PROTOCOLS_COOKBOOK.md#1411-per-protocol-registration-checklist-locked) (registration checklist)  
**Execution:** [plans/2026-06-14-nfc-sequential-execution.md](plans/2026-06-14-nfc-sequential-execution.md)

This document is the standalone entry point for **what users run** (`nfc *` applets), **what developers run** (DK primitives), and **how we test** (QEMU tiers + HIL gates).

---

## Product applets (LOCKED 2026-06-14 — landed Phase A)

The applet model is split into a **headless Layer-1 service**
(`src/nfc/applets/nfc_applet_service.h`, no `struct shell`) and a **Layer-2
shell adapter** (`*_shell_cmds.c`, the only place a `struct shell *` lives). The
table below is the LOCKED product surface; see the layer detail in
[NFC_SHELL_APPLETS.md](NFC_SHELL_APPLETS.md).

| Applet (L1 module) | Shell (L2) | One sentence |
|--------------------|------------|--------------|
| Scan (`nfc_applet_scan`) | `nfc scan start` / `nfc scan stop` | Start/stop **continuous** reader discovery; a per-tag callback fires on each tag (default shell adapter prints UID/protocol/tech) |
| Read (`nfc_applet_read`) | `nfc read <slot> [t]` | One-shot detect + poller clone → store slot |
| Emulate (`nfc_applet_emulate`) | `nfc emulate <slot> [ndef]` · `nfc emulate golden <name>` | Load slot, policy check, start listen (golden = store import + emulate shortcut) |
| Loop (`nfc_applet_loop`) | `nfc loop <slot> [t]` | Orchestrate read → emulate → check on one card (HIL sign-off) |
| Check (`nfc_applet_verify`) | `nfc check <slot> [t]` *(DK only)* | Field diff vs stored slot → PASS/FAIL; **internal to Loop**, exposed only as a DK command (was `nfc verify`) |

Helpers (not standalone applets): Policy (`nfc_applet_policy`, emulate
eligibility) and verify-compare core are internal.

**NOT applets (LOCKED):** store, reader engine, stack, HAL, golden import,
`unlock`, `attack`, extra actions, per-protocol shells. **Exception:**
`aliro provision` remains under `nfc aliro *` (protocol shell).

**DROPPED (LOCKED 2026-06-14, Phase A):**
- `nfc reader clone` — removed; **use `nfc read`** (no alias).
- `nfc verify` as a product command — renamed to DK-only **`nfc check`**.
- blocking `nfc scan` — replaced by `nfc scan start` / `nfc scan stop`
  (continuous discovery + callback).

---

## DK primitives (LOCKED)

Developer-kit command trees — used by applets and debug scripts, not marketed as product applets:

| Tree | Module | Examples |
|------|--------|----------|
| `nfc reader *` | `reader/` | `scan`, `detect` (clone DROPPED — use `nfc read`) |
| `nfc check` | `applets/` | DK field diff vs stored slot (was `nfc verify`) |
| `nfc stack *` | `nfc_stack/` | `start`, `stop`, `load` |
| `nfc store *` | `store/` | `save`, `load`, `list` |
| `nfc_transport *` | `hal/` | backend debug (PN7160 / NFCT) |

Applets compose primitives. CI may call primitives directly for bring-up; product docs lead with applet names.

---

## Test tiers

| Tier | Scope | Where | CI |
|------|-------|-------|-----|
| **A** | Model serialize/deserialize | `tests/unit/nfc_<proto>/` | QEMU |
| **B** | Poller `detect`/`read` vs mock scripts | same + `tests/fixtures/<proto>/*.inc` | QEMU |
| **C** | Listener `on_apdu` vs `nfc_test_apdu` | same + `nfc_response_spy` | QEMU |
| **D** | HIL — physical tag / CE / NFCT | Manual or tagged HIL job | Not default Twister |
| **E** | Applet — store envelope, clone slot, verify diff | `tests/unit/nfc_reader/` | QEMU (Gate 2+) |
| **E+** | Virtual loopback — load → emulate → poller read → save → compare | `tests/unit/nfc_reader/` (`test_virtual_loopback.c`) | QEMU (emulatable protocols) |

**Per-card Flipper parity for offline sim (LOCKED):** Every emulatable protocol/card variant must have Tier A/B/C coverage **and** a Tier E+ virtual loopback case that mirrors Flipper test **intent** — load golden → emulate (listener) → poller read → save → compare — without pasting GPL code. Use `nfc_virtual_loopback_run()` as the standard harness (`tests/common/include/nfc_virtual_loopback.h`).

**NDEF (no Flipper `.nfc` — cookbook §5.1 / NXP `RW_NDEF_T4T`):**

| Variant | Tier A/B/C | Tier E+ | `.card.bin` |
|---------|------------|---------|-------------|
| empty T4T | `tests/unit/nfc_ndef/` green (14/17/31) | `test_virtual_loopback_empty_card` | `ndef_empty.card.bin` |
| URI (5 B) | same | `test_virtual_loopback_uri_5byte` | `ndef_uri_5byte.card.bin` |
| chunk (NLEN=300) | same | `test_virtual_loopback_chunk_255` | `ndef_chunk_255.card.bin` |

**Flipper catalog (12 `.nfc` files — parity tracker):**

| Flipper `.nfc` | Tier A/B fixtures | Tier A/B/C ztest | Tier E+ | `.card.bin` |
|----------------|-------------------|------------------|---------|-------------|
| `Ultralight_11.nfc` | `tests/fixtures/ultralight/` | pending F1 (`nfc_ultralight/`) | pending F1 | `Ultralight_11.card.bin` |
| `Ultralight_21.nfc` | yes | pending F1 | pending F1 | `Ultralight_21.card.bin` |
| `Ultralight_C.nfc` | yes | pending F1 | pending F1 | `Ultralight_C.card.bin` |
| `Ntag213_locked.nfc` | yes | pending F1 | pending F1 | `Ntag213_locked.card.bin` |
| `Ntag215.nfc` | yes | pending F1 | pending F1 | `Ntag215.card.bin` |
| `Ntag216.nfc` | yes | pending F1 | pending F1 | `Ntag216.card.bin` |
| `Felica.nfc` | `tests/fixtures/felica/` | pending F4 | **SKIP** (clone-only) | `Felica.card.bin` |
| `Slix_cap_default.nfc` | `tests/fixtures/slix/` | pending F4 | **SKIP** | `Slix_cap_default.card.bin` |
| `Slix_cap_missed.nfc` | yes | pending F4 | **SKIP** | `Slix_cap_missed.card.bin` |
| `Slix_cap_accept_all_pass.nfc` | yes | pending F4 | **SKIP** | `Slix_cap_accept_all_pass.card.bin` |
| `nfc_nfca_signal_short.nfc` | `tests/fixtures/hal/` | pending (HAL) | n/a | n/a (HAL framing) |
| `nfc_nfca_signal_long.nfc` | yes | pending (HAL) | n/a | n/a |

Clone-only protocols (FeliCa, SLIX, Classic): Tier B + store save/load only; **SKIP E+**. Ultralight emulate path uses NDEF T4 adapter post–F1.

Regen NDEF goldens: `python3 scripts/nfc/ndef_to_fixture.py --variant all`. Regen Flipper-derived fixtures + `.card.bin`: `python3 scripts/nfc/flipper_nfc_to_fixture.py --all --card-bin`. See cookbook [§14.12 Phase 1b](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked).

**NDEF first:** `tests/unit/nfc_ndef/` is the Tier A/B/C template. Other protocols copy after cookbook §14.10 exit criteria.

### NDEF Type-2 vs Type-4 routing (LOCKED)

| Path | Wire | Poll module | Emulate module | Test suite | Tier C owner | Flipper reference |
|------|------|-------------|----------------|------------|--------------|-------------------|
| **T4T** | ISO-DEP SELECT/READ BINARY on NDEF AID | `protocols/ndef/ndef_poller.c` | `protocols/ndef/ndef_listener.c` | `tests/unit/nfc_ndef/` (87 cases, 3 configs) | `nfc_ndef` listener | **None** — no Flipper T4 poller; goldens from NXP `RW_NDEF_T4T` via `ndef_to_fixture.py` |
| **T2 (Ultralight/NTAG)** | NFC-A page READ (`0x30`); CC page 3; TLV page 4+ | `protocols/ultralight/ultralight_poller.c` | T4 adapter → `ndef_listener.c` | `tests/unit/nfc_ultralight/` (32 cases, 2 configs) | **`nfc_ndef` listener** (adapter handoff; **not** native T2 Tier C in `nfc_ultralight/`) | `supported_cards/ndef.c` **`NDEF_PROTO_UL`** — **content parse only** |

**Do not route T2 tests to `nfc_ndef` poller** — page READ scripts belong in `nfc_ultralight/`. **Do not cite Flipper as T4 wire golden** — Flipper `.nfc` Ultralight files are raw pages converted by `flipper_nfc_to_fixture.py`.

**Ultralight F1 emulate (Tier C via adapter):** pending; when landed, Tier E+ loopback uses T4 adapter + `ndef_listener`, not a separate `ultralight_listener` ztest suite. See [`NDEF_T2_T4_ROUTING.md`](NDEF_T2_T4_ROUTING.md).

**T4 golden variants** (`tests/unit/nfc_ndef/testcase.yaml` configs `sample.nfc.unit.nfc_ndef.{model,poller,listener}`):

| Variant | Fixture prefix | Tier E+ loopback |
|---------|----------------|------------------|
| Empty T4T | `tests/fixtures/ndef/empty.*` | `test_virtual_loopback_empty_card` |
| URI 5 B | `tests/fixtures/ndef/uri_5byte.*` | `test_virtual_loopback_uri_5byte` |
| Chunk NLEN=300 | `tests/fixtures/ndef/chunk_255.*` | `test_virtual_loopback_chunk_255` |

Generator: `scripts/nfc/ndef_to_fixture.py --variant all` (cookbook [§5.1](NFC_PROTOCOLS_COOKBOOK.md#51-ndef-type-4-iso-dep--t4t)).

**CI fixture rule (LOCKED):** Twister links `tests/fixtures/<proto>/*.inc` and `*.bin` only — **no FlipperFormat parser in ztest**.

**Ztest counts (QEMU, 2026-06-14):**
- `nfc_ndef`: 87 cases (3 configs)
- `nfc_ultralight`: 32 cases (2 configs)
- `nfc_classic`: 17 cases (2 configs)
- `nfc_felica`: 13 cases (2 configs)
- `nfc_slix`: 18 cases (2 configs)
- `nfc_desfire`: 30 cases (3 configs)
- `nfc_emv`: 17 cases (3 configs)
- `nfc_aliro`: 19 cases (3 configs)
- `nfc_reader`: 59 ztests (3 scenarios: store, store_ram, shell_off)
- `nfc_apdu_asm`: 4 cases (1 config)
- `pn7160_tml`: 9 cases (1 config)

**Total CI matrix:** 24 configs, 392 test cases.

---

## Fixture tree (LOCKED)

```
tests/
  fixtures/
    nfc/flipper/     # 12 GPL .nfc reference files (not linked by CI)
    ndef/            # NDEF *.inc, *.bin
    store/           # Tier E *.card.bin envelopes (13 goldens — see store/README.md)
    ultralight/      # Tier A/B from flipper/ (6 variants)
    felica/          # Tier A/B from Felica.nfc
    slix/            # Tier A/B from Slix_cap_*.nfc (3)
    hal/             # nfca_signal_* from flipper/
    classic/         # (post–F2)
    …
  common/            # nfc_session_mock, nfc_response_spy, nfc_test_apdu, nfc_virtual_rf
  unit/
    nfc_ndef/        # Tier A/B/C template
    nfc_reader/      # Tier E + E+ (13 ztests — store, verify, loopback)
```

Flipper provenance and 12-file manifest: [tests/fixtures/nfc/flipper/README.md](../../tests/fixtures/nfc/flipper/README.md).

**Offline converter:** `scripts/nfc/flipper_nfc_to_fixture.py` — full FlipperFormat parser; `.nfc` → `<stem>_model.bin` + `<stem>_read.inc` (+ optional `<stem>.card.bin` via `--card-bin`). Batch: `--all --card-bin` regenerates all 12 Flipper inputs (10 store goldens; HAL signal files skip `.card.bin`). NDEF (no Flipper protocol folder): `scripts/nfc/ndef_to_fixture.py` → Tier A `.bin` + Tier E `.card.bin` — see cookbook [§14.12](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked).

---

## Card format and Flipper golden chain

**Status:** LOCKED — three representations, translation graph, test-tier inputs, implementation order

A tag or saved slot is never one file format. It is the same **protocol model** expressed in three ways. Tests pick an input tier deliberately; confusing `.card` with Flipper `.nfc` bytes is a policy violation.

### Three representations of the same card

1. **Flipper `.nfc`** (text, FlipperFormat) — human-readable golden and provenance reference. Canonical copies live in `tests/fixtures/nfc/flipper/`. GPL upstream is reference-only in CI; firmware never loads these files.

2. **Protocol model** (`ndef_data_t`, ultralight model, etc.) — in-RAM truth after a poller read **or** after our FlipperFormat parse on the host.

3. **Product blob** — what the store persists on disk:
   - **v1 (now):** `.card` binary TLV envelope (`nfc_store_save` / `nfc_store_load`)
   - **v2 (planned):** `.nfc` on disk via our FF port (same protocol model inside)

`.card` and `.nfc` are **not** byte-for-byte copies. They are different serializations of the same model.

### Translation graph (authoritative)

```
                    ┌─────────────────┐
  Physical tag ────►│ poller read     │───┐
                    └─────────────────┘   │
                                          ▼
                    ┌─────────────────┐   ┌──────────────┐
  Flipper .nfc ────►│ FF parse (ours) │──►│ protocol     │
  (offline/host)    └─────────────────┘   │ model        │
                                          └──────┬───────┘
                                                 │
                    ┌────────────────────────────┼────────────────────────────┐
                    ▼                            ▼                            ▼
              serialize                   Tier A .bin                  Tier B .inc
              (store)                     (model golden)               (poller script)
                    │
                    ▼
              .card blob (v1)  OR  .nfc file (v2 store)
```

**Today:** physical read → model → `.card`. Flipper `.nfc` → model → `.card.bin` via `flipper_nfc_to_fixture.py --card-bin` (10 Flipper-derived store goldens committed; 3 NDEF goldens via `ndef_to_fixture.py`).

**Offline bridge for applet mock tests:**

`.nfc` → FF parse → model → `nfc_store` serialize → `tests/fixtures/store/<name>.card.bin`

Tier E store tests load that golden via the store API — no RF, no runtime FF parser in ztest.

### What each test tier uses (no ambiguity)

| Tier | Input file | Parses `.nfc` at runtime? | Uses `.card`? |
|------|------------|----------------------------|---------------|
| **A** | `.bin` | No | No |
| **B** | `.inc` | No | No |
| **C** | `.inc` / helpers | No | No |
| **E** (applet / store) | `.card.bin` golden | No | **Yes** — load via store API |
| **E+** (virtual loopback) | `.card.bin` golden + `nfc_virtual_rf` | No | **Yes** — load, poller read, save, compare |
| **E** (FF validation, host) | `.nfc` | **Yes** (our FF port) | Compare serialize output to `.card.bin` |
| **D** / **HIL** | physical or emulated tag | No | Yes (from `nfc read`) |

Twister and ztest never parse FlipperFormat. Host-only validation may parse `.nfc` and assert the serialized `.card.bin` matches.

### Implementation order (locked)

1. Gate 4 applets on `.card` from live read (HIL)
2. NDEF `.card.bin` goldens for Tier E emulate-without-RF — **`tests/fixtures/store/ndef_{empty,uri_5byte,chunk_255}.card.bin`** (regen: `scripts/nfc/ndef_to_fixture.py --variant all`)
3. ~~Extend `flipper_nfc_to_fixture.py`: `.nfc` → `.card.bin`~~ — **done** (`--card-bin`; regen `--all --card-bin`)
4. FF store v2: disk `.nfc`, load same as step 3 host path

**NDEF is the reference walkthrough.** It has no Flipper `protocols/ndef/` upstream; goldens come from cookbook §5.1 and `tests/fixtures/ndef/`, with store envelope in `tests/fixtures/store/`. Copy the three-pillar chain (model `.bin` + poller `.inc` + `.card.bin`) and test layout in `tests/unit/nfc_ndef/` + `tests/unit/nfc_reader/` before landing F1+ protocols. Full step-by-step: cookbook [§14.12](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked).

### NOT allowed

- Flipper `nfc_device_load()` in firmware or CI
- Assuming `.card` is a copy of `.nfc` bytes (different formats, same model)

---

## Migration table (old → locked names)

| Previous / draft | Locked command (2026-06-14, Phase A) |
|------------------|------------------------|
| blocking `nfc scan` | **`nfc scan start`** / **`nfc scan stop`** (continuous discovery + callback) |
| `nfc reader scan` | DK primitive, kept (async one-shot discovery start) |
| `nfc reader clone` | **DROPPED** — use **`nfc read <slot>`** (no alias) |
| `nfc clone` | **`nfc read`** (removed as standalone applet name) |
| `nfc emulate` | unchanged |
| `nfc verify` | **`nfc check`** (renamed; DK-only) |
| `nfc reader verify` | folded into **`nfc check`** |
| per-protocol shells (`nfc ndef *`, etc.) | **NOT applets** — DK / debug only |

---

## Gate ↔ tier map (LOCKED)

| Gate | Applets | Ultralight / Classic |
|------|---------|----------------------|
| 2 | `nfc read` (clone dropped) | NDEF Type-4 only at Gate 2 |
| 3 | `nfc emulate` (PN7160 CE infra) | — |
| 4 | `nfc emulate`, `nfc check`, `nfc loop` | PN7160 CE loop first |
| 5 | `nfc emulate` on NFCT | Classic poller post–Gate 5; Ultralight emulate via **NDEF T4 adapter** (no native T2 listener) |

---

## Per-protocol registration

Before landing a new `protocols/<name>/` module, complete the checklist in cookbook [§14.11](NFC_PROTOCOLS_COOKBOOK.md#1411-per-protocol-registration-checklist-locked) and the golden-chain workflow in [§14.12](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked): scaffold, Tier A/B/C fixtures, Twister tags, §5.x matrix row, HIL row.

**Landing a new protocol (4 steps):** cookbook [§14.13](NFC_PROTOCOLS_COOKBOOK.md#1413-landing-a-new-protocol-4-steps-locked). NDEF (`src/nfc/protocols/ndef/`, `tests/unit/nfc_ndef/`, `tests/unit/nfc_reader/src/test_virtual_loopback.c`) is the canonical walkthrough. Clone-only protocols: Tier B + store only; **SKIP E+**.

---

## Open items (all LOCKED — do not re-debate)

| Item | Decision |
|------|----------|
| Applet set | `scan` (start/stop), `read`, `emulate`, `loop` + DK `check` (was `verify`) |
| `nfc read` | sole read/clone command; `nfc reader clone` **dropped** (Phase A) |
| Scan model | continuous discovery (`scan start`/`scan stop`) + per-tag callback; blocking scan dropped |
| Flipper in CI | `.nfc` offline only; `.inc`/`.bin` in Twister |
| Tier E owner | `tests/unit/nfc_reader/` — **59 ztests** (12 store + 23 protocol roundtrip + 3 verify + 12 loopback + 7 headless applet + 2 poller registry) across 3 scenarios (store, store_ram, shell_off) |
| Ultralight listen | NDEF T4 adapter post–Gate 2; skip native T2 Tier C |
| Classic | PN7160 poller only; clone-only; post–Gate 5 |
| Aliro provision | protocol shell, not applet |

---

## Related docs

- [NFC_PROTOCOLS_COOKBOOK.md](NFC_PROTOCOLS_COOKBOOK.md) — protocol recipes, §14 tiers, §14.11 checklist, **§14.12 golden chain workflow**
- [NDEF_T2_T4_ROUTING.md](NDEF_T2_T4_ROUTING.md) — T2 page poll vs T4 ISO-DEP; PN7160 `RW_NDEF_T4T` note
- [CI_TESTING.md](CI_TESTING.md) — Twister jobs and local commands
- [NFC_STACK_PLAN.md](NFC_STACK_PLAN.md) — gate sequencing
