# nfc_pn7160 — NXP PN7160 Zephyr module

Out-of-tree Zephyr module for the NXP PN7160 NFC controller. Lives in **writable_ndef_msg**
alongside the NFC stack (`src/nfc/`).

**Driver status: frozen v1.0 — READER + CARD.** Reader poll/discovery and card-emulation
listen + raw DATA_PACKET exchange are ported from NXP-NCI2.0; NDEF/T4T protocol handling
stays in the application layer (not in this driver).

**Quality bar:** Phase 0 meets [Zephyr main-tree PR standards](../../docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md#upstream-quality-bar).

## Features (v1.0)

| Component | Status |
|---|---|
| I2C TML (`pn7160_tml_i2c.c`) | NXP framing, mutex, retry |
| SPI TML (`pn7160_tml_spi.c`) | `0x7F` write / `0xFF` read, 10 µs delay |
| VEN reset | 10 ms / 10 ms NXP sequence |
| DWL download mode | `pn7160_dwl_enter()` / `pn7160_dwl_leave()`, 1-byte TML header |
| IRQ → work queue | GPIO ISR → `irq_rx_work` → TML recv + NCI process |
| NCI connect | CORE_RESET probe + CORE_INIT + ConfigureSettings |
| Reader mode | Discovery poll (NFC-A/B/V), `pn7160_nci_reader_tag_cmd()` |
| Card mode | Listen discovery (NFC-A/B), `pn7160_nci_card_mode_recv/send()` |
| Shell | probe, init, settings, discovery, listen, card, tagcmd, xcv, dwl |
| Unit tests | TML framing (NCI + DWL), SPI xfer lengths, NCI FW parse (Twister `ci_unit`) |

Full spec: [`docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md`](../../docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md).

## Devicetree

Reference overlays:

- I2C (default): `boards/overlays/pn7160_i2c.overlay` — `i2c21` @ `0x28`, 100 kHz
- SPI: `boards/overlays/pn7160_spi.overlay` — `spi21` @ CS0, 500 kHz

Both use IRQ P1.10 and VEN P1.11 on nRF54L15 DK + PN7160 eval shield.

Label **`pn7160`** is required for `DT_NODELABEL(pn7160)` / `PN7160_DEVICE`.

## Build

App root `CMakeLists.txt` registers this module via `ZEPHYR_EXTRA_MODULES`.

**I2C (default):**

```bash
west build -b nrf54l15dk/nrf54l15/cpuapp \
  -- -DDTC_OVERLAY_FILE=boards/overlays/pn7160_i2c.overlay \
     -DEXTRA_CONF_FILE=overlay-pn7160.conf
```

**SPI:**

```bash
west build -b nrf54l15dk/nrf54l15/cpuapp \
  -- -DDTC_OVERLAY_FILE=boards/overlays/pn7160_spi.overlay \
     -DEXTRA_CONF_FILE=overlay-pn7160-spi.conf
```

## Shell

With `CONFIG_PN7160_SHELL=y` and UART shell enabled:

```
uart:~$ pn7160 probe
PN7160 present (CORE_RESET OK)

uart:~$ pn7160 init
PN7160 connected (CORE_INIT OK)

uart:~$ pn7160 settings
NCI settings applied (Nfc_settings.h blobs)

uart:~$ pn7160 discovery start
Discovery started (NFC-A/B/V poll)

uart:~$ pn7160 listen start
Listen started (NFC-A/B card emulation)

uart:~$ pn7160 card recv
Card RX (N): ...

uart:~$ pn7160 card send 90 00
Card TX sent (2 bytes)
```

Combined reader + card discovery (NXP RWandCE table) is available via
`pn7160_nci_rw_ce_discovery_start()` in application code.

**Download mode** (requires `dwl-gpios` in devicetree):

```
uart:~$ pn7160 dwl enter
uart:~$ pn7160 dwl status
DWL mode: active
uart:~$ pn7160 dwl leave
```

Binary firmware transfer (`pn7160 fwupdate`) is not yet implemented; enter/leave
switches TML framing for future FWupdate port.

## Public API

See `include/nfc/pn7160.h` (doxygen) and `include/nfc/pn7160_tml.h` for framing helpers.

Typical init flow:

1. `device_is_ready(PN7160_DEVICE)`
2. `pn7160_nci_connect(PN7160_DEVICE)`
3. `pn7160_nci_configure_settings(PN7160_DEVICE)`
4. Reader: `pn7160_nci_discovery_start()` → `pn7160_nci_discovery_wait()` → `pn7160_nci_reader_tag_cmd()`
5. Card: `pn7160_nci_listen_start()` → `pn7160_nci_card_mode_recv()` / `pn7160_nci_card_mode_send()`

Runtime NCI exchange: `pn7160_nci_transceive()`.

## Unit tests

```bash
west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit
```

## NXP port

Ported sections from NXP-NCI2.0 retain NXP file-header copyright:

- `source/TML/tml.c` → `pn7160_tml_i2c.c`, `pn7160_tml_spi.c`
- `NfcLibrary/NxpNci20.c` → `pn7160_nci.c`, `pn7160_nci_parse.c`, `pn7160_nci_discovery.c`, `pn7160_nci_card_mode.c`
- `source/nfc_example_RWandCE.c` → listen discovery table + `pn7160_nci_rw_ce_discovery_start()`

Sources under `hals_temp/NXP-NCI2.0_LPC55S6x_examples/`. T4T NDEF emulation (`T4T_NDEF_emu.c`)
is reference-only — host must implement card protocol above `pn7160_nci_card_mode_*`.

## HAL integration (Phase 0.5+)

Replace interim `nci_tml_zephyr.c` with `DEVICE_DT_GET(DT_NODELABEL(pn7160))` in
`src/nfc/hal/nci/nfc_transport_nci.c`.
