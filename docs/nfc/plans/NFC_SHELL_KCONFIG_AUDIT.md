# NFC Shell Kconfig Gating Audit

**Date:** 2026-06-14  
**Scope:** All NFC code under `src/nfc/` and `modules/nfc_pn7160/` that references Zephyr shell APIs.  
**Question:** Is every shell user gated by a `*_SHELL` Kconfig (with `depends on SHELL`) such that a build with `CONFIG_SHELL=n` does not compile shell code?

**Verdict (original):** **Partially gated.** Dedicated shell command files are correct. Four core module files still include or call shell APIs when only feature Kconfig (store, applets, listen stack) is enabled.

> **UPDATE — REMEDIATION LANDED 2026-06-14 (Phase A).** All four leaks below and
> the `nfc_applet.h` header leakage are **closed**. Layer-0/Layer-1 source now
> carries no `shell.h` include or `shell_*` symbol; the only `struct shell *`
> sites are the `*_shell_cmds.c` adapters gated by `*_SHELL`. Verified: reader
> profile `CONFIG_SHELL=n` build links; `nfc_reader` twister 2/2·97/97 green.
> See [`NFC_HARMONIZATION_MASTER_PLAN.md`](NFC_HARMONIZATION_MASTER_PLAN.md)
> Phase A. The "Remediation options" at the bottom record what was done.

---

## Kconfig model (intended)

Each shell surface has a module-specific option that depends on Zephyr `SHELL`:

| Kconfig | Location | Depends on |
|---------|----------|------------|
| `NFC_APPLETS_SHELL` | `src/nfc/applets/Kconfig` | `SHELL` |
| `NFC_READER_SHELL` | `src/nfc/reader/Kconfig` | `SHELL` |
| `NFC_STORE_RAM_SHELL` | `src/nfc/store/Kconfig` | `SHELL` |
| `NFC_LISTEN_STACK_SHELL` | `src/nfc/nfc_stack/Kconfig` | `SHELL` |
| `NFC_HAL_SHELL` | `src/nfc/hal/Kconfig` | `SHELL` |
| `PN7160_SHELL` | `modules/nfc_pn7160/zephyr/Kconfig` | `SHELL` |

When `CONFIG_SHELL=n`, these symbols are unavailable and default off. CMake should exclude all `*_shell*.c` sources.

---

## Correctly gated (pass)

These files are compiled **only** when their shell Kconfig is enabled (CMake `*_ifdef` / `if(CONFIG_*_SHELL)`):

| Source file | CMake gate |
|-------------|------------|
| `src/nfc/applets/nfc_applet_shell_cmds.c` | `CONFIG_NFC_APPLETS_SHELL` |
| `src/nfc/reader/nfc_reader_shell_cmds.c` | `CONFIG_NFC_READER_SHELL` |
| `src/nfc/store/nfc_store_shell_cmds.c` | `CONFIG_NFC_STORE_RAM_SHELL` |
| `src/nfc/nfc_stack/nfc_stack_shell_cmds.c` | `CONFIG_NFC_LISTEN_STACK_SHELL` |
| `src/nfc/hal/nfc_transport_shell_cmds.c` | `CONFIG_NFC_HAL_SHELL` |
| `modules/nfc_pn7160/src/pn7160_shell.c` | `CONFIG_PN7160_SHELL` |

Header: `src/nfc/store/nfc_store_ram.h` exposes shell prototypes and `nfc_store_cmds` only under `#if IS_ENABLED(CONFIG_NFC_STORE_RAM_SHELL)`.

---

## Not gated (fail) — ALL FIXED in Phase A

These files compiled under **feature** Kconfig, not shell Kconfig, and still
`#include <zephyr/shell/shell.h>` and/or called shell APIs. **All four are now
fixed (LANDED 2026-06-14).** The descriptions below are the original findings;
each is annotated with its fix.

### 1. `src/nfc/store/nfc_store.c` — **FIXED**

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_STORE` |
| **Shell Kconfig required** | None |
| **Issue** | Unconditional `#include <zephyr/shell/shell.h>`. `nfc_store_default_save()` calls `shell_print` / `shell_fprintf`. Registered as the default save callback in `nfc_store_init()`. |
| **Runtime note** | Returns `-ENODEV` when `user_ctx` is NULL, but shell symbols are still compiled into the store module. |

**Fix:** default save callback replaced with an inert `-ENODEV` no-shell stub;
`<zephyr/shell/shell.h>` include removed. The `@@NFCDUMP@@` hex dump is now a
shell-registered cb (`nfc_applet_shell_save_cb` in the adapter), never the
store default.

### 2. `src/nfc/store/nfc_store_ram.c` — **FIXED**

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_STORE_RAM` |
| **Shell Kconfig required** | None (include); handlers need `CONFIG_NFC_STORE_RAM_SHELL` |
| **Issue** | `#include <zephyr/shell/shell.h>` is inside the `CONFIG_NFC_STORE_RAM` block but **outside** `CONFIG_NFC_STORE_RAM_SHELL`. `cmd_nfc_store_ram_list` / `cmd_nfc_store_ram_dump` are correctly inside the shell block. |

**Fix:** `#include <zephyr/shell/shell.h>` moved inside the
`CONFIG_NFC_STORE_RAM_SHELL` block (next to the `cmd_*` handlers).

### 3. `src/nfc/applets/nfc_applet_scan.c` — **FIXED**

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_APPLETS` |
| **Shell Kconfig required** | None |
| **Issue** | `nfc_applet_scan_print()` is entirely shell output (`shell_print`, `shell_fprintf`). Only caller is `nfc_applet_shell_cmds.c` (shell-gated), but the implementation file is always built with applets. |

**Fix:** `nfc_applet_scan_print()` deleted; its body moved verbatim into the
adapter as the registered tag callback. `nfc_applet_scan.c` is now headless
(continuous discovery `discover_start/stop/active` + `scan_get_result`) and
includes no shell header.

### 4. `src/nfc/applets/nfc_applet_loop.c` — **FIXED**

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_APPLETS` + `CONFIG_NFC_LISTEN_STACK` |
| **Shell Kconfig required** | None |
| **Issue** | `nfc_applet_loop()` uses shell throughout. Only caller is `nfc_applet_shell_cmds.c`, but the file is built under listen-stack Kconfig, not shell Kconfig. |

---

**Fix:** `nfc_applet_loop()` replaced by headless `nfc_applet_loop_run(slot,
timeout, log_cb, ctx, &result)`; `cmd_nfc_loop` is a thin adapter that supplies
a printing log callback. No shell header in `nfc_applet_loop.c`.

## Header leakage (minor) — **FIXED**

`src/nfc/applets/nfc_applet.h` used to declare unconditionally:

- `void nfc_applet_scan_print(const struct shell *sh);`
- `int nfc_applet_loop(const struct shell *sh, const char *slot, k_timeout_t timeout);`

**Fix:** both shell-typed prototypes removed. The headless API moved to
`nfc_applet_service.h`; `nfc_applet.h` is now a thin `#include` shim. No
`struct shell` appears in any Layer-0/Layer-1 header.

---

## Example misconfiguration

A reader product build with:

```
CONFIG_NFC_PROFILE_READER=y   # implies NFC_STORE, NFC_STORE_RAM, NFC_APPLETS
CONFIG_SHELL=n
```

Expected (per gating intent): no shell code compiled.  
Actual: `nfc_store.c`, `nfc_store_ram.c`, and `nfc_applet_scan.c` still reference shell. Compile/link outcome depends on Zephyr SDK (not verified in this audit — `ZEPHYR_BASE` unavailable in audit environment).

Typical overlays (`overlay-pn7160-stack.conf`, etc.) set `CONFIG_SHELL=y` and `*_SHELL=y`, so the gap is latent, not observed in day-to-day builds.

---

## Summary table

| Category | Count | Status |
|----------|-------|--------|
| Dedicated `*_shell_cmds.c` files | 6 | Gated correctly |
| Core modules with shell usage | 4 | **Not gated** |
| Headers with unconditional shell API | 1 | Minor (`nfc_applet.h`) |
| Headers with gated shell API | 1 | OK (`nfc_store_ram.h`) |

---

## Remediation (LANDED 2026-06-14, Phase A)

1. **`nfc_store.c`** — ✅ default save replaced with an inert `-ENODEV`
   no-shell stub; shell include removed; `@@NFCDUMP@@` dump is a
   shell-registered cb only.
2. **`nfc_store_ram.c`** — ✅ `#include <zephyr/shell/shell.h>` moved inside
   `CONFIG_NFC_STORE_RAM_SHELL`.
3. **`nfc_applet_scan.c`** — ✅ `nfc_applet_scan_print()` body moved into the
   adapter as the registered tag callback; the file is headless (continuous
   discovery) under `NFC_APPLETS`.
4. **`nfc_applet_loop.c`** — ✅ headless `nfc_applet_loop_run()` with an
   optional log callback; `cmd_nfc_loop` is a thin adapter.
5. **`nfc_applet.h`** — ✅ shell-typed prototypes removed; headless API lives in
   `nfc_applet_service.h` and `nfc_applet.h` is a thin include shim.

**Note (chosen approach):** rather than gate the headless `.c` files under
`*_SHELL`, the rendering was *extracted* into the adapter so the applet service
files compile under `NFC_APPLETS` with no shell symbols — this is what makes the
files reusable by L3 (SMF/BLE/HIL) consumers, not just compilable without shell.
