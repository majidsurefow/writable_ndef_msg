# PN7160 Post–Phase 0 Review Checklist

**When to run:** After Phase 0 implementation checklist is complete, **before**
Phase 1 NDEF / example port begins.

**Reference spec:** [`2026-06-14-pn7160-zephyr-driver.md`](../../specs/2026-06-14-pn7160-zephyr-driver.md) § Phase 0 DONE Definition  
**NXP reference:** `hals_temp/NXP-NCI2.0_LPC55S6x_examples/` (local, gitignored)

---

## How to use this checklist

1. Work top-to-bottom; mark `[x]` only with evidence (CI log, HIL photo, LA trace).
2. File issues for any `[ ]` item blocking Phase 1.
3. Full review pass includes code review against ENS160/BMM150 patterns and NXP
   `tml.c` / `NxpNci20.c`.

---

## 1. Driver correctness

### 1.1 TML — I2C

| # | Check | Evidence |
|---|---|---|
| 1.1.1 | Send: full frame write, 10 ms retry on failure (`tml_Tx`) | Code review + bus trace |
| 1.1.2 | Recv: 3-byte header then payload (`tml_Rx`) | Unit test + HIL |
| 1.1.3 | Frame bounds: reject payload > buffer (`PN7160_RX_BUF_SIZE`) | ztest `pn7160_tml` |
| 1.1.4 | Bus mutex guards send/recv pairs | Code review |
| 1.1.5 | I2C address `0x28`, 100 kHz on eval overlay | Overlay + HIL |

### 1.2 TML — SPI

| # | Check | Evidence |
|---|---|---|
| 1.2.1 | Write prefix `0x7F` before payload | Code review + LA |
| 1.2.2 | Read dummy `0xFF`, payload shifted by 1 | Code review + LA |
| 1.2.3 | 10 µs delay after SPI read (`k_busy_wait(10)`) | Code review |
| 1.2.4 | CS held per NXP framing (single transceive) | LA on `pn7160_spi.overlay` |
| 1.2.5 | SPI build passes CI (`ci_build` + `pn7160.spi`) | Twister log |

### 1.3 GPIO — VEN / IRQ / DWL

| # | Check | Evidence |
|---|---|---|
| 1.3.1 | VEN reset: low 10 ms, high 10 ms | Scope / logic analyzer |
| 1.3.2 | IRQ: `GPIO_INT_EDGE_TO_ACTIVE`, no bus I/O in ISR | Code review + LA |
| 1.3.3 | IRQ work → module WQ → TML recv (not system WQ inline I2C) | Code review |
| 1.3.4 | DWL GPIO configured when present; download API deferred OK | DT + code review |
| 1.3.5 | `pn7160_wait_irq()` usable for debug polling path | Shell / test |

### 1.4 NCI

| # | Check | Evidence |
|---|---|---|
| 1.4.1 | `CORE_RESET` cmd `{20 00 01 01}` matches NXP | Code review |
| 1.4.2 | RSP `40 00` status OK parsed | HIL `pn7160 probe` |
| 1.4.3 | `CORE_RESET_NTF` FW version bytes 9–11 cached | HIL `pn7160 fwver` |
| 1.4.4 | `pn7160_nci_transceive` IRQ-driven (sem, not busy poll) | LA + code review |
| 1.4.5 | Anti-tearing `CORE_GENERIC_ERROR_NTF` (0x60 0x07) — document or port | Gap review |
| 1.4.6 | `CORE_INIT` + settings blobs — **Phase 0.8** (explicit defer OK) | Issue link |

---

## 2. Zephyr pattern compliance

Compare against NCS v3.2.4 **ENS160**, **BMM150**, **APDS9960**.

| # | Pattern | Pass? |
|---|---|---|
| 2.1 | `DT_DRV_COMPAT` + `DT_INST_FOREACH_STATUS_OKAY` | |
| 2.2 | `DT_INST_ON_BUS` I2C/SPI split via `COND_CODE_1` | |
| 2.3 | `config` const / `data` per-instance | |
| 2.4 | `LOG_MODULE_REGISTER` + `LOG_MODULE_DECLARE` | |
| 2.5 | `POST_KERNEL` init; bus `*_is_ready_dt` before use | |
| 2.6 | `gpio_is_ready_dt` on IRQ and VEN | |
| 2.7 | ISR: `CONTAINER_OF`, atomic flag, `k_work` only | |
| 2.8 | Optional: disable IRQ edge during drain (ens160 pattern) | |
| 2.9 | errno returns (no silent `void` failures in init) | |
| 2.10 | `device_is_ready()` documented for callers | |

---

## 3. CI / automation

| # | Gate | Command / workflow |
|---|---|---|
| 3.1 | Integration build I2C | Twister `ci_build` + `pn7160.i2c` |
| 3.2 | Integration build SPI | Twister `ci_build` + `pn7160.spi` |
| 3.3 | Unit tests | Twister `ci_unit` → `modules.nfc_pn7160.unit.pn7160_tml` |
| 3.4 | Devicetree bindings | `devicetree.yml` dtschema on `modules/nfc_pn7160/dts/` |
| 3.5 | checkpatch | `coding_guidelines.yml` on `modules/nfc_pn7160/` |
| 3.6 | License / DCO | `compliance.yml`, `license_check.yml` |
| 3.7 | Doxygen delta | `doxygen-coverage-delta.yml` on `include/nfc/` |

All merge gates green on PR before Phase 1 kickoff.

---

## 4. Unit test coverage

| Area | Test location | Minimum |
|---|---|---|
| TML framing helpers | `tests/unit/pn7160_tml/` | Header parse, oversize reject ✅ |
| SPI prefix/dummy helpers | `pn7160_tml.h` inlines | Add ztests for xfer length |
| NCI CORE_RESET_NTF parse | **Add** `pn7160_nci` ztest | Synthetic frame bytes |
| Mock TML (future) | Optional | Send/recv sequence without hardware |

Target: no regression in `ci_unit` on `qemu_cortex_m3`.

---

## 5. Hardware-in-the-loop (manual)

Platform: **nRF54L15 DK + PN7160 eval shield** (or project board).

| # | Test | Pass criteria |
|---|---|---|
| 5.1 | Flash I2C overlay, boot | No init errors in log |
| 5.2 | `pn7160 probe` | CORE_RESET OK |
| 5.3 | `pn7160 fwver` | Matches NXP MCUXpresso readout |
| 5.4 | Repeat on SPI overlay | Same FW version |
| 5.5 | LA: IRQ pulse on NCI response | Edge aligns with TML recv |
| 5.6 | No busy-poll in idle thread | Thread analyzer / GPIO trace |
| 5.7 | (Phase 0.9) `nfc reader scan` | UID printed — defer if not in Phase 0 |

Record FW version, board revision, and overlay used in test log.

---

## 6. Security & license

| # | Item | Action |
|---|---|---|
| 6.1 | Apache-2.0 SPDX on all module files | Scan |
| 6.2 | NXP copyright on ported `tml.c` / `NxpNci20.c` sections | File headers |
| 6.3 | README NXP attribution paragraph | `modules/nfc_pn7160/README.md` |
| 6.4 | No `hals_temp/` or SW6705 binaries in git | `.gitignore` verify |
| 6.5 | No GPL Flipper code mixed into driver | Manual review |
| 6.6 | DCO sign-off on all Phase 0 commits | `git log` |

---

## 7. Documentation

| # | Deliverable | Location |
|---|---|---|
| 7.1 | Module README (wiring, build, quality bar) | `modules/nfc_pn7160/README.md` |
| 7.2 | Binding descriptions (IRQ, VEN, DWL, SPI framing) | `dts/bindings/nfc/*.yaml` |
| 7.3 | Kconfig help on every symbol | `zephyr/Kconfig` |
| 7.4 | Public API doxygen | `include/nfc/pn7160.h` |
| 7.5 | Upstream-style driver doc (draft) | `docs/nfc/pn7160-driver.rst` |
| 7.6 | Architecture spec current with implementation | `docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md` (frozen @ `21bdd71`) |
| 7.7 | Shell / example map | `docs/nfc/PN7160_SHELL_AND_EXAMPLES.md` |

---

## 8. Phase 0 exit criteria (functional)

From spec § Phase 0 DONE — verify each:

- [ ] I2C TML NXP framing parity
- [ ] SPI TML `0x7F`/`0xFF` + ztest
- [ ] VEN 10 ms / 10 ms
- [ ] IRQ → work queue → TML recv (no ISR bus I/O)
- [ ] NCI probe + FW version
- [ ] Host transceive on worker path
- [ ] HAL integration (`nfc_transport` uses `DEVICE_DT_GET`)
- [ ] Shell `pn7160 probe` / `fwver` on HIL
- [ ] Discovery + `nfc reader scan` (0.8 / 0.9 — may remain open for Phase 1)

---

## 9. Ready for Phase 1?

Phase 1 (NDEF clone) may start when:

1. Sections **1–3** and **6** are complete.
2. HIL items **5.1–5.6** pass on at least one transport (I2C minimum).
3. Open Phase 0.8/0.9 items are **tracked as issues**, not silent gaps.
4. Full code review signed off (driver + patterns + NXP parity table).

---

## Review sign-off template

```
Date:
Reviewer:
Branch / commit:
Transport tested: I2C [ ]  SPI [ ]
CI Twister: pass [ ]  fail [ ]
HIL probe/fwver: pass [ ]  fail [ ]
Blocking issues:
Phase 1 approved: yes [ ]  no [ ]
```
