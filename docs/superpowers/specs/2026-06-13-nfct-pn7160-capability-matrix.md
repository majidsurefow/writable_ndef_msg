# NFCT + PN7160 Capability Matrix & Card Support Policy

**Date:** 2026-06-13  
**Status:** Architecture-of-record supplement  
**Locked split:** [`2026-06-13-locked-architecture-summary.md`](2026-06-13-locked-architecture-summary.md)

---

## Card Support Matrix

| Card / Protocol | PN7160 Read / Clone | NFCT Emulate | Category | Notes / Limitations |
|---|---|---|---|---|
| **NDEF (Type 4)** | Yes — SELECT AID `D276…`, READ CC + NDEF file | Yes — `nfc_t4t_lib` raw ISO-DEP or NDEF RW mode | **Full round-trip** | Clone → load → emulate with `EMULATION_COMPLETE`; reader `UPDATE BINARY` on `E104` persists via `nfc_store_on_dirty` (§1.1) |
| **NDEF (Type 2)** | Yes — READ pages, parse NDEF TLV | Yes — `nfc_t2t_lib` READ-only emulation | **Full round-trip** (NDEF content only) | T2T lib handles **READ (0x30) only** — no WRITE emulation in nrfxlib; writable NDEF on T2 uses T4 path for persist |
| **MIFARE Ultralight** (NTAG / UL11 / UL21) | Yes — page READ via Type 2 poller | **Partial** — T4T-via-NDEF adapter OR read-only T2T NDEF | **Partial** | Full page-level mimic needs custom raw listener; nrfxlib T2T cannot WRITE/A2; wave5 Ultralight uses T4 adapter (`DECISION-UL-3`) |
| **MIFARE Classic** | Yes — auth + block read (`NxpNci_ReaderTagCmd`) | **No** | **Clone-only** | Crypto1 not in NFCT; store for replay on PN7160 only / future external emulator |
| **MIFARE Plus / DESFire** | Yes — ISO-DEP poller (partial without keys) | Yes — DeSFire listener (hand-provisioned / partial data) | **Partial** | Reader capture: `READ_ONLY_PARTIAL` without keys; emulation uses stored visible files |
| **EMV (payment)** | Yes — PPSE + AID enumeration | Yes — static record replay | **Partial** | No live bank auth; poller captures public SELECT responses |
| **Aliro** | Yes — poller captures SELECT/EXCHANGE transcript | Yes — Wave 5e listener (PSA crypto) | **Emulate-only** (typical) + **Partial clone** | Credentials are provisioned, not key-cloned; poller on PN7160, listener on NFCT |
| **ISO 15693** | Yes — read/write blocks in NXP example | **No** | **Clone-only** | Flipper has listener; NFCT has no 15693 |
| **FeliCa** | Yes — poll + sync (`NxpNci` NFCF) | **No** | **Clone-only** | Flipper emulates FeliCa; NFCT does not |
| **ISO 14443-B** | Yes — detect + ATTRIB | **No** | **Clone-only** | No NFCT Type B emulation in nrfxlib |
| **ST25TB** | Yes — Flipper poller exists | **No** | **Clone-only** | Low priority |

### Category legend

| Category | Meaning |
|---|---|
| **Full round-trip** | PN7160 clone → `.card` → NFCT emulate → PN7160 verify; live persist where applicable |
| **Clone-only** | PN7160 reads and stores; NFCT cannot faithfully emulate physical protocol |
| **Emulate-only** | Credential provisioned by hand / config — no physical card required |
| **Partial** | Visible public data round-trips; secrets / auth state incomplete |

---

## NFCT Type 2 vs Type 4 Limitations

*Evidence from nrfxlib NCS v3.2.4 — not inferred.*

### Type 4 (`nfc_t4t_lib.h` + `type_4_tag.rst`)

| Capability | Supported? | Evidence |
|---|---|---|
| ISO 14443-4A / ISO-DEP | Yes | Header: "implements ISO14443-4A protocol" |
| Raw ISO-DEP (all APDUs to app) | Yes | `NFC_T4T_EMUMODE_PICC` + `NFC_T4T_EVENT_DATA_IND` |
| NDEF file emulation (RO) | Yes | `nfc_t4t_ndef_staticpayload_set` |
| NDEF file emulation (RW) | Yes | `nfc_t4t_ndef_rwpayload_set` + `NFC_T4T_EVENT_NDEF_UPDATED` |
| DeSFire / EMV / Aliro APDUs | Yes (raw mode) | Raw mode delivers full APDU stream to application |
| Max payload | 0xFFF0 | `NFC_T4T_MAX_PAYLOAD_SIZE` |
| FWI / EMV timing | Yes | `NFC_T4T_PARAM_FWI`, max 7 for EMV |
| NFCID1 rotation | Yes | `NFC_T4T_PARAM_NFCID1` — 4/7/10 bytes |
| **Reader / poll** | **No** | No poller API; on-die NFCT is listen-only |

### Type 2 (`nfc_t2t_lib.h` + `type_2_tag.rst`)

| Capability | Supported? | Evidence |
|---|---|---|
| NFC-A listen | Yes | `nfc_t2t_emulation_start` |
| NDEF via implicit TLV | Yes | `nfc_t2t_payload_set` |
| Custom TLV layout | Yes | `nfc_t2t_payload_raw_set` |
| Internal bytes override | Yes | `nfc_t2t_internal_set` (10 bytes) |
| **WRITE command (0xA2)** | **No** | Doc §Command set: "supports only one command type: READ" |
| **Writable tag emulation** | **No** | Doc: "read-only state… cannot be erased or re-written" |
| Lock bytes | Fixed read-only | Static/dynamic lock bytes set to protect memory |
| Max NDEF payload | 988 bytes | `NFC_T2T_MAX_PAYLOAD_SIZE` |
| Raw payload max | 1008 bytes | `NFC_T2T_MAX_PAYLOAD_SIZE_RAW` |
| Events | FIELD_ON, FIELD_OFF, DATA_READ | No write/update event |

### Hardware / NFCT peripheral

| Question | Answer | Evidence |
|---|---|---|
| Same silicon for T2 and T4? | Yes — one NFCT peripheral | `integration_notes.rst`: "each library must be the only user of NFCT" |
| T2 + T4 simultaneously? | **No** | Only one of `NFC_T2T_NRFXLIB` / `NFC_T4T_NRFXLIB` may own NFCT at a time |
| Mode switching? | **Yes — runtime** | Stop one lib (`*_emulation_stop` + `*_done`), init the other; full re-setup required |
| Timer dependency | One TIMER per NFCT session | TIMER4 (nRF52) / TIMER2 (nRF5340) — exclusive |
| HFXO | Required when field present | `nfc_platform.h` / `integration_notes.rst` |
| EasyDMA buffer sizes | T2: 16 B · T4: 259×2 B | `NFC_PLATFORM_T2T_BUFFER_SIZE`, `NFC_PLATFORM_T4T_BUFFER_SIZE` in `nfc_platform.h` |
| nRF54L15 | Same nrfxlib contract as nRF52 | Architecture spec §6; prebuilt libs in nrfxlib |

### Ultralight decision (recommendation)

| Path | When to use | Verdict |
|---|---|---|
| **T4T-via-NDEF adapter** (current wave5) | Writable NDEF + live persist + DeSFire/EMV/Aliro coexistence on same firmware | **Default — keep** |
| **Native T2T** (`nfc_t2t_lib`) | Read-only NDEF broadcast mimicking NTAG physical layout; no WRITE | **Optional** — add `NFC_PROFILE_ULTRALIGHT_T2T` if physical T2 fidelity needed for verify |
| **Custom raw T2 listener** | Full page WRITE mimic (A2, C0, proprietary) | **Future** — not in nrfxlib; would bypass `nfc_t2t_lib` and talk to HAL raw lane |

**Conclusion:** Architecture spec §4.3 open item is **resolved**: `nfc_t2t_lib` does **not** support WRITE — native Ultralight **cannot** replace the T4 adapter for writable clones. Use T2T path only for read-only NDEF-on-T2 presentation.

---

## PN7160 reader capabilities (from NXP NCI examples)

| Technology | NXP API | Clone support |
|---|---|---|
| NFC-A passive | `TECH_PASSIVE_NFCA` | T2T, T4T, MIFARE, Ultralight |
| NFC-B passive | `TECH_PASSIVE_NFCB` | ISO14443-B cards |
| NFC-F passive | `TECH_PASSIVE_NFCF` | FeliCa |
| ISO 15693 | `TECH_PASSIVE_15693` | Vicinity tags |
| Raw transceive | `NxpNci_ReaderTagCmd` | Any TAG-CMD / ISO-DEP payload |
| High-level NDEF | `NxpNci_ProcessReaderMode(READ_NDEF)` | Type 2/4 NDEF extraction |
| Card emulation on PN7160 | `NxpNci_ProcessCardMode` + T4T NDEF emu | **Wave 7b optional** — NFCT remains default; see final design |

---

## Flipper reuse map

| Protocol | Flipper poller (→ PN7160 port) | Flipper listener (→ NFCT port) | Copy vs reimplement |
|---|---|---|---|
| ISO14443-3A | `iso14443_3a_poller.c` | `iso14443_3a_listener.c` | Reimplement — anticollision/SEL facts |
| ISO14443-4A | `iso14443_4a_poller.c` | `iso14443_4a_listener.c` | Reimplement — RATS/PPS/IBLOCK facts |
| MIFARE Ultralight | `mf_ultralight_poller.c` | `mf_ultralight_listener.c` | **Constants** from `.h`; poller port; listener **skip** (use T4 adapter) |
| MIFARE Classic | `mf_classic_poller.c` | `mf_classic_listener.c` | Poller port + `crypto1.c` facts; listener N/A on NFCT |
| MIFARE DESFire | `mf_desfire_poller.c` | NULL | Poller partial port (wave5) |
| EMV | (via ISO14443-4 + app) | — | PPSE flow from Flipper payment app facts |
| FeliCa | `felica_poller.c` | `felica_listener.c` | Poller only — clone-only |
| ISO15693 | `iso15693_3_poller.c` | `iso15693_3_listener.c` | Poller only — clone-only |
| SLIX | `slix_poller.c` | `slix_listener.c` | Poller only |
| Aliro | — (no Flipper module) | — | Use `aliro/` + wave5-aliro |

**Registry reference:** `flipperzero/lib/nfc/protocols/nfc_poller_defs.c` (11 pollers) vs `nfc_listener_defs.c` (7 non-NULL listeners) — mirrors our NULL-table pattern for unsupported combos.

---

## Wave 5 cross-reference

| Wave plan | NFCT listener | PN7160 poller (Wave 7) | Persist flags |
|---|---|---|---|
| `wave5-ndef.md` | T4T raw + NDEF service | `ndef_poller.c` | `READER_CAPTURED \| EMULATION_COMPLETE` |
| `wave5-ultralight.md` | T4T NDEF adapter | `ultralight_poller.c` | Page model; no live NDEF→page sync |
| `wave5-desfire.md` | ISO-DEP service | `desfire_poller.c` | `READ_ONLY_PARTIAL` without keys |
| `wave5-emv.md` | Static records | `emv_poller.c` | `READ_ONLY_PARTIAL` |
| `wave5-aliro.md` | PSA listener | `aliro_poller.c` | `HAND_AUTHORED` or `READ_ONLY_PARTIAL` |

---

## Policy summary

1. **Default product path:** PN7160 clones → NFCT emulates on T4T (NDEF, DeSFire, EMV, Aliro) or T2T-read-only for physical T2 NDEF tags.
2. **Do not plan ST25R3916** as reader backend — PN7160 only.
3. **Do not expect NFCT to poll** — all reader traffic goes through PN7160 poller sub-API.
4. **GPL pragmatism:** import Flipper facts and selectively port poller logic; cite sources.
5. **Verify loop is mandatory** for full-round-trip protocols before marking wave complete.

---

## Changelog

- **v1 (2026-06-13):** Initial matrix from nrfxlib header/doc audit, NXP NCI examples, Flipper registry, locked architecture 2026-06-13.
