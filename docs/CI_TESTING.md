# CI and local testing

NCS workspace root (`west.yml` pins **v3.2.4**, same as `.github/workflows/twister.yaml`).

For west workspace layout, official doc links, and bootstrap commands, see
[ZEPHYR_WORKSPACE.md](ZEPHYR_WORKSPACE.md).

## Reference directories (local only)

These paths are **not** part of the product repo. Keep them locally for porting
reference; they are gitignored and excluded from CI scans:

| Path | Purpose |
|------|---------|
| `hals_temp/` | NXP PN7160 NCI example HAL |
| `flipperzero/` | GPL Flipper firmware (protocol facts distilled into plans) |
| `aliro/` | Aliro door-lock reference (PSA patterns) |

Driver specs may cite these paths; clone or unpack them locally as needed.

## Twister tag taxonomy

| Tag | Layer | What runs |
|-----|-------|-----------|
| `ci_build` | Integration (`sample.yaml`) | Build-only app + overlay variants |
| `ci_samples_nfc` | Integration | NFC sample subset (with `ci_build`) |
| `pn7160` | Integration / unit | PN7160 overlay builds or module tests |
| `ci_unit` | Unit (`testcase.yaml`) | Ztest on `qemu_cortex_m33` (no hardware) |
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
| `codecov.yaml` | Gcov on `ci_unit` / `qemu_cortex_m33` | Informational (`CODECOV_TOKEN`) |
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

## Local commands (Linux / Multipass)

Requires a west workspace (`west init -l . && west update`) and Zephyr SDK.

```bash
# Integration — build-only (matches CI PR step)
west twister -T . -t ci_build --integration --build-only -v

# Unit — PN7160 TML scaffold on QEMU (matches CI)
west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml \
  -p qemu_cortex_m33 --no-sysbuild -t ci_unit -v

# All ci_build targets (push/nightly scope)
west twister -T . -t ci_build --build-only -v
```

### macOS vs Multipass

| Target | macOS | Linux / Multipass |
|--------|-------|-------------------|
| `ci_build` on nRF DK boards | Yes (cross-compile) | Yes |
| `ci_unit` on `qemu_cortex_m33` | Yes | Yes |
| `native_sim` / `native_sim/native` | **No** | Yes |

Use **Multipass** (or another Linux VM) on Mac for `native_sim` unit tests.
CI runs `qemu_cortex_m33` only — no hardware, no `native_sim` on GitHub runners
for this repo yet.

```bash
# Linux / Multipass only (not run in CI today):
# west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml \
#   -p native_sim --no-sysbuild -t ci_unit -v
```

## Module discovery

Out-of-tree module `modules/nfc_pn7160/` is wired via:

- Root `CMakeLists.txt` → `ZEPHYR_EXTRA_MODULES`
- Unit test `CMakeLists.txt` → same module path
- `modules/nfc_pn7160/zephyr/module.yml`

No extra west manifest entry is required for Twister to find unit tests under
`-T modules/nfc_pn7160/tests/unit/pn7160_tml`.
