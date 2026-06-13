# Archived NFC Specs

Historical design and planning documents superseded by the current authority stack.

## Current authority (`docs/nfc/specs/`)

| Document | Role |
|----------|------|
| [`2026-06-13-nfc-final-design.md`](../../specs/2026-06-13-nfc-final-design.md) | Master design — read first |
| [`2026-06-13-nfct-pn7160-capability-matrix.md`](../../specs/2026-06-13-nfct-pn7160-capability-matrix.md) | NFCT vs PN7160 capability matrix |
| [`2026-06-14-pn7160-zephyr-driver.md`](../../specs/2026-06-14-pn7160-zephyr-driver.md) | PN7160 driver spec (frozen @ `21bdd71`) |

**Execution:** [`NFC_STACK_PLAN.md`](../../NFC_STACK_PLAN.md) — single linear gated plan.  
**Conventions:** [`NFC_STACK_CONVENTIONS.md`](../../NFC_STACK_CONVENTIONS.md) — binding module contracts.

## Archived here (superseded)

| Document | Why archived |
|----------|--------------|
| [`2026-06-08-nfc-emulation-stack-design.md`](2026-06-08-nfc-emulation-stack-design.md) | Pre-PN7160 NFCT-only era; card-mode slice detail retained for wave context |
| [`2026-06-12-nfc-stack-architecture.md`](2026-06-12-nfc-stack-architecture.md) | Superseded by `NFC_STACK_CONVENTIONS.md` + `NFC_STACK_PLAN.md` |
| [`2026-06-13-implementation-phases.md`](2026-06-13-implementation-phases.md) | Wave-era PN7160-first phase ordering; `NFC_STACK_PLAN.md` is execution |
| [`2026-06-13-locked-architecture-summary.md`](2026-06-13-locked-architecture-summary.md) | Duplicate snapshot of final-design decisions |
| [`2026-06-14-pn7160-nxp-library-audit.md`](2026-06-14-pn7160-nxp-library-audit.md) | NXP port audit — complete |
| [`2026-06-14-pn7160-nxp-library-evaluation.md`](2026-06-14-pn7160-nxp-library-evaluation.md) | NXP library evaluation — complete |

Wave implementation detail lives in [`../waves/`](../waves/README.md).
