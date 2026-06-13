# PN7160 NXP Library Audit — Examples vs Project Goals

**Date:** 2026-06-14  
**Status:** Architecture-of-record  
**Local tree audited:** `hals_temp/NXP-NCI2.0_LPC55S6x_examples/` (NXP SW6705 / AN13288)  
**Related:** [`2026-06-13-implementation-phases.md`](2026-06-13-implementation-phases.md) · [`2026-06-13-nfct-pn7160-capability-matrix.md`](2026-06-13-nfct-pn7160-capability-matrix.md)

---

## Executive summary

The **NXP-NCI2.0 MCUXpresso examples (SW6705)** are the **official and correct host stack for PN7160** on bare-metal / RTOS hosts. They provide NCI 2.0 transport (`NxpNci20`), high-level NDEF read/write for T1T–T5T + MIFARE Classic NDEF, Type-4 card emulation (`T4T_NDEF_emu`), P2P/SNEP, and raw transceive (`NxpNci_ReaderTagCmd`) — enough to **port as-is for Phase 0–2** (bring-up, NDEF clone, T4 CE emulate). **No additional NXP library is required or available** for PN7160: the **NFC Reader Library (NfcRdLib)** targets frontend ICs (PN5180, PN5190, CLRC663, PN74/76), not NCI controllers. Gaps for DeSFire, EMV, Aliro, full Classic cloning, and FeliCa beyond T3T NDEF must be built as **custom pollers** (Flipper facts + `NxpNci_ReaderTagCmd`), not sourced from another NXP SDK.

---

## 1. Local NXP example tree audit

### 1.1 Package identity

| Item | Value |
|---|---|
| Package | **SW6705** — PN7160 NXP-NCI2.0 MCUXpresso Example Project |
| Integration guide | **AN13288** — NXP-NCI2.0 MCUXpresso examples guide |
| Quick start | **AN12989** — PN7160 product quick start guide |
| NCI API | **`NxpNci20`** (`NfcLibrary/NxpNci20/`) — NCI 2.0, not legacy NCI 1.0 / `NxpNci` |
| Transport | **TML** (`source/TML/tml.c`) — I²C or SPI framing to PN7160 |
| Target MCU | LPC55S69 (portable to Zephyr; LPC-specific drivers are discardable) |

### 1.2 Example applications (MCUXpresso build configs)

| Build config | Source file | Modes enabled | Purpose |
|---|---|---|---|
| `RW_Debug` | `nfc_example_RW.c` | **POLL** only (`NXPNCI_MODE_RW`) | Raw tag ops: T2 block R/W, ISO-DEP PPSE, MIFARE auth+block, ISO15693 block |
| `RWandCE_Debug` | `nfc_example_RWandCE.c` | **POLL + LISTEN** (`NXPNCI_MODE_RW \| NXPNCI_MODE_CARDEMU`) | NDEF read from tags + T4T CE to external reader |
| `P2P_Debug` | `nfc_example_P2P.c` | **P2P** (`NXPNCI_MODE_P2P`) | SNEP NDEF exchange (initiator + target) |
| `CE_scenario2_Debug` | `CE_scenario2.c` | **LISTEN** + NFCEE_NDEF | Autonomous embedded T4T NDEF (no host APDU handling during field) |
| `FWupdate_Debug` | `nfc_example_FWupdate.c` | FW download | PN7160 firmware update sequence |

**Mode flags** (`Nfc.h`):

```c
#define NXPNCI_MODE_CARDEMU  (1<<0)  // LISTEN
#define NXPNCI_MODE_P2P      (1<<1)
#define NXPNCI_MODE_RW       (1<<2)  // POLL
#define MODE_POLL            0x00
#define MODE_LISTEN          0x80
```

### 1.3 Library structure

```
NfcLibrary/
├── NxpNci20/          # NCI 2.0 state machine, discovery, transceive
│   ├── inc/NxpNci20.h
│   └── src/NxpNci20.c
├── inc/
│   ├── Nfc.h          # Public API surface
│   └── Nfc_settings.h # RF/CORE config blobs (port as const arrays)
└── NdefLibrary/
    ├── RW_NDEF.c      # Dispatcher
    ├── RW_NDEF_T1T.c
    ├── RW_NDEF_T2T.c
    ├── RW_NDEF_T3T.c  # FeliCa / Type 3 NDEF
    ├── RW_NDEF_T4T.c
    ├── RW_NDEF_T5T.c  # ISO15693 / Type 5 NDEF
    ├── RW_NDEF_MIFARE.c
    ├── T4T_NDEF_emu.c # Card emulation
    └── P2P_NDEF.c
```

**Compile-time feature stripping** via defines: `REMOVE_RW_SUPPORT`, `REMOVE_CARDEMU_SUPPORT`, `REMOVE_P2P_SUPPORT`, `REMOVE_NDEF_SUPPORT`, `REMOVE_FACTORYTEST_SUPPORT`.

### 1.4 Protocol / tag support per example

| Example | Technologies in discovery | Protocols exercised | High-level API |
|---|---|---|---|
| `RW` | NFC-A, NFC-B, ISO15693 | T2T (`PROT_T2T`), ISO-DEP/PPSE (`PROT_ISODEP`), T5T (`PROT_T5T`), MIFARE (`PROT_MIFARE`) | Raw `NxpNci_ReaderTagCmd` only |
| `RWandCE` | NFC-A, NFC-F, NFC-B, ISO15693 + LISTEN A/B | All RW_NDEF types via `READ_NDEF` | `RW_NDEF_*`, `T4T_NDEF_EMU_*` |
| `P2P` | NFC-A/F active + passive, LISTEN A/F | NFC-DEP (`INTF_NFCDEP`) | `P2P_NDEF_*` (SNEP) |
| `CE_scenario2` | LISTEN A/B | NFCEE autonomous NDEF | `NFCEE_NDEF_*` + optional `NxpNci_ProcessCardMode` |

**Protocols defined** (`Nfc.h`): `PROT_T1T`, `PROT_T2T`, `PROT_T3T`, `PROT_ISODEP`, `PROT_NFCDEP`, `PROT_T5T`, `PROT_MIFARE` (0x80 TAG-CMD).

**Interfaces:** `INTF_FRAME`, `INTF_ISODEP`, `INTF_NFCDEP`, `INTF_TAGCMD` (MIFARE Classic).

### 1.5 RW_NDEF APIs — tag type coverage

| RW_NDEF module | Tag type constant | Read | Write | Notes |
|---|---|---|---|---|
| `RW_NDEF_T1T` | `0x1` | Yes | No | Topaz |
| `RW_NDEF_T2T` | `0x2` | Yes | Yes | TODO: no tags >1024 B (no SECTOR_SELECT) |
| `RW_NDEF_T3T` | `0x3` | Yes | No | FeliCa NDEF only |
| `RW_NDEF_T4T` | `0x4` | Yes | Yes | Standard NDEF AID `D2760000850101` |
| `RW_NDEF_T5T` | `0x6` | Yes | Yes | ISO15693 NDEF |
| `RW_NDEF_MIFARE` | `0x80` | Yes | Yes | **Simplified Classic NDEF only** — default key `FF…FF`, contiguous sector from block 4 |

**NDEF buffer cap:** `RW_MAX_NDEF_FILE_SIZE` = **500 bytes** (hard limit in `RW_NDEF.h`).

### 1.6 Card emulation — what exists

| Mechanism | Implementation | Scope |
|---|---|---|
| **DH T4T CE** | `T4T_NDEF_emu.c` + `NxpNci_ProcessCardMode()` | ISO-DEP Type-4 NDEF tag (SELECT NDEF app/CC/file, READ/UPDATE BINARY) |
| **NFCEE autonomous NDEF** | `NFCEE_NDEF_Configuration()` / `NFCEE_NDEF_DH_Write()` in `NxpNci20.c` | Firmware-embedded T4T; host preloads NDEF, PN7160 serves autonomously |
| **Raw CE** | `NxpNci_CardModeSend()` / `NxpNci_CardModeReceive()` | Undocumented in examples; host implements full APDU state machine |

**Not present:** Classic CE, FeliCa CE, ISO15693 CE, DeSFire CE, EMV PICC emulation on PN7160 (beyond T4 NDEF / ISO-DEP raw hooks).

`T4T_NDEF_emu.c` defines a `DESFire_prod` enum state that is **never used** — no DeSFire CE logic.

### 1.7 Raw transceive and MIFARE Classic

`NxpNci_ReaderTagCmd()` sends payloads on the active RF interface. For MIFARE Classic, `NxpNci20.c` selects `INTF_TAGCMD` (`NCISelectMIFARE`).

`nfc_example_RW.c` `PCD_MIFARE_scenario()` demonstrates:

- Auth sector 1 with key `FFFFFFFFFFFF`
- Read/write block 4

This is the **foundation for Classic clone** — not a complete poller.

### 1.8 Documented limitations (code comments)

| Location | Limitation |
|---|---|
| `RW_NDEF_MIFARE.c` | "Only simplified scenario… NDEF in contiguous sector from sector #1" |
| `RW_NDEF_T2T.c` | No support for tags >1024 bytes (SECTOR_SELECT) |
| `RWandCE` | T3T write path absent in dispatcher |
| `Nfc_settings.h` | RF tuning blobs target OM27160 demo kit — must be re-tuned for nRF54L15 + eval shield |

### 1.9 What is NOT in the examples

| Capability | In examples? | Evidence |
|---|---|---|
| MIFARE Classic multi-key / Crypto1 recovery | No | Only default-key NDEF + single-sector raw demo |
| MIFARE DESFire app/file poller | No | No source file; ISO-DEP demo is PPSE only |
| EMV payment poller | Partial | PPSE SELECT in `PCD_ISO14443_4_scenario()` — no AID tree, records, PDOL |
| FeliCa system codes / wallet | No | T3T NDEF read only; no FeliCa poller |
| Aliro / Apple ECP | No | Not referenced; requires PN7161 + Apple authorization |
| ISO15693 full poller | Partial | Block R/W demo; no NDEF+system info beyond T5T |
| Writable Classic non-NDEF | No | NDEF-oriented MIFARE path only |
| T4 CE beyond NDEF | No | Fixed NDEF app/CC layout |
| Factory test | API only | `NxpNci_FactoryTest_*` — stripped in all example builds |

### 1.10 License terms

| Component | License |
|---|---|
| `NfcLibrary/*`, `source/nfc_example_*`, `source/tool`, `source/TML` | **NXP proprietary** — "Reproduction in whole or in part is prohibited without the written consent of the copyright owner." Development use under SW6705 download agreement. |
| MCUXpresso SDK drivers (`drivers/`, `board/`, `device/`, `CMSIS/`) | **BSD-3-Clause** / Apache-2.0 (CMSIS) |
| SW6705 package | **[LA_OPT_NXP_SW](https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html)** — personal, non-transferable, non-exclusive; development of Authorized Systems with NXP products; distribution typically object-code embedded in product |
| NFC patent notice (AN12989 §6.3) | Purchase of NXP NFC IC does **not** convey implied patent license for standard combinations |

**Practical note for Zephyr port:** Port logic and APIs; do not redistribute verbatim NXP headers as a standalone SDK. MCUXpresso LPC drivers are discarded; only `NxpNci20` + `NdefLibrary` + `TML` framing are ported.

---

## 2. PN7160 hardware / firmware capabilities (online + UM11495)

### 2.1 Official product positioning

NXP recommends for **no-OS / RTOS hosts** (LPC, i.MX RT, etc.):

- **SW6705** — NXP-NCI2.0 MCUXpresso examples  
- **AN13288** — integration guide  
- **UM11495** — PN7160 user manual (NCI extensions)  
- **PN7160/PN7161 datasheet** — RF/protocol summary  

For Linux/Android: **libnfc-nci** (not NfcRdLib).

Sources:

- [AN12989 — PN7160 quick start](https://www.nxp.com/docs/en/application-note/AN12989.pdf)
- [AN13288 — MCUXpresso examples guide](https://www.nxp.com/doc/AN13288)
- [SW6705 download](https://www.nxp.com/doc/SW6705)
- [PN7160/PN7161 datasheet](https://www.nxp.com/docs/en/data-sheet/PN7160_PN7161.pdf)
- [UM11495 — PN7160 user manual](https://www.nxp.com/docs/en/user-manual/UM11495.pdf)

### 2.2 RF / protocol support (silicon + firmware)

From datasheet + UM11495:

| Domain | PN7160 support |
|---|---|
| **Reader (PCD)** | ISO14443-A/B, ISO15693, MIFARE 1K/4K (TAG-CMD), MIFARE DESFire (ISO-DEP), FeliCa PCD, NFC Forum T1T–T5T |
| **Card (PICC / listen)** | ISO14443-A/B load modulation, T4T platform — **host implements protocol** |
| **P2P** | NFCIP-1/2, NFC-DEP |
| **Host interface** | NCI 2.0 (I²C or SPI) + NXP proprietary extensions |
| **NFCEE_NDEF** | Autonomous embedded T4T NDEF |
| **EMVCo profile** | Firmware EMVCo discovery/polling profiles (UM11495 §10.5) — **not EMVCo L1 certification** on PN7160 |
| **MIFARE Classic** | TAG-CMD interface with split authenticate + 1 ms timeout (UM11495 §7.1.3) |
| **ISO15693** | 26 kb/s in NCI 2.0; up to 3 VICCs in NFC Forum profile |

**NXP product guidance:** PN7160 **does not achieve EMVCo compliancy** for payment terminals — NXP directs payment use to PN5180, PN5190, or PN7220 ([NXP PN7160 product page](https://www.nxp.com/products/PN7160), community/docs). EMVCo *profile* in firmware aids **PICC interaction capture**, not certified payment acceptance.

**Card mode caveat** ([NXP docs — card mode](https://docs.nxp.com/bundle/PN7160_PN7161/page/topics/card_mode.html)): "PN7160 does not support a complete card protocol. This has to be handled by the host controller."

### 2.3 PN7160 vs PN7161

| Feature | PN7160 | PN7161 |
|---|---|---|
| NCI 2.0, same RF front-end | Yes | Yes (same silicon; model ID 0x71) |
| Apple **Enhanced Contactless Polling (ECP)** | **No** | **Yes** (requires Apple formal authorization) |
| Aliro tap with Apple Wallet express mode | **No** | **Yes** (with ECP + certification) |
| EMVCo ECP profile | No | Yes (newer firmware) |

Sources: [PN7160/PN7161 datasheet §1](https://www.nxp.com/docs/en/data-sheet/PN7160_PN7161.pdf), [Elechouse PN7160 vs PN7161](https://www.elechouse.com/compare/pn7160-vs-pn7161/), [NXP Aliro blog](https://www.nxp.com/company/about-nxp/smarter-world-blog/BL-ALIRO-1-0-USHERS-SECURE-HANDS-FREE-ACCESS)

### 2.4 NFC Reader Library (NfcRdLib) — not for PN7160

| Property | NFC Reader Library | NXP-NCI2.0 examples |
|---|---|---|
| Target hardware | Frontend ICs: PN5180, PN5190, CLRC663, PN512, PN74/76 | **NCI controllers: PN7160** |
| Host interface | Direct register / SPI to frontend | **NCI 2.0 over I²C/SPI** |
| EMVCo L1 test apps | Yes (on supported frontends) | No |
| PN7160 listed | **No** | **Yes (SW6705)** |

Source: [NFC Reader Library product page](https://www.nxp.com/design/design-center/development-boards-and-designs/nfc-reader-library-software-support-for-nfc-frontend-solutions:NFC-READER-LIBRARY), [UM10721 — Reader Library scope](https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/nfc/707/2/UM10721_NXP-NFC-Reader-Library-v3.010.pdf)

**Conclusion:** There is no "bigger" NXP host library that supersedes SW6705 for PN7160. Linux users get **libnfc-nci** (parallel stack, not MCUXpresso).

---

## 3. Decision matrix

| Capability | In hals_temp examples? | Needs another NXP lib? | Needs Flipper/custom? |
|---|---|---|---|
| **NDEF T4 read** | **Yes** — `RW_NDEF_T4T` + `READ_NDEF` | No | No (extend buffer >500 B) |
| **NDEF T2 read** | **Yes** — `RW_NDEF_T2T` | No | Minor — large NTAG needs SECTOR_SELECT |
| **MIFARE Classic** | **Partial** — raw auth/read demo + simplified NDEF | No | **Yes** — full poller, keys, multi-sector |
| **MIFARE DESFire** | **No** | No (not in any PN7160 lib) | **Yes** — ISO-DEP poller |
| **EMV** | **Partial** — PPSE SELECT demo only | No | **Yes** — AID tree, records, PDOL |
| **FeliCa** | **Partial** — T3T NDEF read in RWandCE | No | **Yes** — system codes, non-NDEF services |
| **ISO15693** | **Partial** — block R/W + `RW_NDEF_T5T` | No | **Yes** — full poller if beyond NDEF |
| **Aliro** | **No** | No | **Yes** — transcript poller; **PN7161** for Apple tap |
| **T4 CE emulate** | **Yes** — `T4T_NDEF_emu` + NFCEE_NDEF | No | No for NDEF CE |
| **P2P** | **Yes** — `P2P_NDEF` / SNEP | No | No (unless non-SNEP P2P needed) |

---

## 4. Recommendation

### 4.1 Port as-is sufficient for Phase 0–2?

| Phase | Verdict | What to port from SW6705 |
|---|---|---|
| **Phase 0** (bring-up) | **Yes** | `NxpNci20.c`, `Nfc_settings.h` blobs, TML framing → Zephyr I²C + IRQ |
| **Phase 1** (clone) | **Yes, as HAL + NDEF** | `RW_NDEF_*` for T2/T4 gate; `NxpNci_ReaderTagCmd` wrapper; discovery from `nfc_example_RW.c` / `RWandCE` tech tables |
| **Phase 2** (PN7160 emulate) | **Yes** | `T4T_NDEF_emu.c` + `NxpNci_ProcessCardMode()`; optional `NFCEE_NDEF_*` for autonomous CE |
| **Phase 1 expand** (Classic, DeSFire, EMV, Aliro, FeliCa, 15693) | **No — custom pollers** | Examples give transport only; implement per wave5 plans + Flipper facts |
| **Phase 3** (NFCT migrate) | N/A | nrfxlib listeners — separate from NXP tree |

**Do not pursue NfcRdLib** for PN7160 — wrong product class, no PN7160 transport.

### 4.2 If another NXP artifact is needed (reference only)

| Name | Path | Relation to SW6705 |
|---|---|---|
| **UM11495** | https://www.nxp.com/docs/en/user-manual/UM11495.pdf | NCI extension spec — essential for EMVCo profile, TAG-CMD, NFCEE |
| **PN7160/PN7161 datasheet** | https://www.nxp.com/docs/en/data-sheet/PN7160_PN7161.pdf | RF capabilities, firmware versions |
| **libnfc-nci** | Linux/Android stack (AN13189) | Alternative host stack; not for Zephyr port |
| **NfcRdLib** | https://www.nxp.com/design/.../NFC-READER-LIBRARY | **Not applicable** to PN7160 |

### 4.3 Gap list — project must build custom

Even with SW6705 + UM11495, the project must implement:

1. **`desfire_poller.c`** — ISO-DEP APDU sequences (Flipper `mf_desfire_poller` facts)
2. **`emv_poller.c`** — PPSE → AID SELECT → READ RECORD (Flipper payment app facts)
3. **`aliro_poller.c`** — SELECT/EXCHANGE transcript capture; **PN7161 + ECP** for real Apple Wallet tap
4. **`mf_classic_poller.c`** — multi-sector auth, key handling, non-NDEF dumps (Flipper + Crypto1)
5. **`felica_poller.c`** — beyond T3T NDEF (system codes, optional crypto)
6. **`iso15693_poller.c`** — full tag dump if not NDEF-only
7. **NDEF buffer / fragmentation** — raise 500 B cap; fragmented NDEF reassembly (pattern exists in `RWandCE`)
8. **RF tuning** — replace OM27160 blobs in `Nfc_settings.h` for nRF54L15 + PN7160 shield
9. **Async worker refactor** — `NxpNci_ProcessReaderMode` / `ProcessCardMode` are blocking; Phase 0.6 splits per architecture spec
10. **NFCT emulation (Phase 3)** — DeSFire/EMV/Aliro listeners on-die; not covered by NXP examples

---

## 5. Sources

| Source | URL |
|---|---|
| PN7160/PN7161 datasheet Rev. 4.1 | https://www.nxp.com/docs/en/data-sheet/PN7160_PN7161.pdf |
| UM11495 PN7160 user manual | https://www.nxp.com/docs/en/user-manual/UM11495.pdf |
| AN12989 quick start | https://www.nxp.com/docs/en/application-note/AN12989.pdf |
| AN13288 MCUXpresso guide | https://www.nxp.com/doc/AN13288 |
| SW6705 examples | https://www.nxp.com/doc/SW6705 |
| PN7160 product page | https://www.nxp.com/products/PN7160 |
| PN7160 card mode docs | https://docs.nxp.com/bundle/PN7160_PN7161/page/topics/card_mode.html |
| NFC Reader Library | https://www.nxp.com/design/design-center/development-boards-and-designs/nfc-reader-library-software-support-for-nfc-frontend-solutions:NFC-READER-LIBRARY |
| LA_OPT_NXP_SW license | https://www.nxp.com/docs/en/disclaimer/LA_OPT_NXP_SW.html |
| NXP Aliro 1.0 blog | https://www.nxp.com/company/about-nxp/smarter-world-blog/BL-ALIRO-1-0-USHERS-SECURE-HANDS-FREE-ACCESS |
| PN7160 vs PN7161 (ECP) | https://www.elechouse.com/compare/pn7160-vs-pn7161/ |

---

## Changelog

- **v1 (2026-06-14):** Initial audit — local SW6705 tree + UM11495/datasheet/web cross-check.
