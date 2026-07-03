# Dephase the in-tree PN7160 driver, consume the external `nfc-pn7160` module

**Date:** 2026-07-03
**Status:** Approved (design)
**Scope:** Swap + full cleanup (one pass)

## Problem

The project ships two PN7160 drivers with **the same Zephyr module name**
(`name: nfc_pn7160` in both `zephyr/module.yml` files):

- **Old (in-tree):** `modules/nfc_pn7160/` — a hand-written driver, force-injected
  via `ZEPHYR_EXTRA_MODULES` in `CMakeLists.txt:10`.
- **New (external):** `/Users/majidfaroud/nfc-pn7160` (GitHub:
  `majidsurefow/nfc-pn7160`) — a proper native re-implementation, already present
  in the local NCS v3.2.4 west workspace as project `nfc-pn7160`
  (`nrf/west.yml`, `file:///Users/majidfaroud/nfc-pn7160`, `path: modules/nfc-pn7160`)
  and self-discovered via its own `zephyr/module.yml`.

Because `ZEPHYR_EXTRA_MODULES` wins the module-name collision, **builds compile
the old driver while the new one sits unused.** This is the root cause of "using
the outdated driver."

## Compatibility findings (why this is a wiring change, not a rewrite)

The only project code that touches the driver is the HAL adapter
`src/nfc/hal/nfc_transport_pn7160.c`. Its entire call surface is byte-compatible
with the new driver:

| Surface | Old | New | Verdict |
|---------|-----|-----|---------|
| Module name (`module.yml`) | `nfc_pn7160` | `nfc_pn7160` | same |
| Public header path | `nfc/pn7160.h`, `nfc/pn7160_tml.h` | same | same |
| DT compat | `nxp,pn7160` | `nxp,pn7160` | same |
| DT bindings (common/i2c/spi) | 3 files | 3 files | **byte-identical** |
| `PN7160_TML_MAX_PAYLOAD_LEN` | 255U | 255U | same |
| Adapter-called functions | `pn7160_nci_connect`, `_configure_settings`, `_discovery_start/stop/wait`, `_reader_tag_cmd`, `_listen_start/stop`, `_card_mode_send/recv` | identical signatures | same |
| `struct pn7160_nci_rf_intf` | fields A | same fields **+ trailing `info` union** | additive only; adapter reads unchanged fields |
| Kconfig used by confs | `PN7160`, `PN7160_SHELL`, `PN7160_LOG_LEVEL_*` | all present (log module `PN7160`) | same |

**Consequence:** the adapter and all `confs/`, board overlays, and DT need **no
code changes**. This is purely a module-wiring swap plus deletion of the old tree.

## Target state

```
src/nfc/hal/nfc_transport_pn7160.c   (adapter — UNCHANGED)
        │  #include <nfc/pn7160.h>; calls pn7160_nci_*
        ▼
modules/nfc-pn7160   ← NEW driver, discovered via west manifest
```

- **Local dev:** governed by the NCS workspace manifest (`nrf/west.yml`,
  `file://` entry). Untouched by this work; local driver iteration keeps working
  against the live local repo.
- **CI:** governed by the app's own `west.yml` (used by
  `action-zephyr-setup` with `app-path: .`). Gains a pinned GitHub entry so a
  fresh checkout resolves the new driver reproducibly.

## Changes

### 1. Stop injecting the old driver
Remove the `ZEPHYR_EXTRA_MODULES … modules/nfc_pn7160` block from `CMakeLists.txt`.
This alone resolves the name collision so the west-discovered new driver is used.

### 2. Delete the old driver tree
Remove `modules/nfc_pn7160/` entirely (driver src, `include/`, `dts/`, `tests/`,
`zephyr/`, docs). If `modules/` becomes empty, remove it too.

### 3. Wire the new driver into the app `west.yml` for CI
Add a project entry to the app's `west.yml`:

```yaml
  projects:
    - name: nfc-pn7160
      url: https://github.com/majidsurefow/nfc-pn7160
      revision: faef0be3070fce5db8f7289466243b65961e0b79   # pin; bump deliberately
      path: modules/nfc-pn7160
```

- Pin to the current SHA (`faef0be`) for reproducibility; may be moved to a
  release tag later. Bumps are deliberate, not floating on `main`.
- `nfc-pn7160` is a **private** repo → CI needs read access (deploy key or PAT
  in the workflow checkout/west step). Confirm/provision before merging.
- This entry affects **CI only**; local builds continue to use the NCS
  workspace's `file://` entry. If both manifests define `nfc-pn7160`, the
  importing (app) manifest wins by west name-precedence — harmless.

### 4. Migrate CI off in-tree driver paths
The old driver's unit tests and bindings no longer live in this repo (they belong
to `nfc-pn7160`, which runs its own CI). Remove the in-tree references:

- `.github/workflows/twister.yaml:150` — drop
  `modules/nfc_pn7160/tests/unit/pn7160_tml` from `UNIT_DIRS`.
- `.github/workflows/codecov.yaml:26` — drop/replace
  `UNIT_TEST_DIR: modules/nfc_pn7160/tests/unit/pn7160_tml`.
- `.github/workflows/devicetree.yml:67` — drop the
  `modules/nfc_pn7160/dts/bindings` lint path.

Project CI keeps only the app's own tests (`tests/unit/nfc_*`, `nfc_apdu_asm`).

### 5. Fix stale documentation reference
Update the `west.yml` header comment that describes `modules/nfc_pn7160`
discovery via `ZEPHYR_EXTRA_MODULES`.

## Verification

1. Build one PN7160 config against the new module, e.g.:
   ```
   west build -p always -b nrf54l15dk/nrf54l15/cpuapp . \
     -- -DEXTRA_CONF_FILE=confs/layer1-pn7160.conf \
        -DDTC_OVERLAY_FILE=boards/overlays/pn7160_i2c.overlay
   ```
   Confirm it compiles and links, and that build artifacts reference
   `modules/nfc-pn7160` (not `modules/nfc_pn7160`).
2. Confirm **no** duplicate-`nfc_pn7160`-module CMake warning/error.
3. Grep the build's resolved sources to confirm `pn7160_driver.c` etc. come from
   the new path.

## Risks & mitigations

- **CI can't clone private repo** → provision deploy key/PAT (item 3). Blocking
  for CI green; does not block local swap.
- **New Kconfig symbols** (`PN7160_INIT_PRIORITY`, `PN7160_RX_BUF_SIZE`, shell
  monitor stack/priority) — all have defaults; confs don't set them → no action.
  Verified no symbol the confs *rely on* was removed.
- **Trailing `info` union** on `pn7160_nci_rf_intf` — additive; adapter reads
  only the unchanged leading fields.
- **Two manifests defining `nfc-pn7160`** (NCS local + app) — west precedence
  makes the app entry win in CI, local NCS entry win locally; no conflict.

## Out of scope (future passes)

- Adopting the new driver's added capabilities (`_preserve_mode` discovery, the
  `info` union) in the adapter/stack.
- The broader "make everything else work more correctly and cleanly" cleanup.
- Any use of the new driver's `nxp_compat` shim (off by default; project has its
  own HAL/stack).
