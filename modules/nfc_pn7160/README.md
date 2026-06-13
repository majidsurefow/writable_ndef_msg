# nfc_pn7160 — NXP PN7160 Zephyr module

Out-of-tree Zephyr module for the NXP PN7160 NFC controller. Lives in **writable_ndef_msg**
alongside the NFC stack (`src/nfc/`).

**Quality bar:** Phase 0 meets [Zephyr main-tree PR standards](../../docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md#upstream-quality-bar).

## Features (Phase 0)

| Component | Status |
|---|---|
| I2C TML (`pn7160_tml_i2c.c`) | NXP framing, mutex, retry |
| SPI TML (`pn7160_tml_spi.c`) | `0x7F` write / `0xFF` read, 10 µs delay |
| VEN reset | 10 ms / 10 ms NXP sequence |
| DWL download mode | `pn7160_dwl_enter()` / `pn7160_dwl_leave()`, 1-byte TML header |
| IRQ → work queue | GPIO ISR → `irq_rx_work` → TML recv + NCI process |
| NCI probe | `pn7160_nci_check_dev_pres()` / CORE_RESET_NTF FW cache |
| Shell | `pn7160 probe`, `pn7160 fwver`, `pn7160 dwl *` (`CONFIG_PN7160_SHELL`) |
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

uart:~$ pn7160 fwver
FW version: 01.02.03
```

`fwver` runs CORE_RESET automatically if no cached NTF is available.

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
2. `pn7160_nci_check_dev_pres(PN7160_DEVICE)`
3. `pn7160_fw_version_get(PN7160_DEVICE)` for 3-byte version

Runtime NCI exchange: `pn7160_nci_transceive()`.

## Unit tests

```bash
west twister -T modules/nfc_pn7160/tests/unit/pn7160_tml \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit
```

## NXP port

Ported sections from NXP-NCI2.0 retain NXP file-header copyright:

- `source/TML/tml.c` → `pn7160_tml_i2c.c`, `pn7160_tml_spi.c`
- `NfcLibrary/NxpNci20.c` → `pn7160_nci.c`, `pn7160_nci_parse.c`

Sources under `hals_temp/NXP-NCI2.0_LPC55S6x_examples/`.

## HAL integration (Phase 0.5+)

Replace interim `nci_tml_zephyr.c` with `DEVICE_DT_GET(DT_NODELABEL(pn7160))` in
`src/nfc/hal/nci/nfc_transport_nci.c`.
