# Wave 7: PN7160 Reader Backend + Capture Engine

**Status:** DRAFT — 2026-06-13  
**Layer:** `hal/nfc_transport` (PN7160 backend) + `reader/nfc_reader` (poll/capture/verify engine)

> **Implementation order (LOCKED 2026-06-13):** This plan is **Phase 0 + Phase 1** in
> [`2026-06-13-implementation-phases.md`](../specs/2026-06-13-implementation-phases.md).
> PN7160 comes **first** — before NFCT (Waves 1–6). NFCT port is **Phase 3**, deferred
> until PN7160 clone + emulate verify passes on the same chip.

> **Architecture context:** PN7160 is reader primary (+ CE in Phase 2). NFCT is the
> default product emulator but is built **after** PN7160 proof. See
> [`2026-06-13-nfc-final-design.md`](../specs/2026-06-13-nfc-final-design.md).

---

## Goal

Bring up the PN7160 as the sole reader/cloner backend **first**: poll physical tags, run
protocol pollers into data models, emit `.card` blobs. Phase 2 (separate) adds PN7160 card
emulation and verify on the same chip. NFCT emulate comes in Phase 3.

**Success criteria (DK + PN7160 eval board — Phase 0/1):**
- `nfc reader scan` detects Type 2 / Type 4 / ISO15693 tags
- `nfc reader clone <tag>` produces a valid `.card` hex blob
- Re-read after clone matches UID + NDEF bytes

**Success criteria (Phase 2 — PN7160 emulate, not NFCT yet):**
- `nfc_stack load <tag>` + PN7160 emulate → `nfc reader verify` reports PASS
- Live NDEF UPDATE BINARY persists via store

**Phase 3 adds:** same blob on NFCT emulate → PN7160 verify PASS

---

## Dependencies

| Dependency | Required before Phase 0/1? | Notes |
|---|---|---|
| **Waves 1–6 NFCT slice** | **No — deferred to Phase 3** | Was a gate; now built after PN7160 proof |
| Wave 1 poller sub-API (§A) | Yes | `nfc_transport_poll_start/stop/transceive` — interface locked |
| Minimal store (Wave 6 subset) | Yes for clone | Envelope + CRC + flags; full NVS in Phase 3 |
| Wave 3 router + lanes | Phase 2 only | Needed for PN7160 NDEF listener, not Phase 1 clone |
| NFCT T2T backend | Phase 3 | Deferred |
| ST25R3916 RFAL | **No — demoted** | Do not implement |

**Parallel work allowed:** None required before Phase 0. Phase 1 minimal store can be built
inline; do not wait for full Wave 6.

---

## Files to create

### HAL / transport (PN7160)

| File | Purpose |
|---|---|
| `src/nfc/hal/nfc_transport_pn7160.c` | Worker-driven HAL backend: capabilities, lifecycle, poller sub-API, event bridge to `nfc_work_q` |
| `src/nfc/hal/pn7160_nci.c` | NCI 2.0 framing: CORE_RESET, SET_CONFIG, RF_DISCOVER, DATA_PACKET, deactivate |
| `src/nfc/hal/pn7160_nci.h` | Internal NCI types (not exported past HAL) |
| `src/nfc/hal/pn7160_tml.c` | Transport Mapping Layer: I²C or SPI read/write, IRQ-driven RX |
| `src/nfc/hal/pn7160_tml.h` | `pn7160_tml_send/recv`, connect/disconnect |
| `dts/bindings/nfc/nxp,pn7160.yaml` | DT binding: `reg`, `irq-gpios`, optional `reset-gpios`, `enable-gpios` |
| `src/nfc/hal/nfc_transport_pn7160_shell_cmds.c` | `nfc_transport pn7160 fwver/discover/stats` |

### Reader engine

| File | Purpose |
|---|---|
| `src/nfc/reader/nfc_reader.h` | Public reader API: init/start/stop, scan, clone, verify |
| `src/nfc/reader/nfc_reader_engine.c` | Discovery loop, technology→poller dispatch, `.card` serialization |
| `src/nfc/reader/nfc_reader_shell_cmds.c` | `nfc reader scan/clone/verify/stats` |
| `src/nfc/reader/CMakeLists.txt` | Gated on `CONFIG_NFC_ROLE_READER` |
| `src/nfc/reader/Kconfig` | Reader work-queue sizes, default tech mask |

### Protocol pollers (Wave 7 extensions — one per enabled protocol)

| File | Protocol |
|---|---|
| `src/nfc/services/ndef/ndef_poller.c` | Type 4 NDEF read (SELECT + READ BINARY) |
| `src/nfc/services/ultralight/ultralight_poller.c` | Type 2 page READ chain |
| `src/nfc/services/desfire/desfire_poller.c` | Partial ISO-DEP capture (no keys) |
| `src/nfc/services/emv/emv_poller.c` | PPSE + AID SELECT transcript |
| `src/nfc/services/aliro/aliro_poller.c` | Aliro SELECT/EXCHANGE capture |

### Tests

| Path | Coverage |
|---|---|
| `tests/unit/pn7160_nci/` | Pure NCI frame encode/decode (no hardware) |
| `tests/unit/nfc_reader/` | Clone flag assignment, verify diff logic |
| On-target (DK + PN7160 shield) | Discovery, T4T NDEF clone round-trip |

---

## Phased tasks

### Phase 1 — HAL bring-up (8 tasks)

| # | Task | Est. |
|---|---|---|
| 1.1 | Author `nxp,pn7160.yaml` + board overlay (I²C addr `0x28` or SPI + IRQ GPIO) | 30m |
| 1.2 | Implement `pn7160_tml.c`: Zephyr `i2c`/`spi` + `gpio_callback` for HOST_IRQ | 45m |
| 1.3 | Port NCI CORE_RESET / CORE_INIT sequence from `NxpNci20.c` (`NxpNci_CheckDevPres`) | 30m |
| 1.4 | Implement `NxpNci_HostTransceive` equivalent: send NCI frame, await response with timeout | 45m |
| 1.5 | `nfc_transport_pn7160.c` skeleton: `init/shutdown`, static caps `{READER only, multi-tech}` | 30m |
| 1.6 | Worker thread: `k_work` loop polls IRQ + drains RX; self-reschedules (RFAL pattern) | 45m |
| 1.7 | Shell: `nfc_transport pn7160 fwver` reads 3-byte FW from CORE_RESET_NTF | 15m |
| 1.8 | On-target smoke: `CORE_RESET` OK, FW version printed | 15m |

### Phase 2 — Poll / detect (6 tasks)

| # | Task | Est. |
|---|---|---|
| 2.1 | `NxpNci_ConfigureSettings` + `ConfigureMode(NXPNCI_MODE_RW)` — strip P2P/CE from build | 30m |
| 2.2 | Discovery table: `TECH_PASSIVE_NFCA`, `NFCB`, `NFCF`, `PASSIVE_15693` (from `nfc_example_RW.c`) | 30m |
| 2.3 | `nfc_transport_poll_start(tech_mask)` → `NxpNci_StartDiscovery` + notify callback | 45m |
| 2.4 | `on_tag_detected(tech)` / `on_tag_removed()` bridged to `nfc_work_q` | 30m |
| 2.5 | `nfc_transport_poll_stop()` → `NxpNci_StopDiscovery` | 15m |
| 2.6 | Shell: `nfc reader scan` — prints UID, protocol, interface | 30m |

### Phase 3 — Per-protocol pollers (10 tasks)

| # | Task | Est. |
|---|---|---|
| 3.1 | `nfc_transport_transceive()` → `NxpNci_ReaderTagCmd` wrapper with errno mapping | 30m |
| 3.2 | `ndef_poller.c`: SELECT NDEF AID, READ CC, READ NDEF file → `ndef_data_t` | 60m |
| 3.3 | `ultralight_poller.c`: anticollision + READ pages 0..N (Flipper `mf_ultralight_poller` facts) | 60m |
| 3.4 | `desfire_poller.c`: GET_VERSION + limited READ_DATA (no auth → partial) | 45m |
| 3.5 | `emv_poller.c`: PPSE SELECT + candidate AID enumeration | 45m |
| 3.6 | `aliro_poller.c`: SELECT Aliro AID + EXCHANGE transcript capture | 60m |
| 3.7 | Reader engine: technology→poller registry (Flipper table-with-NULL pattern) | 30m |
| 3.8 | ISO15693 + FeliCa: register pollers, return `-ENOTSUP` gracefully on NFCT replay | 30m |
| 3.9 | MIFARE Classic: auth+read via `NxpNci_ReaderTagCmd` (clone-only, no NFCT emulate) | 45m |
| 3.10 | Unit tests: poller data-model fill from recorded NCI trace fixtures | 45m |

### Phase 4 — Capture → `.card` (5 tasks)

| # | Task | Est. |
|---|---|---|
| 4.1 | `nfc_reader_clone(tag)`: poller → `serialize()` per service → `nfc_store` envelope | 45m |
| 4.2 | Set `NFC_STORE_ENTRY_FLAG_READER_CAPTURED` + completeness flags per §1.1 | 30m |
| 4.3 | Shell: `nfc reader clone <tag>` dumps hex blob (same format as `store save`) | 30m |
| 4.4 | `nfc_stack_load(tag)` path tested with reader-produced blob | 30m |
| 4.5 | Error paths: `-EBUSY` if stack STARTED, `-ENOTSUP` for unsupported tech | 15m |

### Phase 5 — Verify loop (4 tasks)

| # | Task | Est. |
|---|---|---|
| 5.1 | `nfc_reader_verify(tag)`: re-poll NFCT emulated tag after `nfc_stack_start` | 45m |
| 5.2 | Compare UID, NDEF bytes, visible page/APDU transcript | 45m |
| 5.3 | Shell output: PASS/FAIL per field | 15m |
| 5.4 | Integration test script: clone physical NDEF tag → load → verify | 30m |

### Phase 6 — Aliro poller (3 tasks)

| # | Task | Est. |
|---|---|---|
| 6.1 | Wire `aliro_poller.c` to Wave 5e data model (public key + config only) | 45m |
| 6.2 | Capture flags: `READER_CAPTURED \| READ_ONLY_PARTIAL` | 15m |
| 6.3 | End-to-end: provision NFCT listener credential → PN7160 poller reads → compare | 60m |

**Total: ~34 tasks · ~18–22 engineering hours · 6 phases**

---

## Port from NXP examples vs clean-room

| NXP example artifact | Action |
|---|---|
| `NfcLibrary/NxpNci20/src/NxpNci20.c` | **Port** CORE_RESET, discovery, `ReaderTagCmd`, `ProcessReaderMode` state machine — re-express in Zephyr C, no malloc, MISRA |
| `source/TML/tml.c` | **Replace** with Zephyr `i2c`/`spi` driver + GPIO IRQ (same framing: 2-byte header on I²C) |
| `NfcLibrary/NdefLibrary/src/RW_NDEF_*.c` | **Facts only** — high-level READ_NDEF flow informs `ndef_poller.c`; do not link NXP NDEF lib |
| `NfcLibrary/NdefLibrary/src/T4T_NDEF_emu.c` | **Phase 2** — port for PN7160 listen; not NFCT |
| `source/nfc_example_RW.c` | **Reference** — discovery tech table, MIFARE/15693 command examples |
| `Nfc_settings.h` | **Port** RF config blobs as `const` byte arrays in `pn7160_nci.c` |

**Clean-room:** `nfc_transport_pn7160.c` event bridge, capability descriptor, stats, and reader engine are ours — follow `NFC_HAL_AUTHORING_GUIDE.md` Model B (worker-driven).

---

## Flipper reuse strategy

| Need | Flipper source | Action |
|---|---|---|
| Poller sequencing / state machines | `lib/nfc/protocols/*_poller.c` | Reimplement in Zephyr; copy control-flow as comment reference |
| Ultralight page counts / types | `mf_ultralight.h`, `mf_ultralight.c` | Import constants only (already in wave5-ultralight) |
| DeSFire command codes | `mf_desfire_poller.c` | Facts already in wave5-desfire |
| ISO14443-4 layer | `iso14443_4_layer.c` | Port framing logic if RFAL-less transceive needs it |
| Registry pattern | `nfc_poller_defs.c` | Mirror table-with-NULL layout |

**Licensing note:** Flipper is GPL-3.0. User has waived concerns for development reuse. For proprietary ship: keep protocol-facts-only discipline; poller **logic** should be reimplemented, not pasted. Document `// ref: flipperzero/lib/nfc/...` on non-trivial ports.

---

## Integration topology

```
                    nRF54L15 (application MCU)
+--------------------------------------------------+
|  NFCT (on-die)          PN7160 (companion)       |
|  Phase 3 — deferred     Phase 0/1/2 — NOW        |
|  default product emu    reader + prove emulate   |
+--------------------------------------------------+
         ^                           ^
         | listen (Phase 3)          | poll + listen (Phase 0–2)
    phone/reader               physical card / self-verify
```

**Recommended wiring:** PN7160 on **I²C** (NXP eval default, addr 0x28) + `HOST_IRQ` GPIO. SPI is supported by NXP TML if board requires it.

**Coexistence:** NFCT listen and PN7160 poll are independent RF frontends — coordinate at application level (do not poll while NFCT field session needs isolation; BLE+NFC policy remains Wave 4 open item).

**Separate MCU option:** Not required for this product; single nRF + companion PN7160 is the locked topology.

---

## Kconfig / backend symbols

```kconfig
# src/nfc/hal/Kconfig — extend existing choice
config NFC_HAL_BACKEND_PN7160
    bool "NXP PN7160 (NCI reader)"
    depends on I2C || SPI
    select NFC_ROLE_READER

# Reader engine
config NFC_READER
    bool "NFC reader/capture engine"
    depends on NFC_ROLE_READER && NFC_HAL_BACKEND_PN7160
    default y

config NFC_READER_WORKQ_STACK_SIZE
    int "Reader worker stack (bytes)"
    default 2048

config NFC_PN7160_TML_I2C
    bool "PN7160 on I2C (default)"
    default y

config NFC_PN7160_TML_SPI
    bool "PN7160 on SPI"
```

**BUILD_ASSERT block (in `nfc_transport_pn7160.c`):**

```c
BUILD_ASSERT(IS_ENABLED(CONFIG_NFC_ROLE_READER));
BUILD_ASSERT(!IS_ENABLED(CONFIG_NFC_ROLE_CARD)); /* PN7160 backend: reader only */
```

When building **dual-backend** firmware (NFCT card + PN7160 reader), use **two backend objects** with role-based routing in `nfc_stack` / `nfc_reader` — not a single `NFC_HAL_BACKEND` choice. See DECISION-PN7-1 below.

### DECISION summary

| ID | Decision |
|---|---|
| **DECISION-PN7-1** | PN7160 backend advertises `NFC_ROLE_READER` only — no card/listen on PN7160 |
| **DECISION-PN7-2** | Dual-backend builds compile both `nfc_transport_nrfx.c` and `nfc_transport_pn7160.c`; card ops call NRFX, reader ops call PN7160 |
| **DECISION-PN7-3** | NCI transport is worker + IRQ (Model B), events dispatched on shared `nfc_work_q` |
| **DECISION-PN7-4** | `NxpNci_ProcessReaderMode` blocking loops are split into async state machine steps |
| **DECISION-PN7-5** | Verify pass criteria: UID match + NDEF byte-equal + no unexpected SW errors on re-read |

---

## Tests and shell commands

### Shell

| Command | Phase |
|---|---|
| `nfc_transport pn7160 fwver` | 1 |
| `nfc_transport pn7160 stats` | 1 |
| `nfc reader scan` | 2 |
| `nfc reader clone <tag>` | 4 |
| `nfc reader verify <tag>` | 5 |
| `nfc reader stats` | 2 |

### Twister / ztest

| Tag | Suite |
|---|---|
| `writable_ndef.nfc.pn7160` | NCI frame codec (unit) |
| `writable_ndef.nfc.reader` | Flag assignment, verify diff (unit) |
| `writable_ndef.nfc.integration` | DK + PN7160 HIL (manual CI job) |

---

## Sequencing diagram

```
Phase 0 HAL bring-up ──> Phase 1 clone (NDEF first)
                              |
                              v
                    Phase 2 PN7160 emulate + verify
                              |
                              v
                    Phase 3 NFCT port (Waves 1–6)
                              |
                              v
                    Aliro poller (Phase 1 expand) + listener (Phase 3 NFCT)
```

See [`2026-06-13-implementation-phases.md`](../specs/2026-06-13-implementation-phases.md) for gates.

---

## Open items (resolve during Wave 7)

1. **Dual-backend Kconfig shape** — split `NFC_HAL_BACKEND` choice into independent `NFC_HAL_NRFX` + `NFC_HAL_PN7160` booleans vs link-time alias.
2. **I²C vs SPI** on product board — confirm from schematic before DT binding lock.
3. **PN7160 FW version** minimum for NCI 2.0 features used.
4. **Reader stats counters** — extend `nfc_transport_stats_t` or separate `nfc_reader_stats_t` (HAL authoring guide §10).

---

## Sources consulted

1. `hals_temp/NXP-NCI2.0_LPC55S6x_examples/` — NxpNci20, TML, RW examples
2. `docs/superpowers/plans/wave1-hal.md` §A — poller sub-API
3. `docs/NFC_HAL_AUTHORING_GUIDE.md` — Model B worker bridge
4. `docs/superpowers/specs/2026-06-13-locked-architecture-summary.md`
5. `docs/superpowers/plans/wave6-store.md` — `.card` format + flags
6. `flipperzero/lib/nfc/` — poller registry reference
