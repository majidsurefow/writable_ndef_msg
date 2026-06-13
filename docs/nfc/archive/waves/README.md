# Archived Wave Plans

**Status:** Superseded — do not use for new implementation work.

These per-layer wave plans (`wave1`–`wave7`) were the original incremental
sequencing for the NFC emulation stack. They are preserved for historical
reference only.

**Current authority:** [`../../NFC_STACK_PLAN.md`](../../NFC_STACK_PLAN.md) — single
linear plan with gated commits.

**Driver wave (7):** PN7160 Zephyr driver v1.0 completed at commit `21bdd71`
(listen mode + reader path frozen per
[`../../specs/2026-06-14-pn7160-zephyr-driver.md`](../../specs/2026-06-14-pn7160-zephyr-driver.md)).

**Conventions:** [`../../NFC_STACK_CONVENTIONS.md`](../../NFC_STACK_CONVENTIONS.md) remains
binding for module contracts. Where wave plans say `services/`, the code tree
uses `protocols/`.
