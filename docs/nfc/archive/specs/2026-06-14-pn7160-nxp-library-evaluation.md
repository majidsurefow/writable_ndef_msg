# PN7160 NXP Library Evaluation — NXP-NCI2.0 vs Alternatives

**Date:** 2026-06-14  
**Status:** Architecture-of-record  
**Local reference tree:** `hals_temp/NXP-NCI2.0_LPC55S6x_examples/` (NXP SW6705, AN13288)

**Related:** [`2026-06-13-implementation-phases.md`](2026-06-13-implementation-phases.md) · [`2026-06-13-nfct-pn7160-capability-matrix.md`](2026-06-13-nfct-pn7160-capability-matrix.md) · [`2026-06-14-pn7160-zephyr-driver.md`](2026-06-14-pn7160-zephyr-driver.md)

---

## Executive verdict

**Use NXP-NCI2.0 (SW6705) as the sole NXP stack for PN7160.** Do **not** add NxpNfcRdLib / RFAL — those target NFC *frontend* ICs (PN5180, PN5190, CLRC663, PN74/76), not NCI controllers. The local examples are **sufficient for Phase 0–2** (HAL bring-up, NDEF clone, Type-4 CE) but **insufficient alone for Phase 1 expansion** (Classic, DeSFire, EMV, Aliro, FeliCa, ISO15693 clone logic). Protocol pollers must be written in-project (Flipper facts + `NxpNci_ReaderTagCmd` raw lane), not sourced from another NXP package.

---

## 1. Local NXP example tree audit

### 1.1 Directory structure

| Path | Role |
|---|---|
| `NfcLibrary/NxpNci20/` | NCI 2.0 host stack — `NxpNci20.c`, `NxpNci20.h` |
| `NfcLibrary/NdefLibrary/` | High-level NDEF: `RW_NDEF_*`, `T4T_NDEF_emu.c`, `P2P_NDEF.c` |
| `NfcLibrary/inc/` | Public API (`Nfc.h`), RF/config blobs (`Nfc_settings.h`) |
| `source/TML/` | Transport Mapping Layer — I²C/SPI + GPIO IRQ (`tml.c`) |
| `source/nfc_example_*.c` | Application demos (see §1.2) |
| `source/CE_scenario2.c` | NFCEE_NDEF autonomous CE path |
| `sFWupdate/` | Firmware download over NCI (separate build config) |
| `board/`, `drivers/`, `CMSIS/`, `startup/` | LPC55S69 MCUXpresso SDK (not NFC-specific) |

**No README in tree.** Documentation is external: [AN13288](https://www.nxp.com/doc/AN13288) (MCUXpresso examples guide), [UM11495](https://www.nxp.com/doc/UM11495) (PN7160 user manual), [AN12989](https://www.nxp.com/doc/AN12989) (quick start).

### 1.2 Example projects (`.cproject` build configs)

| Config | Main source | Compile flags removed | Demonstrates |
|---|---|---|---|
| `RWandCE_Debug` | `nfc_example_RWandCE.c` | `REMOVE_P2P`, `REMOVE_FACTORYTEST` | Poll + listen discovery; NDEF read (poll) + T4T CE (listen) |
| `RW_Debug` | `nfc_example_RW.c` | `REMOVE_P2P`, `REMOVE_FACTORYTEST` | Raw TAG-CMD: MIFARE Classic, ISO15693, T2 pages, ISO-DEP PPSE |
| `P2P_Debug` | `nfc_example_P2P.c` | `REMOVE_FACTORYTEST` | P2P SNEP send/receive |
| `CE_scenario2_Debug` | `CE_scenario2.c` | `REMOVE_P2P`, `REMOVE_FACTORYTEST` | NFCEE_NDEF config + autonomous T4T CE |
| `FWupdate_Debug` | `nfc_example_FWupdate.c` | RW/CE/P2P stripped | PN7160 FW update sequence |

### 1.3 Modes supported (API level)

From `Nfc.h`:

| Mode | API flags | Example usage |
|---|---|---|
| **POLL (reader)** | `MODE_POLL`, `NXPNCI_MODE_RW` | All RW examples |
| **LISTEN (target)** | `MODE_LISTEN`, `NXPNCI_MODE_CARDEMU` | RWandCE, CE_scenario2 |
| **Card emulation** | `NxpNci_ProcessCardMode`, `T4T_NDEF_EMU_*`, `NFCEE_NDEF_*` | RWandCE (DH T4T), CE_scenario2 (NFCEE) |
| **P2P** | `NXPNCI_MODE_P2P`, `NxpNci_ProcessP2pMode`, `P2P_NDEF_*` | P2P example only |

Technologies in discovery tables:

| Tech | Symbol | Poll | Listen |
|---|---|---|---|
| NFC-A | `TECH_PASSIVE_NFCA` | Yes | Yes |
| NFC-B | `TECH_PASSIVE_NFCB` | Yes | Yes |
| NFC-F (FeliCa) | `TECH_PASSIVE_NFCF` | Yes | P2P example only |
| ISO 15693 | `TECH_PASSIVE_15693` | Yes | No |
| Active NFC | `TECH_ACTIVE_NFC` | P2P example | P2P example |

### 1.4 Tag types and protocol coverage

| Tag / protocol | NXP high-level API | Example evidence | Notes |
|---|---|---|---|
| Type 1 NDEF | `RW_NDEF_T1T` | Library only | No dedicated demo scenario |
| Type 2 NDEF | `RW_NDEF_T2T`, `READ_NDEF` | RWandCE | **Limit:** tags &gt;1024 B need SECTOR_SELECT — not implemented (`RW_NDEF_T2T.c` TODO) |
| Type 3 NDEF | `RW_NDEF_T3T` | RWandCE polls NFC-F | NDEF R/W in library; no FeliCa system-code poller |
| Type 4 NDEF | `RW_NDEF_T4T` | RWandCE | Full SELECT CC/NDEF READ/WRITE state machine |
| Type 5 (15693) NDEF | `RW_NDEF_T5T` | RWandCE polls 15693 | NDEF layer exists; raw blocks in `nfc_example_RW.c` |
| MIFARE Classic | `PROT_MIFARE`, `NxpNci_ReaderTagCmd` | `PCD_MIFARE_scenario` | Auth + block R/W via TAG-CMD; `RW_NDEF_MIFARE.c` is **simplified NDEF-only** (sector 1 contiguous — TODO in source) |
| MIFARE Ultralight | T2 path (`PROT_T2T`) | `PCD_ISO14443_3A_scenario` | Page READ/WRITE via raw 0x30/0xA2 — not NTAG feature-complete |
| ISO-DEP / EMV entry | `PROT_ISODEP`, raw APDU | `PCD_ISO14443_4_scenario` | **PPSE SELECT only** — no EMVCo stack |
| MIFARE DESFire | — | **Not in examples** | HW activates ISO-DEP; host must implement DESFire APDUs |
| FeliCa | Poll + `TECH_PASSIVE_NFCF` | Discovery only | No FeliCa service poller |
| Aliro | — | **Not present** | No NXP reference anywhere |

### 1.5 Card emulation paths

Two distinct CE mechanisms:

1. **DH-hosted T4T (`T4T_NDEF_emu.c`)** — Host answers ISO-DEP APDUs over NCI card-mode data interface. Supports SELECT NDEF app/CC/file, READ, WRITE. Used in `nfc_example_RWandCE.c` with `NxpNci_ProcessCardMode()`.

2. **NFCEE_NDEF (PN7160 firmware T4T)** — NDEF stored inside PN7160; host writes via NFCEE APDU (`NFCEE_NDEF_Configuration`, `NFCEE_NDEF_DH_Write`). Used in `CE_scenario2.c`. Firmware ≥12.50 supports RF-side NDEF WRITE (datasheet §6.8–6.9).

**HW CE scope (PN7160 datasheet / product page):** NFC Forum Type 3 and Type 4 (A & B). Examples implement **Type 4 only**.

### 1.6 High-level API summary

| Layer | Prefix | Blocking? | Port to Zephyr |
|---|---|---|---|
| Transport | `tml_*` (internal) | Yes — blocking I²C/SPI | **Replace** with Zephyr bus + IRQ worker |
| NCI core | `NxpNci_*` | Yes — poll/wait loops | **Port + async refactor** (`NxpNci20.c`) |
| NDEF reader | `RW_NDEF_*` | Yes | **Optional reuse** Phase 1 NDEF gate; replace for production |
| T4T CE | `T4T_NDEF_EMU_*` | Yes | **Port** Phase 2 or use NFCEE path |
| P2P | `P2P_NDEF_*` | Yes | Defer — not in project goals |
| Raw lane | `NxpNci_ReaderTagCmd` | Yes | **Wrap** as `nfc_transport_transceive()` |

### 1.7 RTOS vs baremetal

AN13288 describes **NullOS** (no OS) examples. This LPC55S69 tree uses bare-metal blocking MCUX drivers. A `FreeRTOS.h` include appears in UART adapter headers but the NFC examples do not use an RTOS task model. **Zephyr port must not copy blocking semantics** — use worker thread + semaphore (see Phase 0 driver spec).

### 1.8 Documented limitations in source

| Limitation | Location |
|---|---|
| T2T NDEF: no tags &gt;1024 bytes (no SECTOR_SELECT) | `RW_NDEF_T2T.c` |
| MIFARE NDEF: simplified layout, sector 1 contiguous only | `RW_NDEF_MIFARE.c` |
| RF tuning blobs target OM27160 EVK | `Nfc_settings.h` — must retune for custom antenna |
| P2P / factory test compiled out in default RW+CE config | `.cproject` `REMOVE_*` flags |
| No malloc in NCI path | Suitable for embedded port |

### 1.9 License

| Component | License |
|---|---|
| `NfcLibrary/*` | NXP copyright — redistribution requires NXP consent per file header; SW6705 / AN13288 states **BSD-3-Clause** for example code |
| MCUX SDK (`board/`, `drivers/`) | **BSD-3-Clause** (SPDX in headers) |
| CMSIS | Apache-2.0 |

**Patent notice (AN12989 §6.3):** Purchase of PN7160 IC does **not** convey implied patent license for NFC standard implementations or product combinations.

---

## 2. PN7160 hardware capabilities (online sources)

Sources: [PN7160 product page](https://www.nxp.com/products/PN7160), [PN7160/PN7161 datasheet](https://www.nxp.com/docs/en/data-sheet/PN7160_PN7161.pdf), [UM11495](https://www.nxp.com/docs/en/user-manual/UM11495.pdf), [AN12989](https://www.nxp.com/docs/en/application-note/AN12989.pdf).

### 2.1 Reader (poll) mode

| Technology | PN7160 HW |
|---|---|
| ISO 14443-A | Yes — T1T, T2T, MIFARE 1K/4K, ISO-DEP |
| ISO 14443-B | Yes |
| FeliCa / T3T | Yes |
| ISO 15693 / T5T | Yes |
| MIFARE DESFire | Yes — via ISO-DEP (host protocol) |
| MIFARE Plus | Yes — RF layer |
| Active initiator (P2P) | Yes |

### 2.2 Listen / card emulation mode

| Technology | PN7160 HW |
|---|---|
| Type 4 (ISO-DEP) | Yes — DH-NFCEE and NFCEE_NDEF |
| Type 3 (FeliCa) | Yes — per product page; **not in MCUXpresso CE examples** |
| MIFARE Classic emulation | **No** |
| DESFire / EMV / Aliro emulation | **No** — only generic ISO-DEP if host implements APDU listener |
| ISO 15693 emulation | **No** |

### 2.3 EMVCo and Aliro

| Topic | Finding |
|---|---|
| **EMVCo L1/L2 compliance** | PN7160 is **explicitly not** EMVCo-compliant. NXP directs payment designs to PN5180, PN5190, or PN7220. |
| **EMV-style polling** | Host can send PPSE / payment APDUs over ISO-DEP (`nfc_example_RW.c` demo). Analog/timing not certified. |
| **Aliro** | **Not mentioned** in PN7160 docs, NCI examples, or libnfc-nci. Aliro is an application-layer protocol — must be implemented in-project (Wave 5 plan). |
| **Apple ECP** | PN7161 only (not PN7160). |

### 2.4 NXP software ecosystem map

| Package | Target hardware | Protocol style | Use for PN7160? |
|---|---|---|---|
| **NXP-NCI2.0 / SW6705** | PN7160, PN7220 (NCI controllers) | NCI 2.0 over I²C/SPI | **Yes — primary stack** |
| **linux_libnfc-nci** (`NCI2.0_PN7160` branch) | Linux/Android on PN7160 | Same NCI + richer tag ops | Reference only — not Zephyr |
| **Android NFC MW** | PN7160 / PN7220 | NCI + Java JNI | Not applicable |
| **NxpNfcRdLib (NFC Reader Library) + RFAL** | PN5180, PN5190, CLRC663, PN74xx, PN76xx | Direct register/HAL on frontend IC | **No — wrong chip class** |
| **nfct_nrfxlib** | Nordic NFCT peripheral | On-die listen | Phase 3 only — separate backend |

NXP quick start (AN12989 §4.3.3) explicitly points bare-metal/RTOS integrators to **SW6705 + AN13288**, not Reader Library.

### 2.5 Zephyr / community integrations

| Project | Status |
|---|---|
| Upstream Zephyr | **No official PN7160 driver** (GitHub #5703 — NFCT-only Nordic path) |
| [Strooom/PN7160](https://github.com/Strooom/PN7160) | Community Arduino-oriented NCI wrapper — useful for NCI message patterns, not production stack |
| This project | Out-of-tree module + port of `NxpNci20.c` per [`2026-06-14-pn7160-zephyr-driver.md`](2026-06-14-pn7160-zephyr-driver.md) |

---

## 3. Gap analysis

| Capability | In NXP example? | PN7160 HW supports? | Need extra NXP lib? | Our phase needs it? |
|---|---|---|---|---|
| **NDEF T4 read/write** | Yes — `RW_NDEF_T4T`, `READ_NDEF`/`WRITE_NDEF` | Yes (poll) | No | **Phase 1** (first gate) |
| **NDEF T2 read** | Yes — `RW_NDEF_T2T` (≤1024 B) | Yes | No | **Phase 1** |
| **MIFARE Classic read/auth** | Partial — raw TAG-CMD demo; simplified MIFARE NDEF | Yes (poll) | No — need **Flipper/crypto1** poller | **Phase 1** expand |
| **DESFire clone** | No — ISO-DEP only | Yes (poll, host APDU) | No — **in-project poller** | **Phase 1** expand |
| **EMV capture** | Partial — PPSE SELECT demo only | Yes (poll, not EMVCo certified) | No — **Flipper/wave5-emv** | **Phase 1** expand |
| **FeliCa** | Discovery only | Yes (poll) | No — **Flipper felica_poller** | **Phase 1** (clone-only) |
| **ISO15693** | Raw block R/W demo | Yes (poll) | No — **Flipper poller** | **Phase 1** (clone-only) |
| **Aliro** | No | No HW assist — ISO-DEP if card presents | No — **wave5-aliro in-project** | **Phase 1** expand |
| **Type-4 CE emulate** | Yes — `T4T_NDEF_emu` + NFCEE_NDEF | Yes (listen) | No | **Phase 2** |
| **P2P** | Yes — `P2P_NDEF` / SNEP | Yes | No | **Defer** — not in product goals |

---

## 4. Recommendation

### 4.1 Verdict: stay on NXP-NCI2.0

| Question | Answer |
|---|---|
| Use examples as-is? | **No** — TML and blocking model must be rewritten for Zephyr |
| Use NCI + NDEF library code? | **Yes** — port `NxpNci20.c`, settings blobs, optionally `RW_NDEF_T4T` / `T4T_NDEF_emu` |
| Add NxpNfcRdLib / RFAL? | **No** — incompatible with PN7160 (NCI controller vs RF frontend) |
| Add linux_libnfc-nci? | **No** for firmware — study as reference for tag-operation patterns only |
| Switch libs? | **No switch** — correct stack already selected |

### 4.2 What Flipper (and in-project code) still provides

NXP examples deliver **RF discovery + NCI transceive + basic NDEF**. They do **not** deliver clone-grade protocol logic:

| Domain | Flipper / project source |
|---|---|
| MIFARE Classic Crypto1 auth + sector model | `mf_classic_poller.c`, `crypto1.c` |
| Ultralight / NTAG features | `mf_ultralight_poller.c` |
| DESFire file tree + secure messaging | `mf_desfire_poller.c`, wave5-desfire |
| EMV PPSE → AID → record capture | wave5-emv, payment app facts |
| FeliCa polling / sync | `felica_poller.c` |
| ISO15693 / SLIX | `iso15693_3_poller.c`, `slix_poller.c` |
| Aliro transcript listener/poller | wave5-aliro (no Flipper module) |
| Store envelope, verify loop, NFCT migration | Project stack |

### 4.3 Porting strategy by phase

| Phase | From NXP-NCI2.0 | New work | Defer |
|---|---|---|---|
| **0** | Port `NxpNci20.c` core, `Nfc_settings.h`, discovery from `nfc_example_RW.c` | Zephyr TML, async worker, DT binding | NDEF, CE, P2P, FW update |
| **1 (NDEF gate)** | Optionally wrap `RW_NDEF_T4T` / `RW_NDEF_T2T` or reimplement thin pollers | `ndef_poller.c`, `nfc_store`, Flipper-guided expand | DESFire, EMV, Aliro until NDEF round-trip passes |
| **1 (expand)** | `NxpNci_ReaderTagCmd` only | Classic, UL, DESFire, EMV, FeliCa, 15693 pollers | — |
| **2** | `T4T_NDEF_emu.c` **or** NFCEE_NDEF path (`CE_scenario2.c`) | PN7160 listen-mode integration, persist on WRITE | Type 3 CE, P2P |
| **3+** | Drop PN7160 CE for product | NFCT `nfc_t4t_lib` | — |

### 4.4 Optional future NXP additions (not lib switches)

| Item | When | Source |
|---|---|---|
| PN7160 FW blobs | If field FW update needed | `nfc-NXPNFCC_FW` repo / `sFWupdate` example |
| RF tuning | Antenna bring-up | AN14518 + RF Settings Command Builder |
| linux_libnfc-nci tag modules | Protocol reference during poller authoring | GitHub `NXPNFCLinux/linux_libnfc-nci` branch `NCI2.0_PN7160` |

---

## 5. References

| Document | URL / path |
|---|---|
| SW6705 MCUXpresso examples | https://www.nxp.com/webapp/Download?colCode=SW6705 |
| AN13288 — examples guide | https://www.nxp.com/doc/AN13288 |
| AN12989 — quick start | https://www.nxp.com/doc/AN12989 |
| UM11495 — PN7160 user manual | https://www.nxp.com/doc/UM11495 |
| PN7160 datasheet | https://www.nxp.com/docs/en/data-sheet/PN7160_PN7161.pdf |
| linux_libnfc-nci NCI2.0_PN7160 | https://github.com/NXPNFCLinux/linux_libnfc-nci/tree/NCI2.0_PN7160 |
| NFC Reader Library (not for PN7160) | https://www.nxp.com/design/design-center/development-boards-and-designs/nfc-reader-library-software-support-for-nfc-frontend-solutions:NFC-READER-LIBRARY |
| Local tree | `hals_temp/NXP-NCI2.0_LPC55S6x_examples/` |

---

## Changelog

- **v1 (2026-06-14):** Initial evaluation from local SW6705 tree audit, PN7160 datasheet/UM11495, NXP product docs, Zephyr community status.
