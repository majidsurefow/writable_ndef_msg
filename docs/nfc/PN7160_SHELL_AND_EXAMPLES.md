# PN7160 Shell Interface & NXP Example Port Map (Phase 1)

**Status:** Design draft — implement after Phase 0 DONE  
**NXP reference:** `hals_temp/NXP-NCI2.0_LPC55S6x_examples/` (SW6705, gitignored; local only)  
**Related:** [`2026-06-14-pn7160-nxp-library-audit.md`](archive/specs/2026-06-14-pn7160-nxp-library-audit.md)

---

## Goals

1. Reproduce **MCUXpresso demo capabilities** via Zephyr shell (no blocking `main()` loops).
2. Keep **driver shell** (`pn7160 *`) separate from **stack shell** (`nfc *`).
3. Map each NXP example app to a command tree for Phase 1 planning.

---

## Layering

| Layer | Shell prefix | Owns |
|---|---|---|
| Driver (Phase 0) | `pn7160` | Reset, probe, FW version, DWL enter/leave, raw NCI hex (debug) |
| NCI / transport (Phase 0.8) | `pn7160 nci` | Connect, settings, discovery start/stop |
| Reader / poller (Phase 1) | `nfc reader` | Tag detect, UID, NDEF read/write, raw tag cmd |
| Card emulation (Phase 2) | `nfc ce` | T4T emulate, NFCEE autonomous NDEF |
| P2P (Phase 3+) | `nfc p2p` | SNEP / NFC-DEP |

Worker thread: all blocking NCI runs on `nfc_work_q` (async refactor of NXP
`NxpNci_ProcessReaderMode` / `ProcessCardMode` loops).

---

## NXP example → shell mapping

| NXP build / source | NXP capability | Zephyr shell (Phase 1 target) | Phase |
|---|---|---|---|
| `RW_Debug` / `nfc_example_RW.c` | Poll NFC-A/B/15693; raw tag ops (T2 block, ISO-DEP PPSE, MIFARE auth, T5T) | `nfc reader scan`, `nfc reader raw`, `nfc reader mifare` | 0.9 / 1 |
| `RWandCE_Debug` / `nfc_example_RWandCE.c` | Poll + LISTEN; NDEF read all types; T4T CE to external reader | `nfc reader ndef read`, `nfc ce t4t start` | 1 / 2 |
| `P2P_Debug` / `nfc_example_P2P.c` | P2P SNEP NDEF exchange | `nfc p2p snep` | 3 |
| `CE_scenario2_Debug` / `CE_scenario2.c` | NFCEE autonomous embedded T4T NDEF | `nfc ce nfcee config`, `nfc ce nfcee write` | 2 |
| `FWupdate_Debug` / `nfc_example_FWupdate.c` | PN7160 FW download (DWL mode) | `pn7160 fwupdate` | post-Phase 0 |

---

## Command tree (Phase 1)

```
pn7160
├── probe              # CORE_RESET presence (Phase 0 ✅)
├── fwver              # FW version from CORE_RESET_NTF (Phase 0 ✅)
├── dwl                # Firmware download mode (Phase 0 ✅)
│   ├── enter          # DWL high + VEN reset (NXP tml_EnterDwlMode)
│   ├── leave          # DWL low + VEN reset (NXP tml_LeaveDwlMode)
│   └── status         # Show download-mode TML framing flag
├── reset              # VEN hard reset
├── irq                # Show irq_pending / last RX stats (debug)
├── nci
│   ├── connect        # CORE_INIT after probe (NxpNci_Connect)
│   ├── settings       # Apply Nfc_settings.h blobs (NxpNci_ConfigureSettings)
│   ├── tx <hex...>    # Raw host transceive (NxpNci_HostTransceive)
│   └── log on|off     # NCI frame trace (replaces NCI_PRINT_BUF)
└── fwupdate           # Binary FW transfer over DWL TML (Phase 2+)

nfc reader
├── scan [-t ms]       # Start discovery, wait for tag, print UID + protocol
│                      # (NxpNci_WaitForDiscoveryNotification + RfInterface)
├── stop               # NxpNci_StopDiscovery
├── ndef
│   ├── read           # RW_NDEF_Read — auto-dispatch T1T–T5T + MIFARE NDEF
│   ├── write <file>   # RW_NDEF_Write (optional; RWandCE demo)
│   └── dump           # Hex dump of last NDEF buffer
├── raw <hex...>       # NxpNci_ReaderTagCmd (MIFARE / proprietary)
├── mifare
│   ├── auth <sec> [key hex]
│   ├── read <blk>
│   └── write <blk> <hex...>   # RW demo PCD_MIFARE_scenario
└── info               # Last tag: protocol, interface, ATQA/SAK/etc.

nfc ce
├── t4t
│   ├── load <file>    # T4T_NDEF_EMU_SetMessage
│   ├── start          # ConfigureMode CARDEMU + ProcessCardMode loop (async)
│   └── stop
└── nfcee
    ├── config         # NFCEE_NDEF_Configuration (CE_scenario2)
    └── write <file>   # NFCEE_NDEF_DH_Write

nfc p2p                    # Phase 3 — defer
├── listen
├── initiate
└── snep push|pull
```

---

## Phase 0 shell (implemented)

```text
uart:~$ pn7160 probe
PN7160 present (CORE_RESET OK)

uart:~$ pn7160 fwver
FW version: 12.01.05

uart:~$ pn7160 dwl enter
uart:~$ pn7160 dwl status
DWL mode: active
uart:~$ pn7160 dwl leave
```

Registered in `modules/nfc_pn7160/src/pn7160_shell.c` when `CONFIG_PN7160_SHELL=y`.

### DWL (download mode)

Port of NXP `tml_EnterDwlMode()` / `tml_LeaveDwlMode()` in `source/TML/tml.c`:

1. Set `dwl_mode` flag in driver data (selects 1-byte TML header, 2-byte read footer).
2. Assert or deassert **DWL_REQ** (`dwl-gpios`, active high).
3. VEN reset: inactive **10 ms**, active **10 ms** (same as `pn7160_reset()`).

Requires `dwl-gpios` in devicetree; returns `-ENOTSUP` when omitted. Full binary
FW transfer (`pn7160 fwupdate`) remains Phase 2+; enter/leave + TML mode switch
is the Phase 0 deliverable.

---

## Phase 1 shell — implementation notes

### `nfc reader scan`

Port from NXP flow in `nfc_example_RW.c` / `RWandCE`:

1. `NxpNci_Connect()` → shell pre-requisite or implicit on first `scan`
2. `NxpNci_ConfigureSettings()`
3. `NxpNci_ConfigureMode(NXPNCI_MODE_RW)`
4. `NxpNci_StartDiscovery(DiscoveryTechnologies, …)`
5. `NxpNci_WaitForDiscoveryNotification()` → print UID, protocol, interface
6. `NxpNci_StopDiscovery()` on timeout or `reader stop`

Default tech table (RW example):

```c
MODE_POLL | TECH_PASSIVE_NFCA,
MODE_POLL | TECH_PASSIVE_NFCB,
MODE_POLL | TECH_PASSIVE_15693
```

RWandCE adds NFC-F and LISTEN entries — expose via `scan --tech` in Phase 1.

### `nfc reader ndef read`

Port `RW_NDEF_Read()` dispatcher:

| Protocol | NXP module |
|---|---|
| T1T | `RW_NDEF_T1T` |
| T2T | `RW_NDEF_T2T` |
| T3T | `RW_NDEF_T3T` |
| T4T / ISO-DEP | `RW_NDEF_T4T` |
| T5T | `RW_NDEF_T5T` |
| MIFARE NDEF | `RW_NDEF_MIFARE` |

Use `ndef_helper.c` record-type printing as reference for human-readable output.

### `nfc ce t4t start`

Port `T4T_NDEF_emu.c` + `NxpNci_ProcessCardMode()`:

* Preload NDEF via `T4T_NDEF_EMU_SetMessage`
* Run CE loop on `nfc_work_q` (not blocking shell thread)
* `nfc ce stop` posts cancel work

### Async contract

| NXP (blocking) | Zephyr |
|---|---|
| `while(1) { NxpNci_ProcessReaderMode(...) }` | SMF or work item on `nfc_work_q` |
| `NxpNci_WaitForReception()` | `pn7160_nci_transceive()` + IRQ sem |
| `Sleep()` | `k_msleep()` / `k_sem_take()` |

Shell commands **submit work** and print results via shell print callback or
log — never block the shell thread for discovery loops.

---

## Source file port map (Phase 1)

| NXP file | Zephyr destination | Notes |
|---|---|---|
| `NxpNci20.c` — Connect, ConfigureSettings, discovery | `modules/nfc_pn7160/src/pn7160_nci.c` + `pn7160_nci_settings.c` | Phase 0.3–0.8 |
| `Nfc_settings.h` | `pn7160_nci_settings.c` | `const` blob arrays |
| `RW_NDEF*.c` | `src/nfc/reader/pn7160_ndef/` (new) | Phase 1 poller |
| `ndef_helper.c` | `src/nfc/shell/ndef_print.c` | Record pretty-print only |
| `T4T_NDEF_emu.c` | `src/nfc/ce/pn7160_t4t_emu.c` | Phase 2 |
| `P2P_NDEF.c` | deferred | Phase 3 |

Do **not** copy `hals_temp/` into git. Reference SW6705 locally; attribute NXP
in ported file headers.

---

## Test plan (shell)

| Command | HIL expectation |
|---|---|
| `pn7160 probe` | CORE_RESET OK on eval board |
| `pn7160 fwver` | Non-zero 3-byte version |
| `pn7160 dwl enter` / `leave` | Status toggles; TML framing switches (HIL with DWL wired) |
| `pn7160 nci connect` | CORE_INIT status 0 |
| `nfc reader scan` | UID printed for Type-A tag |
| `nfc reader ndef read` | NDEF URL/text from NTAG |
| `nfc ce t4t start` | Phone reads emulated tag |

Automated: framing ztests (Phase 0). Shell: manual HIL checklist in
[`PN7160_REVIEW_CHECKLIST.md`](archive/reviews/PN7160_REVIEW_CHECKLIST.md).
