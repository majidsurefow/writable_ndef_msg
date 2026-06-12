# NFC Emulation Stack — Wave Planning Guide

## Purpose

Defines the process for writing implementation plans for each layer of the NFC
emulation stack. Each wave produces one plan document. Wave N+1 reads Wave N's
output as ground truth. No code is written until all waves are complete and
approved.

This guide exists so that any agent — with zero prior session context — can pick
up a wave, produce a clean plan in the locked format, and hand off cleanly to the
next wave.

---

## Wave Structure

| Wave | Layer | Output | Reads |
|------|-------|--------|-------|
| 1 | HAL (`nfc_transport`) | `plans/wave1-hal.md` | Spec §6.1, nrfxlib headers |
| 2 | Framing (`apdu_assembler`) | `plans/wave2-framing.md` | Spec §6.2 + Wave 1 |
| 3 | Router (`aid_router`) | `plans/wave3-router.md` | Spec §6.3 + Waves 1-2 |
| 4 | Stack (`nfc_stack`) | `plans/wave4-stack.md` | Spec §7+9 + Waves 1-3 |
| 5a | NDEF service | `plans/wave5-ndef.md` | Spec §6.4.1 + Waves 1-4 |
| 5b | DeSFire service | `plans/wave5-desfire.md` | Spec §6.4.2 + Waves 1-4 + Flipper |
| 5c | Ultralight service | `plans/wave5-ultralight.md` | Spec §6.4.3 + Waves 1-4 + Flipper |
| 5d | EMV service | `plans/wave5-emv.md` | Spec §6.4.4 + Waves 1-4 + Flipper |
| 5e | Aliro service | `plans/wave5-aliro.md` | Spec §6.4.5 + Waves 1-4 + aliro/ |

Waves 1-4 are **sequential** — each depends on the locked output of the previous.
Wave 5 services (5a-5e) run **in parallel** — they are independent once the Wave 4
stack interface is locked.

---

## Reference Sources

| Source | Path | Role |
|--------|------|------|
| **Stack conventions** | `docs/NFC_STACK_CONVENTIONS.md` | **Mandatory — read first, every wave.** Binding rules for lifecycle, structs, callbacks, buffers, stats, threading. |
| Architecture spec | `docs/superpowers/specs/2026-06-08-nfc-emulation-stack-design.md` | The *what*: protocols, AIDs, layer responsibilities |
| Firmware design docs | `docs/{API_DESIGN_CREED,CALLBACK_REGISTRATION_GUIDE,STATS_API_DESIGN,NETWORK_BUFFERS,STACK_SPEC}.md` | Governing authority behind the conventions doc — consult when conventions points to them |
| nrfxlib NFC headers | `/opt/nordic/ncs/v3.2.4/nrfxlib/nfc/include/` | `nfc_t4t_lib.h`, `nfc_platform.h`, `nrf_nfc_errno.h` |
| Zephyr kernel headers | `/opt/nordic/ncs/v3.2.4/zephyr/include/` | k_work, k_fifo, net_buf, k_spinlock, smf |
| Flipper Zero firmware | `/Users/majidfaroud/flipperzero/` | Protocol references (DeSFire/Ultralight constants, EMV parser) |
| Aliro codebase | `/Users/majidfaroud/writable_ndef_msg/aliro/` | PSA crypto wrappers for the Aliro service |
| Project stats API | `/Users/majidfaroud/writable_ndef_msg/src/stats.h` | `STATS_*` macros |

All agents have unrestricted read access to every source. They decide what is
relevant — do not prescribe a narrow reading list beyond the mandatory
conventions doc.

**Ignore entirely:** `src/nfc_emulation.c/.h`. It is replaced by this stack. Do
not read it, reference it, or carry its patterns forward.

---

## Coding Standard

All plans target **MISRA C:2012** and the project's embedded-C best practices:

- Fixed-width integer types (`uint8_t`, `uint32_t`, `int32_t`), `U` suffix on
  unsigned literals.
- All variables initialized at declaration (Rule 9.1).
- Return values checked (Rule 17.7); controlling expressions Boolean (Rule 14.4).
- No dynamic allocation in the running phase — static buffers only.
- `switch` always has `default`; no dead code.
- Zephyr-specific MISRA deviations are pre-approved — see
  `rules/coding-standards/zephyr-misra-deviations.md` (CONTAINER_OF, k_work,
  K_MSEC, LOG_*, etc.). Plans should cite the relevant DEV-ZEP record where a
  deviation applies.

Each plan's **Implementation Notes** section must call out the MISRA-relevant
decisions for that layer.

---

## Per-Wave Process

Every wave follows this sequence, in order. Do not skip ahead — later steps
depend on the vocabulary locked in earlier ones.

### 1. Context sweep
Read `docs/NFC_STACK_CONVENTIONS.md` first — it is binding for every layer. Then
read the spec section for this layer, all prior wave plan outputs in full, and the
upstream sources you need (nrfxlib, Flipper, aliro, Zephyr). No writing yet —
gather first.

### 2. Types and constants
Define every type this layer introduces: structs, enums, typedefs, `#define`
constants, error codes. This is the layer's vocabulary. Lock it before touching
functions.

### 3. Public API signatures
Every public function: full signature, parameter names and types, return type,
and a one-line description. Signatures only — no bodies.

### 4. Contracts
For each public function: preconditions (what must hold before the call),
postconditions (what is guaranteed after), and every error code with the exact
condition that triggers it.

### 5. Internal state
All static / module-level state this layer owns. The full state machine: states,
valid transitions, and the event that triggers each transition. Note ISR vs
thread-context ownership of each piece of state.

### 6. Integration points
**Down:** what this layer calls into — dependency layers and the exact functions
used.
**Up:** what calls into this layer — its consumers and how they use it.
Both sides must match the locked signatures from adjacent wave plans.

### 7. Implementation notes
Non-obvious decisions. MISRA C:2012 constraints and the DEV-ZEP deviations that
apply. nrfxlib / Zephyr behaviors that must be respected (ISR context, buffer
lifetimes, FWI). Anything that would surprise the implementer.

### 8. Task breakdown
Step-by-step implementation tasks following the `writing-plans` granularity
(2-5 min per step, TDD where testable, commit points). Each task names exact
files to create/modify and shows the code stub.

---

## Plan Document Template

Every wave plan uses this exact skeleton:

```markdown
# Wave N: <Layer> Implementation Plan

**Layer:** <layer name>
**Files produced:** <exact paths under src/nfc/>
**Depends on:** <prior wave plan paths, or "none">
**Sources consulted:** <what was read in Step 1>

---

## 1. Types and Constants

## 2. Public API

## 3. Contracts

## 4. Internal State

## 5. Integration Points

## 6. Implementation Notes

## 7. Conventions Compliance
<!-- The CONVENTIONS §12 checklist, filled in for this layer -->

## 8. Tasks
- [ ] Task 1 ...
- [ ] Task 2 ...
```

---

## Gates

These are hard stops. Do not cross a gate without explicit user approval.

- Wave N+1 does not start until the Wave N plan is reviewed and **locked** by the
  user.
- Wave 5 agents start only after Wave 4 is locked.
- **No implementation code is written until all wave plans are approved.**
- Once locked, a wave plan is a contract. Implementation agents follow it exactly;
  any required deviation goes back to the user, not silently into the code.

---

## Output Layout

```
docs/
├── superpowers/
│   └── specs/
│       └── 2026-06-08-nfc-emulation-stack-design.md   (the spec — source of truth)
├── NFC_WAVE_PLANNING_GUIDE.md                          (this file)
└── superpowers/
    └── plans/
        ├── wave1-hal.md
        ├── wave2-framing.md
        ├── wave3-router.md
        ├── wave4-stack.md
        ├── wave5-ndef.md
        ├── wave5-desfire.md
        ├── wave5-ultralight.md
        ├── wave5-emv.md
        └── wave5-aliro.md
```

The target source tree the plans build toward:

```
src/nfc/
├── nfc_stack.c / .h
├── hal/        nfc_transport.c / .h
├── framing/    apdu_assembler.c / .h, apdu_types.h
├── router/     aid_router.c / .h, service.h
└── services/
    ├── ndef/        ndef_service.c / .h
    ├── desfire/     desfire_service.c / .h, desfire_data.h
    ├── ultralight/  ultralight_service.c / .h
    ├── emv/         emv_service.c / .h
    └── aliro/       aliro_service.c / .h
```
