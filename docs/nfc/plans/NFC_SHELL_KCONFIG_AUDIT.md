# NFC Shell Kconfig Gating Audit

**Date:** 2026-06-14  
**Scope:** All NFC code under `src/nfc/` and `modules/nfc_pn7160/` that references Zephyr shell APIs.  
**Question:** Is every shell user gated by a `*_SHELL` Kconfig (with `depends on SHELL`) such that a build with `CONFIG_SHELL=n` does not compile shell code?

**Verdict:** **Partially gated.** Dedicated shell command files are correct. Four core module files still include or call shell APIs when only feature Kconfig (store, applets, listen stack) is enabled.

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

## Not gated (fail)

These files compile under **feature** Kconfig, not shell Kconfig, and still `#include <zephyr/shell/shell.h>` and/or call shell APIs.

### 1. `src/nfc/store/nfc_store.c`

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_STORE` |
| **Shell Kconfig required** | None |
| **Issue** | Unconditional `#include <zephyr/shell/shell.h>`. `nfc_store_default_save()` calls `shell_print` / `shell_fprintf`. Registered as the default save callback in `nfc_store_init()`. |
| **Runtime note** | Returns `-ENODEV` when `user_ctx` is NULL, but shell symbols are still compiled into the store module. |

### 2. `src/nfc/store/nfc_store_ram.c`

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_STORE_RAM` |
| **Shell Kconfig required** | None (include); handlers need `CONFIG_NFC_STORE_RAM_SHELL` |
| **Issue** | `#include <zephyr/shell/shell.h>` is inside the `CONFIG_NFC_STORE_RAM` block but **outside** `CONFIG_NFC_STORE_RAM_SHELL`. `cmd_nfc_store_ram_list` / `cmd_nfc_store_ram_dump` are correctly inside the shell block. |

### 3. `src/nfc/applets/nfc_applet_scan.c`

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_APPLETS` |
| **Shell Kconfig required** | None |
| **Issue** | `nfc_applet_scan_print()` is entirely shell output (`shell_print`, `shell_fprintf`). Only caller is `nfc_applet_shell_cmds.c` (shell-gated), but the implementation file is always built with applets. |

### 4. `src/nfc/applets/nfc_applet_loop.c`

| | |
|---|---|
| **Compiled when** | `CONFIG_NFC_APPLETS` + `CONFIG_NFC_LISTEN_STACK` |
| **Shell Kconfig required** | None |
| **Issue** | `nfc_applet_loop()` uses shell throughout. Only caller is `nfc_applet_shell_cmds.c`, but the file is built under listen-stack Kconfig, not shell Kconfig. |

---

## Header leakage (minor)

`src/nfc/applets/nfc_applet.h` always declares:

- `void nfc_applet_scan_print(const struct shell *sh);`
- `int nfc_applet_loop(const struct shell *sh, const char *slot, k_timeout_t timeout);`

No `#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)` guard. Forward `struct shell;` keeps the header compilable without `shell.h`, but the public applet API still mentions shell in no-shell builds.

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

## Remediation options (not implemented)

1. **`nfc_store.c`** — Replace default save stub with a no-shell stub (`-ENODEV` or log-only); move `@@NFCDUMP@@` shell dump behind `CONFIG_NFC_STORE_RAM_SHELL` or shell cmd layer only.
2. **`nfc_store_ram.c`** — Move `#include <zephyr/shell/shell.h>` inside `CONFIG_NFC_STORE_RAM_SHELL`.
3. **`nfc_applet_scan.c`** — Move `nfc_applet_scan_print()` into `nfc_applet_shell_cmds.c`, or compile `nfc_applet_scan.c` only when `CONFIG_NFC_APPLETS_SHELL`.
4. **`nfc_applet_loop.c`** — Same: shell-only file or `CONFIG_NFC_APPLETS_SHELL` in CMake.
5. **`nfc_applet.h`** — Wrap shell prototypes in `#if IS_ENABLED(CONFIG_NFC_APPLETS_SHELL)`.
