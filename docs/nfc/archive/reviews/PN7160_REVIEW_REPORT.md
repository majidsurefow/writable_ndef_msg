# PN7160 Driver Review Report

**Date:** 2026-06-13  
**Reviewer:** Cursor agent (automated code review)  
**Branch / commits:** `906cb9a` (shell DWL + tests), `23ab1d9` (DWL enter/leave + TML mode)  
**References:** ENS160/BMM150 (NCS v3.2.4), NXP `tml.c` / `NxpNci20.c` (`hals_temp/`), [`PN7160_REVIEW_CHECKLIST.md`](PN7160_REVIEW_CHECKLIST.md)

---

## Executive summary

| | |
|---|---|
| **Ready for Phase 1?** | **No** |
| **CI unit tests (`ci_unit`)** | **PASS** — 8/8 on `qemu_cortex_m3` (Twister 2026-06-13) |
| **Integration builds (I2C/SPI)** | Not run in this review (Twister `ci_build` + overlays) |
| **HIL (probe / fwver)** | Not evidenced |

### Phase 1 blockers

1. **No HAL wiring** — `nfc_transport` / `DEVICE_DT_GET` integration absent; PN7160 driver is not reachable from the NFC stack.
2. **No HIL sign-off** — `pn7160 probe` / `pn7160 fwver` not validated on nRF54L15 + eval shield (I2C minimum per checklist §9).
3. **NCI bring-up incomplete** — `CORE_INIT`, `ConfigureSettings`, discovery, and anti-tearing `CORE_GENERIC_ERROR_NTF` handling not ported (required before reader/NDEF work).
4. **Concurrent `pn7160_nci_transceive` unsafe** — no API-level mutex; overlapping callers corrupt `rx_waiting` / `rx_sem`.
5. **Phase 0.8/0.9 items untracked** — discovery / `nfc reader scan` still open without linked issues.

---

## 1. Zephyr driver patterns (vs ENS160 / BMM150)

**Verdict: PASS** (with warnings)

| Pattern | Status | Evidence |
|---|---|---|
| `DT_DRV_COMPAT` + `DT_INST_FOREACH_STATUS_OKAY` | PASS | `pn7160_driver.c:8`, `288` |
| `DT_INST_ON_BUS` I2C/SPI via `COND_CODE_1` | PASS | `pn7160_driver.c:281–283` (same idiom as `bmm150.c:708–711`) |
| `config` const / `data` per-instance | PASS | `pn7160_priv.h:13–41`, `PN7160_DEFINE` |
| `LOG_MODULE_REGISTER` + `LOG_MODULE_DECLARE` | PASS | `pn7160_driver.c:18`, TML/NCI files declare |
| `POST_KERNEL` init; bus `*_is_ready_dt` | PASS | `pn7160_driver.c:207–210`, `285–286` |
| `gpio_is_ready_dt` on IRQ/VEN | PASS | `pn7160_driver.c:212–214` |
| ISR: `CONTAINER_OF`, atomic, `k_work` only | PASS | `pn7160_driver.c:86–95` — no bus I/O in ISR |
| errno returns in init | PASS | `pn7160_init` returns `-ENODEV`/`-EIO`/GPIO errno |
| `device_is_ready()` documented | PASS | `pn7160.h:30–31`, shell checks |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| No bus I/O vtable (`bus_io` struct) like BMM150 | `pn7160_priv.h:23–24` | Optional refactor Phase 2+; function pointers in `config` are acceptable for Phase 0. |
| No `PM_DEVICE` hooks | `pn7160_driver.c` | Defer unless product needs suspend; document as intentional. |
| `irq_work` uses **system** work queue, `irq_rx_work` uses module WQ | `pn7160_driver.c:94`, `62` | Submit `irq_rx_work` directly from ISR via deferred pattern, or document two-hop latency; ens160 uses same `k_work_submit` hop. |
| CMake ignores `CONFIG_PN7160_TML_I2C` / `_SPI` | `CMakeLists.txt:11–12` | Gate `zephyr_library_sources` on Kconfig symbols to avoid empty TUs when only one bus is enabled. |

---

## 2. IRQ safety

**Verdict: WARN**

| Check | Status | Evidence |
|---|---|---|
| No bus I/O in ISR | PASS | `pn7160_gpio_isr` only sets atomic + submits work (`pn7160_driver.c:86–95`) |
| `GPIO_INT_EDGE_TO_ACTIVE` | PASS | `pn7160_driver.c:249` |
| Optional IRQ disable during drain (ENS160) | **WARN** | ENS160 `ens160_trigger.c:15–21` disables edge before bus read; PN7160 leaves IRQ enabled — spurious edges can queue extra `irq_work` while HOST_IRQ is asserted. |
| `pn7160_wait_irq()` for debug polling | PASS | `pn7160_driver.c:177–190` |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| IRQ not masked during TML recv drain | `pn7160_driver.c:37–56` | Add `gpio_pin_interrupt_configure_dt(..., GPIO_INT_DISABLE)` at start of `pn7160_irq_rx_work_handler`, re-enable after `k_sem_give` (ENS160 pattern). |
| `irq_pending` only consumed in `arm_rx` / `wait_irq`, not in ISR drain path | `pn7160_nci.c:73–75` | Document contract; consider clearing `irq_pending` after successful recv to avoid stale re-submits. |

No **FAIL** items; race between fast IRQ and `pn7160_nci_arm_rx` is mitigated by `irq_pending` re-submit in `arm_rx` (`pn7160_nci.c:73–75`).

---

## 3. Work queue safety

**Verdict: WARN**

| Check | Status | Evidence |
|---|---|---|
| Dedicated module WQ for bus I/O | PASS | `pn7160_driver.c:20–34`, `62` |
| Stack size Kconfig | PASS | `CONFIG_PN7160_WORKQ_STACK_SIZE` default 1024 |
| Bus mutex on TML send/recv | PASS | `pn7160_tml_i2c.c:46–53`, `pn7160_tml_spi.c:123–130` |
| `rx_sem` for transceive completion | PASS | `pn7160_nci.c:78–91` |
| `rx_waiting` atomic gate | PASS | `pn7160_driver.c:44–46` |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| No API mutex on `pn7160_nci_transceive` / `check_dev_pres` | `pn7160_nci.c:104–167` | Add `k_mutex` (or document single-thread contract) before Phase 1 multi-command shell/HAL use. |
| `k_work` reentrancy: duplicate `irq_rx_work` submissions | `pn7160_driver.c:58–63` | Benign no-op when `rx_waiting==0`; IRQ-disable pattern reduces duplicate submissions. |
| Module WQ shared across all PN7160 instances | `pn7160_driver.c:20–22` | OK for single-instance boards; document limitation or per-instance WQ if multi-NFC planned. |

---

## 4. Kconfig / Devicetree integration

**Verdict: PASS** (with warnings)

| Check | Status | Evidence |
|---|---|---|
| `CONFIG_PN7160` depends on `DT_HAS_NXP_PN7160_ENABLED` | PASS | `zephyr/Kconfig:5–8` |
| Auto-select I2C/SPI from DT bus | PASS | `Kconfig:9–10`, `PN7160_TML_I2C`/`_SPI` `def_bool` |
| I2C overlay mental build | PASS | `pn7160_i2c.overlay` + `overlay-pn7160.conf` — `CONFIG_I2C`, `CONFIG_PN7160`, `CONFIG_PN7160_TML_I2C=y` |
| SPI overlay mental build | PASS | `pn7160_spi.overlay` + `overlay-pn7160-spi.conf` — `CONFIG_SPI`, DT SPI child, `spi-max-frequency=500000` |
| Bindings split I2C/SPI + common | PASS | `nxp,pn7160-i2c.yaml`, `nxp,pn7160-spi.yaml`, `nxp,pn7160-common.yaml` |
| `module.yml` dts_root | PASS | `zephyr/module.yml:9` |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| Overlays omit `dwl-gpios` | `boards/overlays/pn7160_*.overlay` | Add optional DWL pin for FW-update HIL; document pin in binding example. |
| `overlay-pn7160-spi.conf` omits explicit `CONFIG_PN7160_TML_SPI` | `overlay-pn7160-spi.conf` | Harmless (DT `def_bool`); add comment or symbol for clarity. |
| Kconfig help on `PN7160_SHELL` omits `dwl` subcommands | `zephyr/Kconfig:52–57` | Extend help text to list `pn7160 dwl enter/leave/status`. |

---

## 5. I2C TML vs NXP `tml.c`

**Verdict: PASS**

| NXP behavior | Zephyr | Match |
|---|---|---|
| `tml_Tx`: write + 10 ms retry | `pn7160_tml_i2c_write` `k_msleep(10)` retry | PASS (`pn7160_tml_i2c.c:25–34`) |
| `tml_Rx`: read `HEADER_SZ+1`, then payload+footer | `i2c_read_dt` header then payload (`pn7160_tml_i2c.c:85–107`) | PASS |
| `HEADER_SZ` 2 normal / 1 DWL | `pn7160_tml_header_sz_get` | PASS (`pn7160_tml.h:73–76`) |
| `FOOTER_SZ` 0 normal / 2 DWL | `pn7160_tml_footer_sz_get` | PASS |
| Frame bounds `<= buffLen` | `pn7160_tml_frame_len_validate_mode` | PASS + ztest |
| I2C addr 0x28 | `pn7160_i2c.overlay:11` | PASS (HIL not run) |
| Bus mutex (NXP single-threaded) | `k_mutex` on send/recv | PASS (Zephyr improvement) |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| NXP `tml_Receive` waits on IRQ GPIO before read; Zephyr relies on IRQ WQ | architectural | Document difference; debug path `pn7160_wait_irq` exists but unused in TML recv. |

---

## 6. SPI TML vs NXP

**Verdict: PASS**

| NXP behavior | Zephyr | Match |
|---|---|---|
| Write prefix `0x7F` | `PN7160_TML_SPI_WRITE_PREFIX` + `buf[0]` | PASS (`pn7160_tml_spi.c:72–75`) |
| Read dummy `0xFF`, shift by 1 | `pn7160_tml_spi_read` | PASS (`pn7160_tml_spi.c:95–106`) |
| 10 µs post-read delay | `k_busy_wait(10)` | PASS (`pn7160_tml_spi.c:108`) |
| CS held per segment | `spi_write_dt` / `spi_transceive_dt` per segment | PASS (verify on LA HIL) |
| Send retry 10 ms | `k_msleep(10)` in write/transceive | PASS |
| DWL framing on recv | `dwl_mode` in `pn7160_tml_spi_recv` | PASS (`pn7160_tml_spi.c:139–183`) |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| NXP `INTF_WRITE` uses TX-only SPI; Zephyr `spi_write_dt` | `pn7160_tml_spi.c:77` | Equivalent on most controllers; confirm no MISO glitch on HIL LA trace. |
| Stack buffer cap 256+1 bytes | `PN7160_SPI_STACK_BUF_MAX` | Matches NXP `temp[257]`; document max frame size. |

---

## 7. DWL vs NXP Enter/Leave + download framing

**Verdict: PASS** (commits `23ab1d9`, `906cb9a`)

| NXP `tml.c` | Zephyr | Match |
|---|---|---|
| `tml_EnterDwlMode`: `isDwlMode=true`, DWL high, `tml_Reset` | `pn7160_dwl_enter` (`pn7160_driver.c:134–150`) | PASS |
| `tml_LeaveDwlMode`: `isDwlMode=false`, DWL low, reset | `pn7160_dwl_leave` (`pn7160_driver.c:152–168`) | PASS |
| `HEADER_SZ=1`, `FOOTER_SZ=2` when DWL | `pn7160_tml_*_mode` helpers | PASS + ztest `test_dwl_*` |
| VEN 10 ms / 10 ms on reset | `pn7160_reset` | PASS |
| Shell `pn7160 dwl *` | `pn7160_shell.c:106–174` | PASS |
| Binary FW transfer | Not implemented | Deferred (documented in README) — OK for Phase 0 |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| `dwl_mode` is RAM-only; lost on `pn7160_reset` from shell without re-enter | `pn7160_data.dwl_mode` | Document that VEN-only reset preserves flag but full re-init clears it; DWL GPIO state is HW truth. |
| No HIL for DWL enter/leave | — | Run scope/LA on eval shield before FW-update work. |

---

## 8. NCI vs `NxpNci20.c` — Phase 1 gaps

**Verdict: WARN** (Phase 0 probe scope OK; Phase 1 needs more)

| Capability | NXP | Zephyr | Status |
|---|---|---|---|
| `NxpNci_CheckDevPres` / CORE_RESET `{20 00 01 01}` | Yes | `pn7160_nci.c:26`, `134–161` | PASS |
| CORE_RESET_RSP `40 00` status | Yes | `pn7160_nci_parse_core_reset_rsp` | PASS |
| CORE_RESET_NTF FW bytes 9–11 | Yes | `pn7160_nci_parse.c:25–27` | PASS |
| IRQ-driven receive (not busy-poll GPIO) | `tml_WaitForRx` polls IRQ pin | sem + WQ | PASS (architectural improvement) |
| `CORE_GENERIC_ERROR_NTF` `60 07` anti-tearing | Yes (`NxpNci20.c:51–55`) | Not handled | **WARN** |
| `NxpNci_Connect` / CORE_INIT `{20 01 02 00 00}` | Yes | Missing | **FAIL for Phase 1** |
| `NxpNci_ConfigureSettings` + `Nfc_settings.h` blobs | Yes | Missing | **FAIL for Phase 1** |
| `NxpNci_StartDiscovery` / `WaitForDiscoveryNotification` | Yes | Missing | **FAIL for Phase 1** |
| `NxpNci_HostTransceive` | Yes | `pn7160_nci_transceive` | PASS (transport only) |
| Reader tag ops / NDEF RW | Yes | Not in driver (stack Phase 1) | Expected defer |

**FAIL / WARN fixes**

| Item | Location | Recommended fix |
|---|---|---|
| No CORE_INIT | — | Add `pn7160_nci_connect()` porting `NxpNci_Connect` (Phase 0.8). |
| No settings blobs | — | Port `NxpNci_ConfigureSettings` + embed `Nfc_settings.h` equivalents. |
| Anti-tearing NTF ignored | `pn7160_nci.c:94–101` | Handle `0x60 0x07` / `0xE6` flag per NXP; document in probe path. |
| Second RX after probe accepts any timeout silently | `pn7160_nci.c:152–160` | Align with NXP: fail on unexpected non-empty non-NTF frame. |

---

## 9. Shell API completeness

**Verdict: WARN**

| Command (Phase 0 target) | Status |
|---|---|
| `pn7160 probe` | PASS |
| `pn7160 fwver` | PASS |
| `pn7160 dwl enter/leave/status` | PASS (`906cb9a`) |
| `pn7160 reset` | Missing |
| `pn7160 irq` (debug stats) | Missing |
| `pn7160 nci connect/settings/tx` | Missing (Phase 0.8 per `PN7160_SHELL_AND_EXAMPLES.md`) |
| `pn7160 fwupdate` | Missing (deferred) |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| Shell uses `DT_COMPAT_GET_ANY_STATUS_OKAY` — first instance only | `pn7160_shell.c:18–19` | Document single-instance limit; add optional `pn7160 -d <name>` later. |
| No success message on `dwl enter/leave` | `pn7160_shell.c:119–147` | Print confirmation on success for HIL scripts. |

---

## 10. Unit test coverage gaps

**Verdict: PASS** (Phase 0 minimum met)

**Twister:** `modules.nfc_pn7160.unit.pn7160_tml` — **8/8 PASS** on `qemu_cortex_m3` (2026-06-13).

| Area | Covered | Gap |
|---|---|---|
| TML header parse / oversize | ztest | — |
| SPI xfer length inlines | ztest `test_spi_xfer_lengths` | — |
| CORE_RESET_NTF FW parse | ztest `test_core_reset_ntf_*` | — |
| DWL framing helpers | ztest `test_dwl_*` (`906cb9a`) | — |
| Mock TML send/recv sequence | — | **WARN** — future `pn7160_nci` ztest with emulated IRQ |
| `pn7160_nci_parse_core_reset_rsp` | — | **WARN** — add pure parse test |
| Concurrent transceive / IRQ race | — | Defer to HIL + stress test |

---

## 11. Documentation (.rst, README, bindings)

**Verdict: WARN**

| Deliverable | Status |
|---|---|
| `modules/nfc_pn7160/README.md` | PASS — wiring, build, DWL, tests |
| Bindings IRQ/VEN/DWL/SPI framing | PASS |
| Kconfig help all symbols | WARN — `PN7160_SHELL` incomplete |
| Public API doxygen | PASS — `pn7160.h` |
| `docs/nfc/pn7160-driver.rst` | WARN — stale API list (no DWL symbols); note "doxygen groups will be added" outdated |
| `PN7160_SHELL_AND_EXAMPLES.md` | PASS (`906cb9a`) |
| Spec sync with implementation | WARN — uncommitted local edits in `2026-06-14-pn7160-zephyr-driver.md` per git status |

**WARN fixes**

| Item | Location | Recommended fix |
|---|---|---|
| RST API list missing DWL | `docs/nfc/pn7160-driver.rst:103–108` | Add `pn7160_dwl_enter/leave/mode_get`. |
| RST IRQ section omits DWL | `pn7160-driver.rst` | Add DWL subsection mirroring README. |

---

## 12. License / SPDX / NXP attribution

**Verdict: WARN**

| Check | Status |
|---|---|
| Apache-2.0 SPDX on all module files | PASS — grep all `modules/nfc_pn7160/**` |
| NXP copyright on ported TML/NCI | PASS on `pn7160_tml_i2c.c`, `pn7160_tml_spi.c`, `pn7160_nci.c` |
| README NXP attribution | PASS — `README.md:99–106` |
| `hals_temp/` not in git | PASS — `.gitignore:11` |
| No GPL Flipper in driver | PASS — manual review |
| DCO on Phase 0 commits | PASS — `Signed-off-by` on `23ab1d9`, `906cb9a` |

**WARN**

| Item | Location | Recommended fix |
|---|---|---|
| DWL logic in `pn7160_driver.c` ports NXP `tml_EnterDwlMode` but file lacks NXP copyright line | `pn7160_driver.c:1–6` | Add `Copyright (c) 2026 NXP Semiconductors` + derivation note in header. |
| `pn7160_tml.h` DWL helpers port `HEADER_SZ`/`FOOTER_SZ` macros without NXP line | `pn7160_tml.h:1–6` | Add NXP attribution comment. |
| `pn7160_tml_framing.c` ports `tml_Rx` bounds check | `pn7160_tml_framing.c` | Add NXP derivation note. |

---

## Verification log

```text
$ source scripts/env/ncs-env.sh
$ cd /opt/nordic/ncs/v3.2.4
$ west twister -T .../modules/nfc_pn7160/tests/unit/pn7160_tml \
    -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

INFO - 1/1 qemu_cortex_m3 modules.nfc_pn7160.unit.pn7160_tml PASSED
INFO - 8 of 8 executed test cases passed (100.00%)
```

Integration builds (`sample.nfc.writable_ndef_msg.pn7160.i2c` / `.spi`) and HIL were **not** executed in this review pass.

---

## Review sign-off

```
Date:              2026-06-13
Reviewer:          Cursor agent
Branch / commit:   906cb9a (+ 23ab1d9 DWL)
Transport tested:  I2C [ ]  SPI [ ]
CI Twister:        pass [x] unit  fail [ ] integration (not run)
HIL probe/fwver:   pass [ ]  fail [ ]
Blocking issues:   HAL integration, HIL, CORE_INIT/settings/discovery, transceive mutex
Phase 1 approved:  yes [ ]  no [x]
```
