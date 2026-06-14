# src/nfc/run — CONTEXT

**Layer:** L0 (runtime) · **Lifecycle:** minimal

## Purpose

Shared NFC work queue: lazy-started `k_work_q` for all blocking HAL/engine operations. Centralizes thread priority and stack allocation. Does NOT own any NFC logic—just the execution context.

## Key files

| File | Owns |
|------|------|
| `nfc_stack_workq.c` | Work queue init, `nfc_stack_wq_get()` accessor |
| `nfc_stack_workq.h` | Public API: `nfc_stack_wq_get()` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_STACK_WORKQ_STACK_SIZE` | 2048 | Thread stack for `nfc_stack_wq` |
| `NFC_STACK_WORKQ_PRIORITY` | 5 | Thread priority |

## Deps

- **Upstream (calls):** Zephyr kernel (`k_work_queue_init`, `k_work_queue_start`)
- **Downstream (called by):** `reader/` (scan/read/clone work), `nfc_stack/` (implicitly via transport callbacks), `hal/` (listen recv loop)

## Invariants & safety

- **Lazy start:** `nfc_stack_wq_get()` initializes the queue exactly once on first call; subsequent calls return the same pointer
- **Single queue:** All NFC blocking work shares one queue—no concurrent poll+listen on the same controller (single-flight enforced at engine level, not here)
- **Stack sizing:** Default 2048 bytes must accommodate deepest call chain (discover_wait → NCI transceive → poller read); increase if adding heavy protocol logic

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| All `nfc_reader` tests | E | `NFC_STACK` | Work submitted via `nfc_stack_wq_get()` completes |

## Shell

No shell commands. This layer is purely a kernel primitive.
