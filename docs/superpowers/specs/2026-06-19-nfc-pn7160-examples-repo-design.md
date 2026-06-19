# nfc-pn7160 — PN7160 Zephyr driver + NXP examples repo

**Date:** 2026-06-19
**Status:** Design approved (pending user review). This spec details **Phase 0 (Foundation)** only;
Phases 1–4 are roadmap context and each gets its own deep spec when reached.
**Author:** brainstormed with Claude Code
**Source reference:** `writable_ndef_msg/hals_temp/NXP-NCI2.0_LPC55S6x_examples` (NXP NCI 2.0, LPC55S69)
**Existing driver:** `writable_ndef_msg/modules/nfc_pn7160` (frozen v1.0, READER + CARD)

---

## 1. Goal

Create a **fresh, standalone Zephyr repo** — `nfc-pn7160` — that packages the NXP PN7160 NFC
controller as a reusable, west-importable Zephyr module and ships the **five NXP NCI 2.0 example
programs as Zephyr samples**:

1. Reader/Writer (RW)
2. Reader/Writer + Card Emulation (RWandCE)
3. Card Emulation (CE)
4. Peer-to-Peer (P2P / SNEP)
5. PN7160 firmware update (FWupdate)

The repo is independent of `writable_ndef_msg` — a true fresh start containing *only* the PN7160
driver and the examples. Where building an example needs protocol logic the driver does not provide
(NDEF read/write, T4T emulation, SNEP, firmware download), we **port NXP's own canonical, hardware-
agnostic `NfcLibrary`** onto the Zephyr driver rather than reinventing it.

## 2. Key decisions (from brainstorming)

| Decision | Choice | Rationale |
|---|---|---|
| Repo purpose | Reusable driver module + samples | Others can consume just the driver via west import |
| Relationship to `writable_ndef_msg` | **Fresh start / snapshot** | New repo holds only driver + examples; no dependency back |
| Examples in scope | **All five** | Full parity with NXP reference |
| Protocol-logic strategy | **Canonical NXP port + thin shim** | Fastest path to all 5 at high fidelity; P2P + FW-update come from NXP's proven code |
| Reference board | nRF54L15 DK + PN7160 shield | Reuse existing `pn7160_i2c.overlay` / `pn7160_spi.overlay` |
| Transport | **I2C default + SPI variant** | I2C is the default build; SPI a documented variant |
| Repo name / location | `/Users/majidfaroud/nfc-pn7160` (sibling of `writable_ndef_msg`) | — |
| First milestone | **Phase 0 (Foundation) only** | Keep the first implementation plan small and reviewable |

## 3. Architecture — layering

```
┌──────────────────────────────────────────────────────────┐
│  samples/   (5 Zephyr apps, each ≈ one NXP nfc_example_*.c)│
├──────────────────────────────────────────────────────────┤
│  nxp_compat/   (ported, hardware-agnostic NXP NfcLibrary)  │
│   RW_NDEF_T1T..T5T · T4T_NDEF_emu · P2P_NDEF ·             │
│   NxpNci_Process{Reader,Card,P2p}Mode  +  Nfc.h types      │
│   ── shim: NxpNci_*  →  pn7160_nci_*  ─────────────────┐   │
├────────────────────────────────────────────────────────┼──┤
│  driver:  nfc_pn7160  (snapshot, canonical here)       ▼  │
│   pn7160_nci_* · TML i2c/spi · DWL · discovery · card     │
├──────────────────────────────────────────────────────────┤
│  Zephyr / NCS v3.2.4  (I2C, SPI, GPIO, kernel)            │
└──────────────────────────────────────────────────────────┘
```

- **Driver layer** (already implemented in `modules/nfc_pn7160`): the NCI/transport half —
  `pn7160_nci_connect/configure_settings/configure_mode/discovery_*/reader_tag_cmd/
  card_mode_send/recv/listen_*`, plus TML I2C/SPI framing, VEN reset, and DWL download mode.
- **`nxp_compat` layer** (ported in later phases): NXP's `NfcLibrary` — the NDEF read/write tables
  (`RW_NDEF_T1T..T5T`), T4T card-emulation (`T4T_NDEF_emu`), peer-to-peer (`P2P_NDEF`), and the
  `NxpNci_Process{Reader,Card,P2p}Mode` orchestration extracted from `NxpNci20.c`. It is hardware-
  agnostic *except* its bottom edge, which a **shim** re-points from NXP's transport calls to the
  Zephyr `pn7160_nci_*` API. The shim also re-exposes NXP's `Nfc.h` types/macros
  (`NxpNci_RfIntf_t`, `PROT_T2T`, `TECH_PASSIVE_NFCA`, …) so the ported `RW_NDEF*`/`P2P_NDEF`/
  example sources compile unmodified.
- **Samples layer**: each NXP `nfc_example_*.c` adapted as a Zephyr `main()` plus `prj.conf`,
  devicetree overlay, and `sample.yaml`.

### Shim mapping (target — implemented incrementally per phase)

| NXP API (used by NfcLibrary / examples) | Zephyr driver API |
|---|---|
| `NxpNci_Connect` / `NxpNci_Disconnect` | `pn7160_nci_connect` + `pn7160_reset` |
| `NxpNci_GetFwVersion` | `pn7160_fw_version_get` |
| `NxpNci_ConfigureSettings` | `pn7160_nci_configure_settings` |
| `NxpNci_ConfigureMode` | `pn7160_nci_configure_mode` |
| `NxpNci_StartDiscovery` / `StopDiscovery` | `pn7160_nci_discovery_start` / `_stop` |
| `NxpNci_WaitForDiscoveryNotification` | `pn7160_nci_discovery_wait` (+ map `pn7160_nci_rf_intf` → `NxpNci_RfIntf_t`) |
| `NxpNci_ReaderTagCmd` | `pn7160_nci_reader_tag_cmd` |
| `NxpNci_CardModeSend` / `CardModeReceive` | `pn7160_nci_card_mode_send` / `_recv` |
| `NxpNci_ReaderActivateNext` / `ReaderReActivate` | **driver gap** — add or stub (Phase 1/3 risk) |
| `NxpNci_Process{Reader,Card,P2p}Mode` | ported into `nxp_compat` (call the above + RW_NDEF/T4T_emu/P2P_NDEF) |

## 4. Repo topology (target — full vision)

```
nfc-pn7160/                      ← new repo root = the Zephyr module
├── west.yml                      ← T2 star: self + import sdk-nrf v3.2.4 (same pin as today)
├── zephyr/
│   ├── module.yml                ← cmake: . / kconfig: zephyr/Kconfig / dts_root: .
│   └── Kconfig                   ← PN7160 driver Kconfig (snapshot)
├── Kconfig                       ← repo-level (sources zephyr/Kconfig + nxp_compat/Kconfig)
├── CMakeLists.txt                ← driver library wiring (snapshot)
├── include/nfc/                  ← pn7160.h, pn7160_tml.h (snapshot)
├── src/                          ← pn7160_driver.c + nci_* + tml_* + shell + priv.h (snapshot)
├── dts/bindings/nfc/             ← nxp,pn7160-common/-i2c/-spi.yaml (snapshot)
├── nxp_compat/                   ← ported NfcLibrary + shim (Kconfig-gated, OFF by default)
│   ├── Kconfig                   ← NXP_NCI_COMPAT + sub-options (RW/CE/P2P/FW)
│   ├── CMakeLists.txt
│   ├── include/                  ← Nfc.h, NxpNci_compat.h, RW_NDEF.h, T4T_NDEF_emu.h, P2P_NDEF.h
│   └── src/                      ← shim + RW_NDEF_T*.c + T4T_NDEF_emu.c + P2P_NDEF.c + Process*.c
├── boards/overlays/              ← pn7160_i2c.overlay, pn7160_spi.overlay (snapshot)
├── samples/
│   ├── pn7160_shell/             ← Phase 0 smoke app (driver + shell only)
│   ├── reader_writer/            ← Phase 1
│   ├── reader_and_card/          ← Phase 2
│   ├── card_emulation/           ← Phase 2
│   ├── peer_to_peer/             ← Phase 3
│   └── fw_update/                ← Phase 4
├── tests/unit/pn7160_tml/        ← driver TML/NCI unit tests (snapshot)
├── .github/workflows/twister.yaml← CI: build samples + run unit tests
└── README.md
```

## 5. Phasing roadmap

| Phase | Deliverable | Notes / risk |
|---|---|---|
| **0** | **Foundation** — repo scaffold, driver snapshot, build/module/west wiring, board overlays, `nxp_compat` skeleton (types + shim stubs, OFF by default), `pn7160_shell` smoke sample, unit tests, CI, README | **This spec.** Nothing protocol-level yet; gate is "everything builds + unit tests pass" |
| 1 | Reader/Writer sample — shim impl + `RW_NDEF*` port + `reader_writer` | First end-to-end proof; risk: `ReaderActivateNext/ReActivate` driver gap |
| 2 | Card emulation — `T4T_NDEF_emu` port + `card_emulation` + `reader_and_card` | Uses driver card mode (done) |
| 3 | P2P — `P2P_NDEF`/SNEP/LLCP port + `peer_to_peer` | Separate subsystem; risk: NFC-DEP discovery config the driver may not expose; P2P/LLCP is NFC-Forum-deprecated |
| 4 | FW update — DWL flow + firmware-image handling + `fw_update` | Separate subsystem; DWL primitives exist, needs `sFWupdate` logic |

Each of Phases 1–4 gets its own brainstorm → spec → plan cycle.

---

## 6. Phase 0 — Foundation (detailed, implementable scope)

**Objective:** stand up the new repo so that the driver builds as a self-discovered Zephyr module,
a minimal application builds end-to-end on the reference board for **both** I2C and SPI, the driver
unit tests pass under twister, and the `nxp_compat` skeleton compiles when enabled — with **no
protocol logic yet**.

### 6.1 Repo initialization
- Create `/Users/majidfaroud/nfc-pn7160`, `git init`, default branch `main`.
- Add `LICENSE` (Apache-2.0, matching the module's SPDX headers) and `.gitignore` (`build/`,
  `.west/`, `*.pyc`, `.cache/`).

### 6.2 Driver snapshot (copy verbatim from `writable_ndef_msg/modules/nfc_pn7160`)
- `include/nfc/pn7160.h`, `include/nfc/pn7160_tml.h`
- `src/pn7160_driver.c`, `pn7160_tml_framing.c`, `pn7160_nci_parse.c`, `pn7160_nci.c`,
  `pn7160_nci_settings.c`, `pn7160_nci_discovery.c`, `pn7160_nci_card_mode.c`,
  `pn7160_tml_i2c.c`, `pn7160_tml_spi.c`, `pn7160_shell.c`, `pn7160_priv.h`
- `dts/bindings/nfc/nxp,pn7160-common.yaml`, `-i2c.yaml`, `-spi.yaml`
- `zephyr/Kconfig`, `CMakeLists.txt` (unchanged — already root-relative)
- `CONTEXT.md`, driver `README.md` content folded into the new repo README

### 6.3 Build / module / workspace wiring
- `zephyr/module.yml` — snapshot (`cmake: .`, `kconfig: zephyr/Kconfig`, `dts_root: .`). Because the
  manifest/self repo carries `zephyr/module.yml`, west auto-discovers it as a module — **no
  `ZEPHYR_EXTRA_MODULES` needed** (unlike the in-tree `modules/` layout today).
- `west.yml` — T2 star topology:
  ```yaml
  manifest:
    version: "0.13"
    self:
      path: .            # repo is the NCS workspace root, matching writable_ndef_msg today
    remotes:
      - name: ncs
        url-base: https://github.com/nrfconnect
    defaults:
      remote: ncs
    projects:
      - name: nrf
        repo-path: sdk-nrf
        revision: v3.2.4
        import: true
  ```
- Root `Kconfig` — sources `zephyr/Kconfig` (driver) and `nxp_compat/Kconfig`.

### 6.4 Board overlays (snapshot from `writable_ndef_msg/boards/overlays`)
- `boards/overlays/pn7160_i2c.overlay` (default), `pn7160_spi.overlay`,
  and the unit-test overlay (`pn7160_unit_test.overlay`) for the test build.

### 6.5 `nxp_compat` skeleton (compiles, OFF by default)
- `nxp_compat/Kconfig`: `NXP_NCI_COMPAT` (bool, default n, `depends on PN7160`) with sub-stubs
  `NXP_NCI_COMPAT_RW / _CE / _P2P / _FW` (all default n) for later phases.
- `nxp_compat/include/Nfc.h`: the NXP type/macro definitions re-exposed (`NxpNci_RfIntf_t` and
  friends, `PROT_*`, `TECH_*`, `MODE_*`, `INTF_*`, `NXPNCI_MODE_*`) — copied from the reference,
  cleaned of LPC includes.
- `nxp_compat/include/NxpNci_compat.h`: declarations of the shim functions (the left column of the
  §3 mapping table).
- `nxp_compat/src/nxp_nci_shim.c`: **stub** bodies that map to `pn7160_nci_*` for the calls that
  already have a 1:1 driver counterpart (connect, settings, mode, discovery, reader_tag_cmd,
  card_mode_send/recv, fw_version). Gaps (`ReaderActivateNext`/`ReActivate`) return `NFC_ERROR`
  with a `// TODO Phase 1` marker. No `RW_NDEF*`/`T4T`/`P2P` sources yet.
- `nxp_compat/CMakeLists.txt`: compile the shim only when `CONFIG_NXP_NCI_COMPAT`.
- **Acceptance for this unit:** building any sample with `CONFIG_NXP_NCI_COMPAT=y` compiles and
  links the shim cleanly (proves the type/shim boundary is sound before Phase 1 piles protocol
  code on it).

### 6.6 Phase-0 smoke sample — `samples/pn7160_shell`
- `src/main.c`: bring up the PN7160 device (`DEVICE_DT_GET(DT_NODELABEL(pn7160))`,
  `device_is_ready`), print a banner, idle (`k_sleep(K_FOREVER)`). Driver + `pn7160` shell do the
  rest interactively (`pn7160 probe`, `init`, `settings`, `discovery start`, …).
- `prj.conf`: `CONFIG_PN7160=y`, `CONFIG_SHELL=y`, `CONFIG_PN7160_SHELL=y`, UART/console + log.
- Default build uses `boards/overlays/pn7160_i2c.overlay`; SPI documented as a variant.
- `sample.yaml`: `build_only` twister entries for I2C and SPI on `nrf54l15dk/nrf54l15/cpuapp`.

### 6.7 Tests (snapshot)
- `tests/unit/pn7160_tml/` (CMakeLists, prj.conf, src/main.c, testcase.yaml) copied verbatim.
- Confirm the unit-test build path resolves the module from the new repo root (it should, via the
  same module-discovery mechanism).

### 6.8 CI
- `.github/workflows/twister.yaml`: install west, `west init -l`, `west update`, then
  `west twister -T samples -T tests` for `nrf54l15dk/nrf54l15/cpuapp`. Mirror the structure of the
  existing `writable_ndef_msg` twister workflow, trimmed to this repo.

### 6.9 README
- Repo overview, layering diagram, the §5 phase table (so contributors see the roadmap), and
  build/flash/shell instructions for the smoke sample (I2C default + SPI variant), plus how to
  consume the module via west import.

### 6.10 Phase 0 acceptance criteria (the gate)
1. `west build -b nrf54l15dk/nrf54l15/cpuapp samples/pn7160_shell` succeeds with the **I2C**
   overlay.
2. The same sample builds with the **SPI** overlay.
3. `west twister -T tests` passes the `pn7160_tml` unit tests.
4. A build with `CONFIG_NXP_NCI_COMPAT=y` compiles and links the `nxp_compat` shim skeleton.
5. CI workflow runs items 1–3 green.
6. README documents build + shell usage.

### 6.11 Phase 0 risks / notes
- **Module self-discovery:** the self/manifest repo must be recognized as a Zephyr module via its
  root `zephyr/module.yml`. If west does not auto-discover it in this topology, fall back to
  `ZEPHYR_EXTRA_MODULES` (as `writable_ndef_msg` does today). *Verify early.*
- **Snapshot drift:** this is a one-time copy; the driver now has two homes (`writable_ndef_msg`
  and `nfc-pn7160`). Accepted per the "fresh start" decision; revisit consolidation later.
- **Uncommitted WIP in source repo:** snapshot the **current working-tree** state of
  `modules/nfc_pn7160` (it has local modifications), not `HEAD`.
- **License/headers:** preserve SPDX `Apache-2.0` headers; NXP `NfcLibrary` (later phases) carries
  NXP's proprietary header — confirm redistribution terms before porting in Phase 1+.

## 7. Out of scope for Phase 0
- Any NDEF / T4T-emulation / SNEP / firmware-update logic (Phases 1–4).
- Refactoring `writable_ndef_msg` to consume the new module.
- Hardware-in-the-loop test automation (samples are `build_only` in CI).
