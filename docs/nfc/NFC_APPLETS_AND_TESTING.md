# NFC applets and testing

**Status:** LOCKED — product shell names, test tiers, and fixture policy  
**Normative detail:** [NFC_PROTOCOLS_COOKBOOK.md](NFC_PROTOCOLS_COOKBOOK.md) §7.1 (applets), §14 (tiers A–E), [§14.11](NFC_PROTOCOLS_COOKBOOK.md#1411-per-protocol-registration-checklist-locked) (registration checklist)  
**Execution:** [plans/2026-06-14-nfc-sequential-execution.md](plans/2026-06-14-nfc-sequential-execution.md)

This document is the standalone entry point for **what users run** (`nfc *` applets), **what developers run** (DK primitives), and **how we test** (QEMU tiers + HIL gates).

---

## Product applets (LOCKED)

| Applet | Shell | Notes |
|--------|-------|-------|
| `nfc scan` | `nfc scan [timeout_ms]` | Prints UID / protocol / tech to shell |
| `nfc read <slot>` | `nfc read <slot> [timeout_ms]` | One-shot scan + save |
| `nfc emulate <slot>` | `nfc emulate <slot> [ndef]` | Requires listen overlay |
| `nfc verify <slot>` | `nfc verify <slot> [timeout_ms]` | Poll + diff vs `.card` |
| `nfc loop <slot>` | `nfc loop <slot> [timeout_ms]` | read → emulate → verify (HIL) |

**Shell aliases (LOCKED):** `nfc read <slot>` and **`nfc reader clone <slot>`** are the same applet — register both names.

**NOT applets (LOCKED):** `unlock`, `attack`, extra actions, per-protocol shells. **Exception:** `aliro provision` remains under `nfc aliro *` (protocol shell).

---

## DK primitives (LOCKED)

Developer-kit command trees — used by applets and debug scripts, not marketed as product applets:

| Tree | Module | Examples |
|------|--------|----------|
| `nfc reader *` | `reader/` | `scan`, `clone`, `stats` |
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

**NDEF first:** `tests/unit/nfc_ndef/` is the Tier A/B/C template. Other protocols copy after cookbook §14.10 exit criteria.

**CI fixture rule (LOCKED):** Twister links `tests/fixtures/<proto>/*.inc` and `*.bin` only — **no FlipperFormat parser in ztest**.

---

## Fixture tree (LOCKED)

```
tests/
  fixtures/
    nfc/flipper/     # 12 GPL .nfc reference files (not linked by CI)
    ndef/            # NDEF *.inc, *.bin
    store/           # Tier E *.card.bin envelopes (e.g. ndef_empty.card.bin)
    ultralight/      # (post–F1) derived from flipper/
    classic/         # (post–F2)
    …
  common/            # nfc_session_mock, nfc_response_spy, nfc_test_apdu, nfc_virtual_rf
  unit/
    nfc_ndef/        # Tier A/B/C template
    nfc_reader/      # Tier E scaffold
```

Flipper provenance and 12-file manifest: [tests/fixtures/nfc/flipper/README.md](../../tests/fixtures/nfc/flipper/README.md).

**Offline converter:** `scripts/nfc/flipper_nfc_to_fixture.py` — `.nfc` → `.inc`/`.bin` (stub OK; full parser not required for CI). NDEF (no Flipper protocol folder): `scripts/nfc/ndef_to_fixture.py` → Tier A `.bin` + Tier E `.card.bin` — see cookbook [§14.12](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked).

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

**Today:** physical read → model → `.card`. Flipper `.nfc` → **NOT YET** → `.card` (converter stub).

**Required bridge for applet mock tests:** offline toolchain on the host:

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
2. NDEF `.card.bin` golden for Tier E emulate-without-RF — **`tests/fixtures/store/ndef_empty.card.bin`** (regen: `scripts/nfc/ndef_to_fixture.py`)
3. Extend `flipper_nfc_to_fixture.py`: `.nfc` → `.card.bin` + `.inc` + `.bin` per protocol (checklist: cookbook §14.12)
4. FF store v2: disk `.nfc`, load same as step 3 host path

**NDEF is the reference walkthrough.** It has no Flipper `protocols/ndef/` upstream; goldens come from cookbook §5.1 and `tests/fixtures/ndef/`, with store envelope in `tests/fixtures/store/`. Copy the three-pillar chain (model `.bin` + poller `.inc` + `.card.bin`) and test layout in `tests/unit/nfc_ndef/` + `tests/unit/nfc_reader/` before landing F1+ protocols. Full step-by-step: cookbook [§14.12](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked).

### NOT allowed

- Flipper `nfc_device_load()` in firmware or CI
- Assuming `.card` is a copy of `.nfc` bytes (different formats, same model)

---

## Migration table (old → locked names)

| Previous / draft | Locked applet or alias |
|------------------|------------------------|
| `nfc reader scan` | `nfc scan` (applet); DK keeps `nfc reader scan` |
| `nfc reader clone` | `nfc read <slot>` (applet alias) |
| `nfc clone` | **`nfc read`** (removed as standalone applet name) |
| `nfc emulate` | unchanged |
| `nfc verify` | unchanged |
| `nfc reader verify` | folded into **`nfc verify`** |
| per-protocol shells (`nfc ndef *`, etc.) | **NOT applets** — DK / debug only |

---

## Gate ↔ tier map (LOCKED)

| Gate | Applets | Ultralight / Classic |
|------|---------|----------------------|
| 2 | `nfc read` / `nfc reader clone` | NDEF Type-4 only at Gate 2 |
| 3 | `nfc emulate` (PN7160 CE infra) | — |
| 4 | `nfc emulate`, `nfc verify`, `nfc loop` | PN7160 CE loop first |
| 5 | `nfc emulate` on NFCT | Classic poller post–Gate 5; Ultralight emulate via **NDEF T4 adapter** (no native T2 listener) |

---

## Per-protocol registration

Before landing a new `protocols/<name>/` module, complete the checklist in cookbook [§14.11](NFC_PROTOCOLS_COOKBOOK.md#1411-per-protocol-registration-checklist-locked) and the golden-chain workflow in [§14.12](NFC_PROTOCOLS_COOKBOOK.md#1412-protocol-golden-chain-workflow-locked): scaffold, Tier A/B/C fixtures, Twister tags, §5.x matrix row, HIL row.

---

## Open items (all LOCKED — do not re-debate)

| Item | Decision |
|------|----------|
| Applet set | `scan`, `read`, `emulate`, `verify`, `loop` only |
| `nfc read` alias | `nfc reader clone` always registered |
| Flipper in CI | `.nfc` offline only; `.inc`/`.bin` in Twister |
| Tier E owner | `tests/unit/nfc_reader/` at Gate 2 scaffold |
| Ultralight listen | NDEF T4 adapter post–Gate 2; skip native T2 Tier C |
| Classic | PN7160 poller only; clone-only; post–Gate 5 |
| Aliro provision | protocol shell, not applet |

---

## Related docs

- [NFC_PROTOCOLS_COOKBOOK.md](NFC_PROTOCOLS_COOKBOOK.md) — protocol recipes, §14 tiers, §14.11 checklist, **§14.12 golden chain workflow**
- [CI_TESTING.md](CI_TESTING.md) — Twister jobs and local commands
- [NFC_STACK_PLAN.md](NFC_STACK_PLAN.md) — gate sequencing
