# NFC Stack — Implementation Plan

**Status:** LOCKED — single linear execution plan (replaces wave sequencing)  
**Date:** 2026-06-14  
**Authority:** Module contracts in [`NFC_STACK_CONVENTIONS.md`](NFC_STACK_CONVENTIONS.md).
Where conventions say `services/`, implement as `protocols/` in code.

**Design:** [`specs/2026-06-13-nfc-final-design.md`](specs/2026-06-13-nfc-final-design.md),
[`specs/2026-06-13-nfct-pn7160-capability-matrix.md`](specs/2026-06-13-nfct-pn7160-capability-matrix.md)  
**Driver:** Frozen @ `21bdd71` —
[`specs/2026-06-14-pn7160-zephyr-driver.md`](specs/2026-06-14-pn7160-zephyr-driver.md)

---

## Source tree (`src/nfc/`)

```
hal/           nfc_transport + PN7160/NFCT backends
reader/        nfc_reader_engine — poll, clone, verify
nfc_stack/     card-role orchestrator (listen path)
protocols/     per-card data model + *_poller.c + *_listener.c
applets/       end-to-end workflows: clone, emulate, verify shells
store/         .card envelope + live persist
run/           init/start wiring, profile selection
```

## Threading

One application work queue: `nfc_stack_wq`. PN7160 driver owns `pn7160_wq`
(IRQ → NCI drain). No per-protocol threads.

## Gated commits (5)

| # | Scope | Gate (must pass before next) |
|---|-------|------------------------------|
| 1 | `hal/` PN7160 poll + `reader/` scan | `nfc reader scan` detects tag; driver unit tests green |
| 2 | `store/` minimal + `reader/` clone (NDEF) | `nfc reader clone` produces valid `.card` blob |
| 3 | `protocols/ndef` + framing/router + `nfc_stack` on PN7160 listen | Type-4 CE responds to SELECT / READ BINARY |
| 4 | `applets/` emulate + verify on PN7160 | clone → emulate → verify **PASS** on same chip |
| 5 | NFCT backend + port proven `protocols/` listeners | same `.card` on NFCT emulate; PN7160 verify **PASS** |

## Backlog (per protocol — not waves)

Add when the gated path above is green. Archived wave detail in
[`archive/waves/`](archive/waves/README.md).

| Protocol | Direction | Notes |
|----------|-----------|-------|
| ultralight | poller + T4 adapter listener | page model via NDEF wrapper |
| classic | poller only | clone-only; NFCT cannot emulate |
| emv | poller + static listener | public APDU data |
| desfire | poller + partial listener | auth SMF per conventions §2 |
| aliro | poller transcript + PSA listener | hand-provision path |

## Before any module work

Read `NFC_STACK_CONVENTIONS.md` §1–12 — lifecycle, coupling map, buffers,
stats recipe, threading, MISRA. Do not re-derive decisions locked there.
