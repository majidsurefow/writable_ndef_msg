# modules/nfc_pn7160 — CONTEXT

**Layer:** L0 (HAL backend) · **Lifecycle:** full Zephyr module

## Purpose

NXP PN7160 NFC controller driver: devicetree-driven device instantiation, TML (Transport Messaging Layer) I2C/SPI framing, NCI command/response/notification handling, and RF discovery/listen APIs. Does NOT own card-emulation APDU routing (delegated to `src/nfc/framing` + `router`).

## Key files

| File | Owns |
|------|------|
| `src/pn7160_driver.c` | Device init, GPIO (VEN/IRQ/DWL), ISR→work queue dispatch |
| `src/pn7160_tml_framing.c` | TML frame length validation (NCI vs DWL mode) |
| `src/pn7160_tml_i2c.c` / `_spi.c` | Bus-specific TML send/recv |
| `src/pn7160_nci.c` | CORE_RESET/INIT, transceive, connect |
| `src/pn7160_nci_settings.c` | Nfc_settings.h blob application |
| `src/pn7160_nci_discovery.c` | RF discovery start/stop/wait (reader poll) |
| `src/pn7160_nci_card_mode.c` | Listen mode recv/send (card emulation) |
| `src/pn7160_nci_parse.c` | NCI response/notification parsing |
| `src/pn7160_shell.c` | Shell adapter (L2 — `PN7160_SHELL`) |
| `include/nfc/pn7160.h` | Public API |
| `include/nfc/pn7160_tml.h` | TML helpers (frame len, header size) |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `PN7160` | `y` if DT enabled | Gate entire module |
| `PN7160_TML_I2C` | auto | Compile I2C TML when DT instance on I2C bus |
| `PN7160_TML_SPI` | auto | Compile SPI TML when DT instance on SPI bus |
| `PN7160_RX_BUF_SIZE` | 258 | Static RX buffer (NCI max 255 + TML header) |
| `PN7160_WORKQ_STACK_SIZE` | 1024 | Module-local work queue stack |
| `PN7160_SHELL` | `y` if `SHELL` | Shell commands (`pn7160 probe/fwver/...`) |

## Deps

- **Upstream (calls):** Zephyr I2C/SPI/GPIO drivers
- **Downstream (called by):** `src/nfc/hal/nfc_transport_pn7160.c` (HAL backend adapter)

## Invariants & safety

- **Buffers:** `rx_buf[CONFIG_PN7160_RX_BUF_SIZE]` is static per device; TML frame length validated before read (`pn7160_tml_frame_len_validate`) — oversize → `-EINVAL`
- **Concurrency:** `bus_mutex` serializes TML I/O; `api_mutex` guards multi-step NCI sequences; `rx_sem` signals IRQ→work completion
- **Session:** `discovery_active` flag tracks RF state; `-EBUSY` patterns exist but not fully enforced in all paths (acceptable for L0 driver)
- **Error propagation:** Functions return negative errno; `last_rx_err` cached for debug

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `modules.nfc_pn7160.unit.pn7160_tml` | A (model) | `PN7160=y` (QEMU emul) | TML frame length validation (9 cases) |

## Shell

L2 adapter: `src/pn7160_shell.c` under `PN7160_SHELL`. Exposes: `pn7160 probe`, `fwver`, `reset`, `init`, `settings`, `discovery start/stop/status`, `listen start/stop/status`, `card recv/send`, `tagcmd`, `xcv`, `dwl enter/leave/status`. No business logic duplicated — each command is a thin wrapper over the public API.
