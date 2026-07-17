# Portable NFC Stack — Flipper Zero → Zephyr RTOS

> A portable, backend-neutral NFC stack built on Zephyr RTOS that cleanly ports
> the Flipper Zero NFC functionality (reader, writer, card emulation, applets)
> to Nordic nRF54 hardware — with extensibility to any controller via a
> vendor-clean HAL.

---

## What This Is

Started as Nordic's `writable_ndef_msg` (Type 4 Tag emulation sample), evolved
into a **production-grade embedded NFC stack** that mirrors the Flipper Zero
firmware's NFC subsystem — scan, read, emulate, check, loop — on bare-metal
Zephyr, with clean layering and zero hardware coupling above the transport HAL.

| | Flipper Zero | This Stack |
|---|---|---|
| **OS** | FreeRTOS + Furi | Zephyr RTOS |
| **NFC Controller** | ST25R3916 (RFAL) | PN7160 (NCI) / NFCT (nrfxlib) |
| **Reader** | poller registry → detect/clone | `nfc_transport.discover` → poller registry |
| **Listen/Card Emulation** | T4T + custom AID routing | ISO-DEP AID router → protocol services |
| **Storage** | `.nfc` files | `.card` envelope (TLV + CRC16) |
| **Shell** | CLI app | Zephyr shell (`nfc scan/read/emulate/loop/check`) |
| **Applets** | scan, read, emulate, write | scan, read, emulate, loop, check |

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  L2  Shell Adapters                                      │
│      nfc_{transport,store,reader,applet,stack}_shell_cmds│
│      — Zephyr shell commands; no business logic          │
├─────────────────────────────────────────────────────────┤
│  L1  Applets  (src/nfc/applets/)                        │
│      scan  read  emulate  loop  check  policy           │
│      — headless orchestration; no shell coupling         │
├─────────────────────────────────────────────────────────┤
│  L0  Engines                                              │
│      reader/          nfc_stack/       store/            │
│      — discovery      — listen orch.   — envelope        │
│      — poller reg.    — AID reg.       — save/load       │
├─────────────────────────────────────────────────────────┤
│  L0  Core                                                 │
│      router/          framing/         run/              │
│      — AID dispatch   — APDU parse     — work queue      │
├─────────────────────────────────────────────────────────┤
│  L0  HAL  (src/nfc/hal/)                                 │
│      nfc_transport API   ←   vendor-clean header         │
│      ├─ nfc_transport_pn7160.c  (reader + listen)        │
│      └─ nfc_transport_nrfx.c    (listen-only)            │
├─────────────────────────────────────────────────────────┤
│  Hardware / Driver Layer                                 │
│      modules/nfc_pn7160/   |   nrfxlib NFC T4T           │
│      (PN7160 NCI driver)   |   (Nordic NFCT peripheral)  │
└─────────────────────────────────────────────────────────┘
```

**Key principle:** everything above the HAL (`nfc_transport.h`) is backend-neutral.
Adding a new controller (e.g. ST25R3916 via RFAL, PN532 via I²C) means writing
one backend `.c` file — nothing else changes. See
[docs/NFC_HAL_AUTHORING_GUIDE.md](docs/NFC_HAL_AUTHORING_GUIDE.md).

---

## Supported NFC Protocols (9)

| Protocol | Read | Emulate | Notes |
|---|---|---|---|
| **NDEF** (NFC Forum Type 2/4) | ✓ | ✓ | Default listen profile |
| **MIFARE Ultralight** | ✓ | ✓ | NTAG series |
| **MIFARE Classic** | ✓ | — | 1K/4K, CRYPTO1 auth |
| **FeliCa** (Type 3) | ✓ | — | JIS X 6319-4 |
| **ISO 15693-3** | ✓ | — | Vicinity cards |
| **SLIX** | ✓ | — | NXP ICODE SLIX |
| **DESFire** | ✓ | ✓ | EV1/EV2/EV3 |
| **EMV** | ✓ | ✓ | Payment applet SELECT |
| **Aliro** | ✓ | ✓ | Apple access control |

---

## Build Profiles (Kconfig)

| Profile | Symbol | What's Included |
|---|---|---|
| **Reader** | `CONFIG_NFC_PROFILE_READER` | Reader engine + poller registry + all protocol pollers |
| **Card Emulation** | `CONFIG_NFC_PROFILE_CARD_EMULATION` | Listen stack + AID router + NDEF/DESFire/EMV/Aliro services |
| **Lab** | `CONFIG_NFC_PROFILE_LAB` | Everything: reader + listen + applets + shell |

### Quick overlay matrix

```bash
# Reader-only (PN7160 activation)
west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild \
  -DOVERLAY_FILE=boards/overlays/pn7160_i2c.overlay \
  -- -DEXTRA_CONF_FILE=overlay-pn7160-stack.conf

# Listen-only (NFCT on nRF)
west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild \
  -- -DEXTRA_CONF_FILE=overlay-nfct-stack.conf

# Full stack (PN7160 reader + listen)
west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild \
  -DOVERLAY_FILE=boards/overlays/pn7160_i2c.overlay \
  -- -DEXTRA_CONF_FILE=overlay-pn7160.conf
```

---

## Source Tree

```
src/
├── main.c                     # Entry point (stack-aware or legacy T4T)
├── nfc_emulation.{c,h}        # Legacy T4T convenience wrapper
├── stats.h                    # Spinlock-guarded stats macros
└── nfc/                       # ← The portable NFC stack
    ├── nfc_config.h           # Compile-time backend/role policy
    ├── CMakeLists.txt          # Conditional module inclusion
    ├── Kconfig                 # Profile choice + protocol gates
    ├── run/                    # Shared nfc_stack_wq (work queue)
    ├── hal/                    # nfc_transport API + PN7160/NRFX backends
    ├── framing/                # ISO 7816-4 APDU parse + assembly
    ├── router/                 # ISO-DEP SELECT-by-AID dispatch
    ├── store/                  # .card envelope serialization (TLV+CRC)
    ├── reader/                 # Async discover + poller registry
    ├── nfc_stack/              # Listen stack orchestrator
    ├── protocols/              # 9 protocol implementations
    │   ├── ndef/    ultralight/  classic/
    │   ├── felica/  iso15693_3/  slix/
    │   └── desfire/ emv/         aliro/
    ├── applets/                # L1 headless applets (scan/read/emulate/loop/check)
    └── helpers/                # FeliCa CRC utilities

docs/                           # Design & reference documentation
├── STACK_SPEC.md               # Authoritative stack specification (LOCKED)
├── API_DESIGN_CREED.md         # Module lifecycle, patterns, naming conventions
├── NFC_HAL_AUTHORING_GUIDE.md  # How to write a new nfc_transport backend
├── CALLBACK_REGISTRATION_GUIDE.md  # Canonical callback registration pattern
├── NETWORK_BUFFERS.md          # net_buf ownership rules
├── STATS_API_DESIGN.md         # Unified stats API design
├── CI_TESTING.md               # CI workflows + local testing
└── ZEPHYR_WORKSPACE.md         # West workspace topology (T2)

modules/nfc_pn7160/             # Out-of-tree PN7160 NCI driver module
aliro/                          # Aliro reference (Apple access protocol)
flipperzero/                    # Flipper Zero firmware reference (not built)
hals_temp/                      # Reference HAL implementations
boards/overlays/                # Devicetree overlays (PN7160 I²C/SPI)
confs/                          # Layered Kconfig fragment library
tests/                          # Unit + integration tests
scripts/                        # Build helpers, env scripts, CI support
```

---

## Design Principles

### 1. Vendor-clean HAL

`src/nfc/hal/nfc_transport.h` contains **zero vendor includes, types, or
constants**. Each limit is a transport-owned `#define`; the backend maps it to
the vendor value and `BUILD_ASSERT`s equivalence. This is the locked Gate 1
contract — see [NFC_HAL_AUTHORING_GUIDE.md](docs/NFC_HAL_AUTHORING_GUIDE.md).

### 2. Headless L1 / Shell at L2

Business logic (scan, read, emulate, loop, check) lives at L1 in
`src/nfc/applets/` with **no shell coupling** — no `struct shell *`, no
`shell_print`, no `#include <shell/shell.h>`. Shell adapters at L2
(`*_shell_cmds.c`) translate CLI commands to L1 API calls. The stack runs
headless when `CONFIG_SHELL=n`.

### 3. Module Lifecycle (Init → Start → Stop → Shutdown)

Every module follows the API Design Creed: `init()` → `register_callbacks()` →
`start()` → operational → `stop()` → can restart → `shutdown()` → must re-init.
See [API_DESIGN_CREED.md](docs/API_DESIGN_CREED.md).

### 4. net_buf Ownership: One Owner At a Time

Every `net_buf` has exactly one owner. Transfer is explicit at function-call
boundaries. No ref-sharing without documented reason. See
[NETWORK_BUFFERS.md](docs/NETWORK_BUFFERS.md).

### 5. Unified Stats

Every stats-bearing module uses the `stats.h` spinlock-guarded macro API:
`STATS_INC`, `STATS_ERROR`, `STATS_SCOPE`, `STATS_COPY_OUT`. Every struct
includes `error_count` + `last_error_code`. See
[STATS_API_DESIGN.md](docs/STATS_API_DESIGN.md).

### 6. Callback Registration Canon

Every `register_*_cb(handler, user_ctx)` follows the same guard-state pattern:
`-ENODEV` if uninitialized, `-EBUSY` if started, `-EIO` if error state.
NULL handler = clear registration. See
[CALLBACK_REGISTRATION_GUIDE.md](docs/CALLBACK_REGISTRATION_GUIDE.md).

---

## Flipper Zero to Zephyr Mapping

This stack ports Flipper Zero's NFC subsystem concepts cleanly to Zephyr idioms:

| Flipper Zero Concept | Zephyr Equivalent | Where |
|---|---|---|
| `NfcWorker` (thread) | `nfc_stack_wq` (system work queue) | `src/nfc/run/` |
| `NfcScanner` (poller registry) | `nfc_reader_poller_registry` | `src/nfc/reader/` |
| `NfcProtocol` (listener vtables) | `nfc_service_t` (protocol service) | `src/nfc/router/service.h` |
| `.nfc` file save/load | `.card` envelope (TLV+CRC16) | `src/nfc/store/` |
| `NfcApp` (scene manager) | `nfc_applet_*` (headless L1) | `src/nfc/applets/` |
| `furi_hal_nfc_*` (HAL) | `nfc_transport_*` (HAL) | `src/nfc/hal/` |
| `libnfc` / `lib/st25r3916` | `modules/nfc_pn7160` / vendor driver | `modules/` |
| CLI (`nfc` command) | Zephyr shell (`nfc scan/read/emulate/loop`) | `*_shell_cmds.c` |

---

## Getting Started

### Prerequisites

- **Zephyr SDK** (arm-zephyr-eabi toolchain)
- **west** (Zephyr meta-tool)
- **nRF Connect SDK v3.2.4** (installed via Toolchain Manager or `west update`)
- macOS or Linux; QEMU optional for unit tests

### Bootstrap

```bash
git clone <this-repo>
cd writable_ndef_msg

# T2 topology: this repo is both the manifest and the application
west init -l .
west update

# Set up environment (macOS with Toolchain Manager)
source scripts/env/ncs-env.sh

# Build — Reader (PN7160 on I²C)
west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild \
  -DOVERLAY_FILE=boards/overlays/pn7160_i2c.overlay \
  -- -DEXTRA_CONF_FILE=overlay-pn7160-stack.conf

# Build — Card Emulation (NFCT)
west build -b nrf54l15dk/nrf54l15/cpuapp . --sysbuild \
  -- -DEXTRA_CONF_FILE=overlay-nfct-stack.conf

# Unit tests
west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v
```

See [docs/CI_TESTING.md](docs/CI_TESTING.md) and
[docs/ZEPHYR_WORKSPACE.md](docs/ZEPHYR_WORKSPACE.md) for details.

---

## Shell Commands

```
nfc stack init|start [uid]|stop|load <tag>
nfc scan start|stop
nfc read <slot>
nfc emulate <slot>
nfc emulate golden <name>
nfc loop <slot>
nfc check <slot>              (DK only)
nfc store list|dump|import|save|load
nfc_transport config|stats|state
```

---

## Testing

| Tier | Scope | Example |
|---|---|---|
| **A** | Model / pure unit | `nfc_apdu_asm` fragment fit, oversize drop |
| **C** | Protocol loopback | NDEF/DESFire/EMV/Aliro listen via framing+router |
| **E** | Integration | 97-test reader suite: store roundtrip, verify-compare, loopback |

CI runs via GitHub Actions: Twister builds (integration), Twister unit (QEMU),
compliance, CodeQL, devicetree schema checks, Doxygen coverage, and license scanning.

---

## License

Apache 2.0 (`SPDX-License-Identifier: Apache-2.0`) for stack code.
Nordic 5-Clause for legacy sample files (`LicenseRef-Nordic-5-Clause`).

---

## Further Reading

| Document | What |
|---|---|
| [STACK_SPEC.md](docs/STACK_SPEC.md) | Authoritative layer map, coupling patterns, threading model |
| [API_DESIGN_CREED.md](docs/API_DESIGN_CREED.md) | Lifecycle FSM, required structures, thread-safety annotations |
| [NFC_HAL_AUTHORING_GUIDE.md](docs/NFC_HAL_AUTHORING_GUIDE.md) | How to add a new transport backend |
| [CALLBACK_REGISTRATION_GUIDE.md](docs/CALLBACK_REGISTRATION_GUIDE.md) | Canonical `register_*_cb` pattern |
| [NETWORK_BUFFERS.md](docs/NETWORK_BUFFERS.md) | `net_buf` pool types + ownership semantics |
| [STATS_API_DESIGN.md](docs/STATS_API_DESIGN.md) | Unified stats recipe + naming conventions |
| [CI_TESTING.md](docs/CI_TESTING.md) | CI workflow matrix + local testing setup |
| [ZEPHYR_WORKSPACE.md](docs/ZEPHYR_WORKSPACE.md) | West workspace topology + bootstrap |
