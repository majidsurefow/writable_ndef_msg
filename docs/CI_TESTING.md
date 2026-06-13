# CI and local testing

NCS workspace root (`west.yml` pins **v3.2.4**, same as `.github/workflows/twister.yaml`).

For west workspace layout, official doc links, and bootstrap commands, see
[ZEPHYR_WORKSPACE.md](ZEPHYR_WORKSPACE.md).

## What exists today

| Layer | Location | CI job | Status |
|-------|----------|--------|--------|
| Integration (app) | Root `sample.yaml` | `twister.yaml` → `twister-build` | **Active** — build-only on nRF DK matrix + `pn7160.i2c` / `pn7160.spi` overlays (`pn7160` tag; also in `devicetree.yml`) |
| Unit (module) | `modules/nfc_pn7160/tests/unit/pn7160_tml/` | `twister.yaml` → `twister-unit` | **Scaffold** — one ztest (`test_scaffold_passes`); Twister tag `ci_unit` |

**Unit suite layout:** `CMakeLists.txt` (sets `ZEPHYR_EXTRA_MODULES` to the module), `prj.conf` (`CONFIG_ZTEST`, I2C/GPIO/EMUL, `CONFIG_PN7160`) + unit-test overlay, `testcase.yaml` (`modules.nfc_pn7160.unit.pn7160_tml`), `src/main.c`.

**How unit tests plug in:** Twister `-T` points at the test directory (not the app root). The test app is a minimal ztest binary (`--no-sysbuild`). The out-of-tree module is pulled in via `ZEPHYR_EXTRA_MODULES` so Phase 0.2+ tests can compile TML/NCi sources under test without a west manifest entry for the module.

## How to run tests locally

Use **one** west workspace; only the **cwd** and **manifest root** differ.

| Setup | When to use | Prep |
|-------|-------------|------|
| **Repo workspace (CI-like)** | PR validation, fresh clones | `cd <repo>` → `west init -l .` → `west update` → Zephyr SDK / toolchain |
| **Global NCS tree** | nRF Connect / Toolchain Manager install at `/opt/nordic/ncs/v3.2.4` | `cd /opt/nordic/ncs/v3.2.4` (or your NCS path); put `west` on `PATH` from the matching toolchain; set `ZEPHYR_BASE` to that tree’s `zephyr/` |

Twister and `west build` only need `ZEPHYR_BASE` and the SDK; `-T` may be an **absolute path** to this repo’s test dir from a global NCS cwd.

```bash
export ZEPHYR_BASE="${ZEPHYR_BASE:-$(pwd)/zephyr}"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
# Optional on Mac with Toolchain Manager — prepend toolchain bin/ and zephyr-sdk arm-zephyr-eabi/bin to PATH
```

### macOS vs Multipass

| Target | macOS | Linux / Multipass |
|--------|-------|-------------------|
| `ci_build` on nRF DK boards | Yes (cross-compile) | Yes |
| `ci_unit` on QEMU ARM | Yes (if QEMU board exists in your Zephyr pin) | Yes |
| `native_sim` / `native_sim/native` | **No** | Yes (optional; not in merge-gate CI today) |

Use **Multipass** on Mac for `native_sim`. Merge-gate CI uses QEMU only (see platform note below).

## Local development on macOS

**Toolchain Manager** installs NCS at `/opt/nordic/ncs/v3.2.4` and a toolchain bundle at
`/opt/nordic/ncs/toolchains/<bundle_id>/`. `west` lives in the bundle, **not** on the default macOS `PATH`.

| What | Path (NCS v3.2.4 on this Mac) |
|------|--------------------------------|
| NCS root / `cd` for mode A | `/opt/nordic/ncs/v3.2.4` |
| `ZEPHYR_BASE` | `/opt/nordic/ncs/v3.2.4/zephyr` |
| Toolchain bundle | `/opt/nordic/ncs/toolchains/185bb0e3b6` |
| `west` | `/opt/nordic/ncs/toolchains/185bb0e3b6/bin/west` |
| `toolchains.json` (bundle ↔ version) | `/opt/nordic/ncs/toolchains/toolchains.json` |

Find your bundle: `find /opt/nordic/ncs/toolchains -path '*/bin/west' 2>/dev/null`, or Toolchain Manager → **Open terminal** / nRF Connect for VS Code.

### Finding `west`

```bash
find /opt/nordic -path '*/bin/west' 2>/dev/null
/opt/nordic/ncs/toolchains/185bb0e3b6/bin/west --version
```

**`~/.zshrc` one-liner block** (fix `bundle_id` if `toolchains.json` differs):

```bash
export NCS_ROOT=/opt/nordic/ncs/v3.2.4
export NCS_TOOLCHAIN=/opt/nordic/ncs/toolchains/185bb0e3b6
export ZEPHYR_BASE="$NCS_ROOT/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR="$NCS_TOOLCHAIN/opt/zephyr-sdk"
export PATH="$NCS_TOOLCHAIN/bin:$NCS_TOOLCHAIN/opt/zephyr-sdk/arm-zephyr-eabi/bin:$PATH"
```

Or: `source /path/to/writable_ndef_msg/scripts/env/ncs-env.sh` then `west --version`.
Rarely need `west zephyr-export` with Toolchain Manager; run from `NCS_ROOT` if CMake misses Zephyr packages.

### Two workspace modes

| Mode | Cwd | App |
|------|-----|-----|
| **A — Global NCS** (recommended Mac) | `/opt/nordic/ncs/v3.2.4` | `"$REPO"` (absolute clone path) |
| **B — Repo workspace** (CI-like) | `/path/to/writable_ndef_msg` | `.` after `west init -l . && west update` |

Mode **A**: set `REPO`, source env (above), `cd "$NCS_ROOT"`. Twister `-T` uses `"$REPO/..."` absolute paths.
Mode **B**: `export ZEPHYR_BASE="$(pwd)/zephyr"`; still add Toolchain Manager dirs to `PATH`.

### Build commands (mode A templates)

```bash
export REPO=/path/to/writable_ndef_msg
source "$REPO/scripts/env/ncs-env.sh"
cd "$NCS_ROOT"

west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild

west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160.conf"

west twister -T "$REPO/modules/nfc_pn7160/tests/unit/pn7160_tml" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

west twister -T "$REPO" -t ci_build --integration --build-only -v
```

Mode **B**: same commands from repo root with `.` instead of `"$REPO"` and relative overlay paths.

### Environment checklist

| Variable | Example (global NCS) |
|----------|----------------------|
| `ZEPHYR_BASE` | `/opt/nordic/ncs/v3.2.4/zephyr` |
| `ZEPHYR_TOOLCHAIN_VARIANT` | `zephyr` |
| `ZEPHYR_SDK_INSTALL_DIR` | `/opt/nordic/ncs/toolchains/185bb0e3b6/opt/zephyr-sdk` |
| `PATH` | `.../toolchains/<bundle_id>/bin` + `.../opt/zephyr-sdk/arm-zephyr-eabi/bin` |

### QEMU on Mac

```bash
brew install qemu
qemu-system-arm --version
```

Without QEMU, unit Twister **build** succeeds; **run** fails with `QEMU-NOTFOUND`.

### Common failures

| Symptom | Fix |
|---------|-----|
| `west: command not found` | Toolchain `.../toolchains/<bundle_id>/bin` on `PATH` |
| `QEMU-NOTFOUND` | `brew install qemu`, or `--build-only` |
| Wrong / missing `dtc` | Use Nordic `.../toolchains/<bundle_id>/bin/dtc` |
| `ZEPHYR_BASE` unset | Set to global `.../v3.2.4/zephyr` or repo `$(pwd)/zephyr` |
| Host gcc used | Set `ZEPHYR_SDK_INSTALL_DIR` + arm-zephyr-eabi on `PATH` |

**Multipass/Ubuntu:** optional — only for `native_sim` or exact Linux CI parity; nRF cross-builds and QEMU unit tests work on macOS.


## Commands cheat sheet

Run from **repo workspace root** unless using global NCS (then keep `-T` as absolute path to this repo).

| Goal | Command |
|------|---------|
| Integration (PR scope) | `west twister -T . -t ci_build --integration --build-only -v` |
| Integration (all `ci_build`) | `west twister -T . -t ci_build --build-only -v` |
| Unit (`ci_unit`, matches CI intent) | `west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v` |
| PN7160 overlay smoke (Twister) | `west twister -T . -t pn7160 --build-only -v` |
| Single-board app build | `west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild` |

**Global NCS example (absolute test path):**

```bash
cd /opt/nordic/ncs/v3.2.4
west twister -T /path/to/writable_ndef_msg/modules/nfc_pn7160/tests/unit/pn7160_tml \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v
```

## Twister tag taxonomy

| Tag | Layer | What runs |
|-----|-------|-----------|
| `ci_build` | Integration (`sample.yaml`) | Build-only app + overlay variants |
| `ci_samples_nfc` | Integration | NFC sample subset (with `ci_build`) |
| `pn7160` | Integration / unit | PN7160 overlay builds or module tests |
| `ci_unit` | Unit (`testcase.yaml`) | Ztest on QEMU ARM (no hardware; platform in `testcase.yaml`) |
| `hil` | Unit / HIL | Board-attached tests (future; not in CI yet) |

**Two layers:** integration tests live in root `sample.yaml`; unit tests live under
`modules/<module>/tests/unit/<suite>/testcase.yaml`.

## CI workflows (`.github/workflows/`)

### v1 — build and code quality

| Workflow | Scope | Merge gate |
|----------|-------|------------|
| `twister.yaml` | `ci_build` (integration, build-only) + `ci_unit` (qemu) | **Yes** (`ready-to-merge`) |
| `compliance.yml` | Checkpatch / compliance on product paths only | Recommended |
| `coding_guidelines.yml` | Coccinelle / guideline_check on product paths | Recommended |
| `license_check.yml` | Scancode on staged `scan/` tree (`src/`, `modules/`, `boards/`) | Recommended |
| `clang.yaml` | LLVM `--toolchain llvm` build via Twister | Informational |
| `codeql.yml` | CodeQL on Python + GitHub Actions | Informational |
| `pinned-gh-actions.yml` | Enforce SHA-pinned actions (workflow edits only) | On workflow edits |
| `ready-to-merge.yml` | Reusable gate (called by gated workflows) | — |

### v2 — DT, docs, manifest, metrics

| Workflow | Scope | Merge gate |
|----------|-------|------------|
| `devicetree.yml` | `dtschema` on module bindings + Twister `pn7160` overlay builds | **Yes** |
| `doc-build.yml` | Doxygen HTML + coverxygen JSON for module headers | **Yes** |
| `doxygen-coverage-delta.yml` | Fail PR if new public API lacks Doxygen docs | Recommended |
| `doc-publish-pr.yml` | PR comment with link to doc-build artifact | — |
| `doc-publish.yml` | Deploy API docs to GitHub Pages (push only) | — |
| `manifest.yml` | Validate `west.yml` on PRs (`pull_request_target`) | Recommended |
| `footprint-tracking.yml` | Flash/RAM from `ci_build` → artifact JSON | Informational |
| `codecov.yaml` | Gcov on `ci_unit` / QEMU unit platform | Informational (`CODECOV_TOKEN`) |
| `release.yml` | Draft GitHub release + SPDX on `v*` tags | — |
| `scorecards.yml` | OpenSSF supply-chain score | Informational |

**Skipped (not used):** `errno.yml` (Zephyr kernel libc), `stats_merged_prs.yml`, stale/greet bots, AWS/Elasticsearch publish, upstream `devicetree_checks.yml`.

**Errno convention:** public APIs return `0` or negative POSIX errno per `docs/nfc/NFC_STACK_CONVENTIONS.md` §9 — enforced by review and unit tests, not a separate CI workflow.

### API documentation (local)

Requires Doxygen and `pip install -r doc/requirements.txt`.

```bash
make -C doc html
make -C doc doxygen-coverage-json
open doc/_build/doxygen/html/index.html
```

Coverage delta (matches CI):

```bash
make -C doc doxygen-coverage-json
cp doc/_build/doc-coverage.json /tmp/pr-coverage.json
# ... compare against base branch with scripts/ci/doxygen_coverage_diff.py
```

## PN7160 driver — unit tests to add (Phase 0.2+)

Do **not** expand the placeholder ztest alone; grow this suite as TML lands (see `docs/nfc/NFC_STACK_PLAN.md` gate 1).

| Area | Approach |
|------|----------|
| **TML framing (I2C)** | ztests for 2-byte length header, payload boundaries, `-EINVAL` / `-EIO` paths; compile `pn7160_tml_i2c.c` with **mock or emulated I2C** (no PN7160 silicon) |
| **TML SPI** | Same pattern once SPI backend is implemented (Phase 0.2 gate currently allows `-ENOTSUP`) |
| **NCI helpers** | Pure parse/serialize tests on `pn7160_nci.c` stubs (Phase 0.3+); keep off the hot path until TML send/receive is mockable |
| **Future HIL** | Tag `hil`, real `nxp,pn7160` on DK — not in merge-gate CI yet |

Reference: Phase 0 gate table in `docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md` §10.

## Module discovery

Out-of-tree module `modules/nfc_pn7160/` is wired via:

- Root `CMakeLists.txt` → `ZEPHYR_EXTRA_MODULES`
- Unit test `CMakeLists.txt` → same module path
- `modules/nfc_pn7160/zephyr/module.yml`

No extra west manifest entry is required for Twister to find unit tests under
`-T modules/nfc_pn7160/tests/unit/pn7160_tml`.

## Reference directories (local only)

These paths are **not** part of the product repo. Keep them locally for porting
reference; they are gitignored and excluded from CI scans:

| Path | Purpose |
|------|---------|
| `hals_temp/` | NXP PN7160 NCI example HAL |
| `flipperzero/` | GPL Flipper firmware (protocol facts distilled into plans) |
| `aliro/` | Aliro door-lock reference (PSA patterns) |

Driver specs may cite these paths; clone or unpack them locally as needed.

## Local verification notes (unit scaffold)

Last checked against **NCS v3.2.4** (`/opt/nordic/ncs/v3.2.4`):

| Check | Result |
|-------|--------|
| `west twister … -t ci_unit -p qemu_cortex_m3` (CI / Linux with QEMU) | **Pass** — `testcase.yaml` + `twister.yaml` use `qemu_cortex_m3`; overlay `boards/overlays/pn7160_unit_test.overlay` |
| `west twister … --build-only` on macOS without `qemu-system-arm` | **Build pass** — run step needs QEMU on `PATH` (GitHub `ubuntu-24.04` runners provide it) |

Unit test app sets `EXTRA_DTC_OVERLAY_FILE` in `modules/nfc_pn7160/tests/unit/pn7160_tml/CMakeLists.txt`. Module sources build only when `CONFIG_PN7160` is enabled (devicetree + `prj.conf`).

### Phase 0.2 — next test tasks

1. **I2C TML framing ztests** — header encode/decode and length errors using the I2C emulator node in `pn7160_unit_test.overlay`.
2. **SPI TML ztests** — separate overlay or dual-instance DT when SPI backend tests land.
3. **Mock NCI** — exercise `pn7160_nci.c` without a real controller response.
