# Zephyr / NCS workspace layout

This repo is an **NCS workspace application** in Zephyr **T2 topology**: the Git
repository is both the west **manifest repository** and the **application** root
(`west.yml` → `self.path: .`).

Official references below use Zephyr “latest” doc URLs (stable across NCS 3.2.x);
pin in this repo is **NCS v3.2.4** (`west.yml` / CI).

## Official documentation

| Topic | Zephyr doc | NCS doc (v3.2.4) |
|-------|------------|------------------|
| West workspace, topologies (T1/T2/T3) | [West workspaces](https://docs.zephyrproject.org/latest/develop/west/workspaces.html) — [T2: app is manifest](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository) | [Creating an application — workspace type](https://docs.nordicsemi.com/bundle/ncs-3.2.4/page/nrf/app_dev/create_application.html#create_application_types_workspace) |
| `west init`, `west update`, manifest terms | [West built-in commands](https://docs.zephyrproject.org/latest/develop/west/built-in.html) — [West manifests](https://docs.zephyrproject.org/latest/develop/west/manifest.html) | [Installing the NCS](https://docs.nordicsemi.com/bundle/ncs-3.2.4/page/nrf/installation/install_ncs.html) |
| Out-of-tree app (`west init -l`) | [Application development](https://docs.zephyrproject.org/latest/develop/application/index.html) — [workspace application](https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app) | same NCS create-application page |
| Out-of-tree module (`zephyr/module.yml`, `EXTRA_ZEPHYR_MODULES`) | [Modules](https://docs.zephyrproject.org/latest/develop/modules.html) — [module.yml](https://docs.zephyrproject.org/latest/develop/modules.html#module-yml) — [build integration](https://docs.zephyrproject.org/latest/develop/modules.html#integrate-modules-in-zephyr-build-system) | — |
| Devicetree bindings in modules (`dts_root`) | [Bindings intro — where bindings live](https://docs.zephyrproject.org/latest/build/dts/bindings-intro.html) — [module build settings](https://docs.zephyrproject.org/latest/develop/modules.html#build-settings) | — |
| Twister `sample.yaml` / `testcase.yaml` | [Twister](https://docs.zephyrproject.org/latest/develop/test/twister.html) | — |

**Terminology (west):**

- **West workspace** — directory tree containing `.west/` and all cloned **projects**.
- **Manifest repository** — Git repo that owns `west.yml` (here: this repo).
- **West project** — one Git clone listed under `manifest.projects` (e.g. `nrf`, `zephyr`).

**Where SDK deps are fetched:** `west update` reads `west.yml`, clones/updates
projects. This repo lists only `nrf` (`sdk-nrf` @ **v3.2.4**) with `import: true`,
so `west update` also pulls everything in Nordic’s manifest — typically
`zephyr/`, `nrfxlib/`, `bootloader/`, and west-managed entries under `modules/`
(from Zephyr/NCS manifests). Those live as **sibling directories**, not inside
this application tree.

## This repo mapped to Zephyr terms

| Path / artifact | Zephyr / NCS role |
|-----------------|-------------------|
| `writable_ndef_msg/` (repo root) | T2 **manifest repository** + **workspace application** (`self.path: .`) |
| `west.yml` | Manifest; pins `nrf` @ v3.2.4; `import: true` pulls Zephyr + NCS modules |
| `CMakeLists.txt`, `prj.conf`, `src/` | Application sources |
| `sample.yaml` | Twister **integration** config (build-only `ci_build` targets) |
| `modules/nfc_pn7160/` | **Out-of-tree Zephyr module** (not a west project) |
| `modules/nfc_pn7160/zephyr/module.yml` | Module metadata (`name`, `cmake`, `kconfig`, `dts_root`) |
| Root `CMakeLists.txt` → `ZEPHYR_EXTRA_MODULES` | Explicit module discovery (same path for unit tests) |
| `modules/nfc_pn7160/dts/bindings/` | Module devicetree bindings (via `dts_root: .`) |
| `modules/nfc_pn7160/tests/unit/.../testcase.yaml` | Twister **unit** config (`ci_unit` on `qemu_cortex_m33`) |
| `.github/workflows/` | **Project CI** (Twister, compliance, CodeQL) — not part of upstream Zephyr tree |
| `hals_temp/`, `flipperzero/`, `aliro/` | Local reference only (gitignored; see `docs/CI_TESTING.md`) |

After `west update`, expect a layout like:

```
writable_ndef_msg/          ← manifest + app (this repo)
├── .west/
├── west.yml
├── src/  sample.yaml  …
├── modules/nfc_pn7160/     ← product module (in-repo)
├── nrf/                    ← west project (sdk-nrf v3.2.4)
├── zephyr/                 ← imported via nrf
├── nrfxlib/                ← imported via nrf
├── bootloader/             ← imported via nrf
└── modules/                ← west-fetched SDK modules (lib/, hal/, …)
```

## Bootstrap from scratch

Prerequisites: Python 3, `west`, [Zephyr SDK](https://github.com/zephyrproject-rtos/sdk-ng) (arm-zephyr-eabi), git.

```bash
git clone <your-remote>/writable_ndef_msg.git
cd writable_ndef_msg

# T2: existing manifest repo at repo root (west.yml self.path: .)
west init -l .
west update

# Optional: match CI toolchain env
export ZEPHYR_BASE="$(pwd)/zephyr"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

# Build (example — nRF54L15 DK)
west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild

# Twister — integration (matches CI PR)
west twister -T . -t ci_build --integration --build-only -v

# Twister — unit (PN7160 TML scaffold on QEMU)
west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml \
  -p qemu_cortex_m33 --no-sysbuild -t ci_unit -v
```

PN7160 overlay variants are defined in root `sample.yaml` (`pn7160.i2c` / `pn7160.spi`).

## NCS-specific notes

- **Pin:** `west.yml` → `nrf` revision **v3.2.4** (same as `.github/workflows/twister.yaml` `NCS_REVISION`).
- **Import:** `import: true` on the `nrf` project avoids hand-maintaining `zephyr`, `nrfxlib`, and module SHAs — Nordic’s `nrf/west.yml` at that tag is the source of truth.
- **Toolchain install:** CI uses `zephyrproject-rtos/action-zephyr-setup` with `app-path: .`; locally, install the SDK or use Toolchain Manager / nRF Connect for VS Code.
- **Sysbuild:** NCS enables sysbuild by default; integration tests in `sample.yaml` set `sysbuild: true`.
- **Module env var:** Zephyr docs prefer `EXTRA_ZEPHYR_MODULES` (CMake or env); this repo sets `ZEPHYR_EXTRA_MODULES` in `CMakeLists.txt` — equivalent for out-of-tree modules.
- **Preinstalled NCS tree:** If you already have `/opt/nordic/ncs/v3.2.4/`, you can still use this repo as the workspace root via `west init -l . && west update`; do not build from inside `nrf/samples/nfc/writable_ndef_msg` — that upstream path is the old in-tree sample location (`README.rst` sample path).

See also [CI and local testing](CI_TESTING.md) for workflow matrix and tag taxonomy.
