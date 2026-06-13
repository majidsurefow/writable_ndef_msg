# CI and local testing

NCS workspace root (`west.yml` pins **v3.2.4**, same as `zephyr_CICD/workflows/twister.yaml`).

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

| Workflow | Scope |
|----------|-------|
| `twister.yaml` | `ci_build` (integration, build-only) + `ci_unit` (qemu) |
| `compliance.yml` | Checkpatch / compliance on product paths only |
| `license_check.yml` | Scancode on staged `scan/` tree (`src/`, `modules/`, `boards/`) |

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
