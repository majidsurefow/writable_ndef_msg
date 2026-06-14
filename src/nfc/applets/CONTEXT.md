# src/nfc/applets/ — Layer-1 Headless NFC Applets

**Purpose:** Business logic applets (scan, read, emulate, loop, check) that orchestrate L0 engines (reader, stack, store) without any shell coupling. These are the headless services consumed by L2 shell adapters or L3 application/SMF code.

**Status:** LOCKED 2026-06-14 (Phase A landed the L1/L2 deconvolution).

---

## Key Files

| File | Module | Description |
|------|--------|-------------|
| `nfc_applet_service.h` | API | L1 headless API — result structs, callbacks, no `struct shell *` |
| `nfc_applet_service.c` | shared | `nfc_applet_get_card_meta()` — slot metadata helper |
| `nfc_applet_scan.c` | Scan | Continuous discovery loop + per-tag callback |
| `nfc_applet_read.c` | Read | One-shot scan + poller clone → slot |
| `nfc_applet_emulate.c` | Emulate | Load slot, policy check, start listen (requires `NFC_LISTEN_STACK`) |
| `nfc_applet_loop.c` | Loop | Orchestrate read → emulate → check (requires `NFC_LISTEN_STACK`) |
| `nfc_applet_verify.c` | Check | Field diff vs stored slot — internal to Loop; DK shell only |
| `nfc_applet_verify_compare.c` | Check | Compare core (ndef_data_t diff) |
| `nfc_applet_policy.c` | Policy | Emulate eligibility: clone-only vs emulatable (internal) |
| `nfc_applet_shell_cmds.c` | **L2** | Shell adapter — the only file with `struct shell *` |

---

## Kconfig Symbols

| Symbol | Default | Depends on | Description |
|--------|---------|------------|-------------|
| `NFC_APPLETS` | y | `NFC_READER && NFC_STORE` | Enable L1 headless applets |
| `NFC_APPLETS_SHELL` | y | `NFC_APPLETS && SHELL` | Enable L2 shell adapters (`nfc scan/read/emulate/loop/check`) |

---

## L1 API Summary (`nfc_applet_service.h`)

### Scan Applet
- `nfc_applet_discover_start(cb, ctx)` — start continuous discovery
- `nfc_applet_discover_stop()` — stop discovery loop
- `nfc_applet_discover_active()` — true while loop running
- `nfc_applet_scan_get_result(out)` — snapshot active session to `nfc_applet_scan_result_t`

### Read Applet
- `nfc_applet_read_start(slot, timeout)` — one-shot scan + clone
- `nfc_applet_read_busy()` / `nfc_applet_read_wait(deadline)`

### Emulate Applet
- `nfc_applet_emulate(slot, profile)` — load + policy + stack start

### Check Applet (internal to Loop + DK)
- `nfc_applet_verify_start(slot, timeout)` / `_busy()` / `_wait()` / `_last_result()`
- `nfc_applet_verify_compare(expected, actual, ...)` — Tier E core

### Loop Applet
- `nfc_applet_loop_run(slot, timeout, log_fn, ctx, &result)` — full orchestration
- `nfc_applet_log_fn` callback for progress (NULL = silent/headless)
- `nfc_applet_loop_result_t` — failed stage + verify result

### Metadata Helper
- `nfc_applet_get_card_meta(slot, &out)` → `nfc_applet_card_meta_t`

---

## Callback Model

**Scan:** Continuous discovery runs a self-rescheduling work item on `nfc_stack_wq`. Each detected tag invokes `nfc_applet_tag_cb_t(info, ctx)`. The shell adapter registers a callback that prints UID/protocol/tech; headless consumers pass NULL or their own callback.

**Loop:** Orchestration emits per-stage progress via `nfc_applet_log_fn(ctx, stage, code)`. Shell adapter prints stage lines; tests pass NULL and assert on the result struct.

---

## Invariants

1. **No shell below L2:** `nfc_applet_service.h` and all `nfc_applet_*.c` (except `*_shell_cmds.c`) contain no `struct shell *`, no `#include <zephyr/shell/shell.h>`, and no `shell_print`/`shell_fprintf`.

2. **Headless operation:** L1 applets work with `CONFIG_SHELL=n`. The shell adapter is gated by `NFC_APPLETS_SHELL` which `depends on SHELL`.

3. **Check is internal:** The Check applet (`nfc_applet_verify_*`) is called internally by Loop and exposed as a DK-only `nfc check` command — not a product applet.

4. **Policy is internal:** `nfc_applet_policy.c` determines emulate eligibility; not exposed via shell.

---

## Shell Mapping (L2 → L1)

| Shell Command | L2 Adapter | L1 API Call |
|---------------|------------|-------------|
| `nfc scan start` | `cmd_nfc_scan_start` | `nfc_applet_discover_start(cb, sh)` |
| `nfc scan stop` | `cmd_nfc_scan_stop` | `nfc_applet_discover_stop()` |
| `nfc read <slot>` | `cmd_nfc_read` | `nfc_applet_read_start()` + `_wait()` |
| `nfc emulate <slot>` | `cmd_nfc_emulate` | `nfc_applet_get_card_meta()` + `nfc_applet_emulate()` |
| `nfc emulate golden <name>` | `cmd_nfc_emulate` | store import + `nfc_applet_emulate()` |
| `nfc loop <slot>` | `cmd_nfc_loop` | `nfc_applet_loop_run(slot, timeout, log, sh, &result)` |
| `nfc check <slot>` (DK) | `cmd_nfc_check` | `nfc_applet_verify_start()` + `_wait()` + `_last_result()` |

Full shell inventory: [`docs/nfc/NFC_SHELL_APPLETS.md`](../../../docs/nfc/NFC_SHELL_APPLETS.md)

---

## Tests That Prove It

| Suite | What it proves |
|-------|----------------|
| `tests/unit/nfc_reader` 97/97 | Store roundtrip, verify-compare, loopback |
| Grep audit (P4b) | No `struct shell` in L1 files |
| `CONFIG_SHELL=n` build | L1 applets link without shell symbols |

---

## Related Docs

- [`docs/nfc/NFC_SHELL_APPLETS.md`](../../../docs/nfc/NFC_SHELL_APPLETS.md) — full shell command reference (LOCKED)
- [`docs/nfc/NFC_APPLETS_AND_TESTING.md`](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md) — applet table + test tiers (LOCKED)
- [`plans/NFC_APPLET_API_LAYERING.md`](../../../docs/nfc/plans/NFC_APPLET_API_LAYERING.md) — result struct detail
