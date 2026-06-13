# NFC Stack — Implementation Phases (PN7160 First)

> **Execution:** Superseded for day-to-day work by [`../../NFC_STACK_PLAN.md`](../../NFC_STACK_PLAN.md).
> Keep this file for historical PN7160-first phase ordering and gate criteria.

**Date:** 2026-06-13  
**Status:** LOCKED — supersedes the prior "Waves 1–6 first, then PN7160" sequencing  
**Platform:** Zephyr / Nordic NCS v3.2.4 · nRF54L15

> **What changed:** Build order only. The API shape is unchanged (`nfc_transport` HAL,
> `nfc_reader`, `nfc_stack`). Prove reader + emulator on one PN7160 chip before porting
> the same protocols to NFCT.

**Master design:** [`2026-06-13-nfc-final-design.md`](2026-06-13-nfc-final-design.md)  
**Architecture:** [`2026-06-12-nfc-stack-architecture.md`](2026-06-12-nfc-stack-architecture.md)

---

## Why PN7160 first

PN7160 is the Swiss Army knife: poll (reader), listen (card emulation), and multi-tech
discovery on one chip. NFCT is listen-only and cannot poll. Proving clone → emulate →
verify on PN7160 de-risks the protocol layer before the harder NFCT port.

**Product end-state is unchanged:** NFCT remains the default on-die emulator for shipping
firmware. PN7160-first is a development sequence, not a role swap.

---

## Phase overview

| Phase | Goal | Primary backend | Old wave mapping |
|---|---|---|---|
| **0** | PN7160 bring-up | PN7160 HAL | Wave 7a Phase 1–2 |
| **1** | PN7160 clone | PN7160 poll + store | Wave 7a Phase 3–4 + Wave 6 (minimal) |
| **2** | PN7160 emulate | PN7160 listen (Type-4 CE) | Wave 7b (new) |
| **3** | NFCT migration | NFCT listen | Waves 1–6 (repurposed) |

---

## Phase 0 — PN7160 bring-up

**Goal:** PN7160 talks NCI over I²C/SPI, IRQ fires, worker thread drains RX, discovery
can start/stop.

**NXP stack evaluation:** [`2026-06-14-pn7160-nxp-library-evaluation.md`](2026-06-14-pn7160-nxp-library-evaluation.md) — use NXP-NCI2.0 (SW6705) only; do not add NxpNfcRdLib/RFAL.

### Tasks

| # | Task | Source |
|---|---|---|
| 0.1 | Devicetree binding + board overlay (`nxp,pn7160.yaml`, I²C addr 0x28, IRQ GPIO) | wave7 §Phase 1 |
| 0.2 | TML: Zephyr `i2c`/`spi` + GPIO IRQ callback | Replace NXP `tml.c` |
| 0.3 | NCI CORE_RESET / CORE_INIT sequence | Port `NxpNci20.c` |
| 0.4 | Host transceive: send frame, await response with timeout | Port `NxpNci_HostTransceive` |
| 0.5 | `nfc_transport_pn7160.c` skeleton: init/shutdown, caps `{READER, multi-tech}` | New (Model B worker) |
| 0.6 | Worker thread: IRQ → drain RX → reschedule on `nfc_work_q` | RFAL pattern / HAL guide |
| 0.7 | Shell: `nfc_transport pn7160 fwver` | New |
| 0.8 | Discovery: `ConfigureMode(NXPNCI_MODE_RW)`, tech table, poll_start/stop | wave7 §Phase 2 |
| 0.9 | Shell: `nfc reader scan` — UID, protocol, interface | wave7 §Phase 2 |

### Key files

| File | Action |
|---|---|
| `src/nfc/hal/nfc_transport_pn7160.c` | Create |
| `src/nfc/hal/pn7160_nci.c` / `.h` | Create |
| `src/nfc/hal/pn7160_tml.c` / `.h` | Create |
| `dts/bindings/nfc/nxp,pn7160.yaml` | Create |
| `src/nfc/hal/nfc_transport_pn7160_shell_cmds.c` | Create |
| `src/nfc/reader/nfc_reader.h` | Create (stub API) |
| `src/nfc/reader/nfc_reader_engine.c` | Create (scan only) |
| `src/nfc/reader/nfc_reader_shell_cmds.c` | Create |

### Build vs port

| Source | Action |
|---|---|
| NXP `NxpNci20.c` | **Port** — CORE_RESET, discovery, state machine; no malloc |
| NXP `tml.c` | **Replace** — Zephyr drivers + IRQ |
| NXP `Nfc_settings.h` RF blobs | **Port** — const byte arrays in `pn7160_nci.c` |
| NXP `nfc_example_RW.c` | **Reference** — discovery tech table |
| Flipper | **Skip** — not needed for HAL bring-up |

### Success criteria (gate to Phase 1)

- [ ] `CORE_RESET` returns valid FW version on DK + PN7160 eval shield
- [ ] IRQ-driven RX works without polling loops blocking the main thread
- [ ] `nfc reader scan` detects a Type 2 or Type 4 tag (UID printed)
- [ ] `nfc_transport_poll_stop()` cleanly deactivates RF

---

## Phase 1 — PN7160 clone

**Goal:** Read physical tags into data models, serialize `.card` blobs. Start with NDEF
Type 4; expand to Ultralight, Classic, DeSFire, EMV, Aliro after NDEF round-trip works.

### Tasks — NDEF first (minimum gate)

| # | Task | Source |
|---|---|---|
| 1.1 | `nfc_transport_transceive()` → `NxpNci_ReaderTagCmd` wrapper | wave7 §Phase 3 |
| 1.2 | `ndef_poller.c`: SELECT NDEF AID, READ CC, READ NDEF file | wave7 + Flipper facts |
| 1.3 | Minimal `nfc_store` envelope (Wave 6 subset): save/load hex blob, CRC, flags | wave6-store |
| 1.4 | `nfc_reader_clone(tag)`: poller → serialize → store | wave7 §Phase 4 |
| 1.5 | Shell: `nfc reader clone <tag>` | wave7 §Phase 4 |

### Tasks — expand (after NDEF gate)

| # | Task | Source |
|---|---|---|
| 1.6 | `ultralight_poller.c` — page READ chain | wave5-ultralight + Flipper constants |
| 1.7 | `desfire_poller.c`, `emv_poller.c`, `aliro_poller.c` | wave5-* + wave7 §Phase 6 |
| 1.8 | MIFARE Classic auth+read (clone-only) | wave7 §Phase 3 |
| 1.9 | ISO15693 / FeliCa pollers — graceful `-ENOTSUP` on replay | wave7 §Phase 3 |
| 1.10 | Unit tests: NCI trace fixtures, flag assignment | wave7 tests |

### Key files

| File | Action |
|---|---|
| `src/nfc/services/ndef/ndef_poller.c` | Create |
| `src/nfc/store/nfc_store.h` / `.c` | Create (minimal envelope first) |
| `src/nfc/reader/nfc_reader_engine.c` | Extend — clone dispatch |
| `src/nfc/services/*/` pollers | Create per protocol (expand phase) |

### Build vs port

| Source | Action |
|---|---|
| NXP `RW_NDEF_*.c` | **Facts only** — informs `ndef_poller.c`; do not link NXP NDEF lib |
| NXP `nfc_example_RW.c` | **Reference** — MIFARE/15693 command examples |
| Flipper `*_poller.c` | **Reimplement** — control-flow as comment reference; cite source |
| Flipper constants (`mf_ultralight.h`, etc.) | **Import facts** — command bytes, page counts |
| Wave 6 full store | **Subset first** — envelope + CRC + flags; NVS backend stub OK |

### Success criteria (gate to Phase 2)

- [ ] `nfc reader clone tag1` on a physical Type-4 NDEF tag produces valid hex blob
- [ ] Blob reloads via `nfc_store_load` without deserialize errors
- [ ] UID + NDEF bytes match the physical tag on re-read (`nfc reader scan` after clone)
- [ ] Store flags set: `READER_CAPTURED`, `EMULATION_COMPLETE` for full NDEF capture

---

## Phase 2 — PN7160 emulate

**Goal:** Same chip that cloned the tag can emulate it. Type-4 ISO-DEP / T4T NDEF listen
using NXP CE patterns. Verify loop runs entirely on PN7160 (poll emulated listen tag).

### Tasks

| # | Task | Source |
|---|---|---|
| 2.1 | Enable `NXPNCI_MODE_CARDEMU` in Kconfig gate (`CONFIG_NFC_PN7160_LISTEN`) | final design §7b |
| 2.2 | `NxpNci_ConfigureMode(RW \| CARDEMU)` + discovery with `MODE_LISTEN` | NXP `nfc_example_RWandCE.c` |
| 2.3 | `nfc_transport_pn7160_listen_*`: start/stop, send_response, callbacks | Port `ProcessCardMode` |
| 2.4 | Type-4 NDEF listener: SELECT NDEF AID, READ/UPDATE BINARY on CC + NDEF file | Port `T4T_NDEF_emu.c` logic |
| 2.5 | Wire listener through `nfc_stack` with PN7160 backend (not NFCT) | Same API, different HAL |
| 2.6 | `nfc_reader_verify(tag)`: poll PN7160-emulated tag vs stored blob | wave7 §Phase 5 |
| 2.7 | Live NDEF persist: UPDATE BINARY → data model → `nfc_store_on_dirty` | wave6-store hook |
| 2.8 | Optional: NFCEE NDEF preload (`NFCEE_NDEF_*`) for simple tags | NXP `CE_scenario2.c` |

### Key files

| File | Action |
|---|---|
| `src/nfc/hal/nfc_transport_pn7160.c` | Extend — listen sub-API |
| `src/nfc/services/ndef/ndef_listener.c` | Create (or reuse from Wave 5a design) |
| `src/nfc/nfc_stack.c` | Create — routes to PN7160 listen backend |
| `src/nfc/framing/apdu_assembler.c` | Create (needed for ISO-DEP lane) |
| `src/nfc/router/aid_router.c` | Create (needed for NDEF AID dispatch) |

> **Note:** Phase 2 pulls in a **minimal card slice** (framing + router + ndef listener +
> stack orchestrator) wired to PN7160 listen — not the full NFCT backend. Same modules,
> different HAL target.

### Build vs port

| Source | Action |
|---|---|
| NXP `T4T_NDEF_emu.c` | **Port** — SELECT/READ/UPDATE BINARY handler |
| NXP `NxpNci_ProcessCardMode` | **Port** — async state machine on worker thread |
| NXP `CE_scenario2.c` | **Reference** — NFCEE NDEF optional path |
| NXP `nfc_example_RWandCE.c` | **Reference** — combined RW+CE discovery table |
| Flipper `*_listener.c` | **Reimplement** — same discipline as pollers |
| NFCT / nrfxlib | **Skip** — not used in Phase 2 |

### Success criteria (gate to Phase 3)

- [ ] Clone physical NDEF tag → load blob → PN7160 emulate → PN7160 verify **PASS**
- [ ] Phone or external reader can read emulated NDEF (SELECT + READ BINARY)
- [ ] UPDATE BINARY on NDEF file updates data model + persists via store
- [ ] Stop/start/reload preserves written NDEF content

---

## Phase 3 — NFCT migration

**Goal:** Port the proven protocol stack from PN7160 to on-die NFCT. Waves 1–6 become
the NFCT port plan — same data models, same `.card` format, same API, different listen
HAL backend.

### What Waves 1–6 mean now

| Wave | Original goal | Phase 3 meaning |
|---|---|---|
| **1** | HAL + NFCT listen + capability model | Port listen HAL; poller seam already done on PN7160 |
| **2** | APDU framing | Port if not already built in Phase 2; verify against PN7160 transcripts |
| **3** | AID router + ISO-DEP lane | Port; validate dispatch matches PN7160 listener behavior |
| **4** | `nfc_stack` orchestrator | Port backend routing: card ops → NRFX, reader ops → PN7160 |
| **5a–5e** | Protocol services (NDEF, UL, DeSFire, EMV, Aliro) | Port listeners to NFCT; pollers already on PN7160 |
| **6** | Full store + live persist + NVS backend | Extend Phase 1 minimal store; blobs interchange with PN7160 captures |

### Tasks (high level)

| # | Task |
|---|---|
| 3.1 | `nfc_transport_nrfx.c` — wrap `nfc_t4t_lib` / `nfc_t2t_lib` |
| 3.2 | Dual-backend Kconfig: compile both NRFX + PN7160; role selects runtime path |
| 3.3 | Port ndef/ultralight/desfire/emv/aliro **listeners** to NFCT raw ISO-DEP mode |
| 3.4 | Re-run full verify loop: PN7160 clone → NFCT emulate → PN7160 verify |
| 3.5 | T2T read-only path (`nfc_t2t_lib`) for Type-2 NDEF presentation |
| 3.6 | Aliro: listener on NFCT, poller stays on PN7160 (locked dual-side story) |

### Build vs port

| Source | Action |
|---|---|
| nrfxlib `nfc_t4t_lib`, `nfc_t2t_lib` | **Wrap** — Wave 1 NRFX backend |
| PN7160 Phase 1–2 code | **Keep** — reader path unchanged |
| PN7160 Phase 2 listeners | **Reuse logic** — swap HAL calls to NRFX |
| Flipper | **Same discipline** — facts + reimplemented control flow |

### Success criteria (product gate)

- [ ] Same `.card` blob works on both PN7160 emulate and NFCT emulate
- [ ] PN7160 clone → NFCT emulate → PN7160 verify **PASS** for NDEF, DeSFire, Aliro
- [ ] Live NDEF persist on NFCT survives stop/start/reload
- [ ] Dual-backend firmware builds with both antennas wired

---

## Wave number mapping (old → new)

| Old sequencing | New phase | Notes |
|---|---|---|
| Wave 1 (HAL NFCT) | **Phase 3** (Wave 1 plan) | Listen HAL only; poller seam done in Phase 0 |
| Wave 2 (framing) | **Phase 2 or 3** | Built early if needed for PN7160 CE; finalized on NFCT |
| Wave 3 (router) | **Phase 2 or 3** | Same — needed for PN7160 NDEF listener |
| Wave 4 (nfc_stack) | **Phase 2** (PN7160) then **Phase 3** (dual-backend) | Stack exists before NFCT |
| Wave 5a–5e (services) | **Phase 1** (pollers) + **Phase 2** (listeners on PN7160) + **Phase 3** (listeners on NFCT) | Data models shared |
| Wave 6 (store) | **Phase 1** (minimal) → **Phase 3** (full) | Envelope first, NVS later |
| Wave 7a (PN7160 reader) | **Phase 0 + Phase 1** | Now first, not last |
| Wave 7b (PN7160 CE) | **Phase 2** | Required gate, not optional |

---

## Build vs port summary by phase

| Phase | NXP `hals_temp` | Flipper `lib/nfc` | nrfxlib |
|---|---|---|---|
| **0** | Port NCI core + TML replace | Skip | Skip |
| **1** | Facts from RW_NDEF; reference RW example | Reimplement pollers; import constants | Skip |
| **2** | Port T4T_NDEF_emu + ProcessCardMode | Reimplement listeners | Skip |
| **3** | Keep PN7160 code as-is | Same listener logic, new HAL target | Wrap t4t/t2t libs |

---

## Success gates (must pass before next phase)

```
Phase 0 ──[CORE_RESET OK, scan detects tag]──> Phase 1
Phase 1 ──[clone NDEF tag, valid .card blob]──> Phase 2
Phase 2 ──[clone→emulate→verify PASS on PN7160]──> Phase 3
Phase 3 ──[same blob on NFCT emulate, verify PASS]──> Product ready
```

No phase may be skipped. Phase 2 is the critical proof: if clone+emulate+verify works
on one chip, the protocol layer is validated before NFCT-specific constraints (T2T
read-only, no poll, single-library ownership) are layered on.

---

## API shape (unchanged)

Only build order changes. These contracts stay locked:

| Layer | API | Phase 0 | Phase 1 | Phase 2 | Phase 3 |
|---|---|---|---|---|---|
| HAL | `nfc_transport_*` | PN7160 poll | + transceive | + listen sub-API | + NRFX listen |
| Reader | `nfc_reader_*` | scan | clone | verify | verify (NFCT target) |
| Card | `nfc_stack_*` | — | — | load/start/stop | + NFCT backend |
| Store | `nfc_store_*` | — | minimal save/load | + on_dirty | + NVS backend |
| Services | `nfc_service_t` vtable | — | pollers | listeners | listeners on NFCT |

Dual-backend routing (final product):

```
nfc_reader_*  →  nfc_transport_pn7160  (poll)
nfc_stack_*   →  nfc_transport_nrfx     (listen, default)
              →  nfc_transport_pn7160    (listen, clone-only replay — optional)
```

---

## Build from this repo

All NFC stack and PN7160 work is implemented in **writable_ndef_msg**. From the repo root
(NCS v3.2.4 environment):

**Existing NFCT Type-4 sample (unchanged):**

```bash
west build -b nrf54l15dk/nrf54l15/cpuapp
```

**PN7160 module scaffold (Phase 0):**

```bash
west build -b nrf54l15dk/nrf54l15/cpuapp \
  -- -DDTC_OVERLAY_FILE=boards/overlays/pn7160_i2c.overlay \
     -DEXTRA_CONF_FILE=overlay-pn7160.conf
```

**Full NFC stack + PN7160 (Phase 0.5+):**

```bash
west build -b nrf54l15dk/nrf54l15/cpuapp \
  -- -DCONFIG_NFC_STACK=y \
     -DDTC_OVERLAY_FILE=boards/overlays/pn7160_i2c.overlay \
     -DEXTRA_CONF_FILE=overlay-pn7160.conf
```

Module wiring: `ZEPHYR_EXTRA_MODULES` in root `CMakeLists.txt` points at
`modules/nfc_pn7160/`. Adjust I2C bus and GPIO pins in
`boards/overlays/pn7160_i2c.overlay` for your eval shield.

---

## Related documents

| Document | Role |
|---|---|
| [`2026-06-13-nfc-final-design.md`](2026-06-13-nfc-final-design.md) | Master design — hardware topology, API, card matrix |
| [`../waves/wave7-pn7160-reader.md`](../waves/wave7-pn7160-reader.md) | Phase 0/1 task detail |
| `docs/superpowe../waves/wave1-hal.md` … `wave6-store.md` | Phase 3 port plans |
| [`2026-06-13-nfct-pn7160-capability-matrix.md`](2026-06-13-nfct-pn7160-capability-matrix.md) | Per-protocol limits |

---

## Changelog

- **v1.1 (2026-06-14):** Added §Build from this repo; implementation home is `writable_ndef_msg`.
