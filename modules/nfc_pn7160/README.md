# nfc_pn7160 — NXP PN7160 Zephyr module

Out-of-tree Zephyr module for the NXP PN7160 NFC controller. Lives in **writable_ndef_msg**
alongside the NFC stack (`src/nfc/`).

**Quality bar:** Phase 0 is written to [Zephyr main-tree PR standards](../../docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md#upstream-quality-bar) — out-of-tree location only; not a lower code standard.

## Locked decisions

| ID | Decision |
|---|---|
| **DECISION-DRV-1** | Out-of-tree module at `modules/nfc_pn7160/` (this directory) |
| **DECISION-DRV-2** | Dual transport — I2C (primary/default) **and** SPI (fully supported); one instance uses **either** bus via devicetree, not both |

Reference overlays:

- I2C (default): `boards/overlays/pn7160_i2c.overlay`
- SPI: `boards/overlays/pn7160_spi.overlay`

Full spec: `docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md`.

> Implementation beyond the Phase 0 scaffold is paused until the user approves the full driver spec.

## Layout

```
modules/nfc_pn7160/
├── zephyr/module.yml
├── zephyr/Kconfig
├── dts/bindings/nfc/
│   ├── nxp,pn7160-common.yaml
│   ├── nxp,pn7160-i2c.yaml
│   └── nxp,pn7160-spi.yaml
├── include/nfc/pn7160.h
├── src/pn7160_driver.c
├── src/pn7160_tml_i2c.c
├── src/pn7160_tml_spi.c
└── src/pn7160_nci.c
```

Transport is selected by devicetree (`DT_INST_ON_BUS`); Kconfig `CONFIG_PN7160_TML_I2C` /
`CONFIG_PN7160_TML_SPI` are auto-derived from enabled instances.

HAL integration: `src/nfc/hal/nfc_transport_pn7160.c` (Phase 0.5).

## Wiring

| Transport | Overlay | Notes |
|---|---|---|
| I2C (default) | `boards/overlays/pn7160_i2c.overlay` | `i2c21` @ `0x28`, 100 kHz |
| SPI | `boards/overlays/pn7160_spi.overlay` | `spi21` @ CS0, 500 kHz |

Both use IRQ P1.10 and VEN P1.11 on nRF54L15 DK + PN7160 eval shield (adjust for your board).

The app root `CMakeLists.txt` registers this module via `ZEPHYR_EXTRA_MODULES`.

## Build (Phase 0 scaffold)

From the **writable_ndef_msg** repo root (NCS v3.2.4 environment):

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
     -DEXTRA_CONF_FILE=overlay-pn7160.conf
```

With the full NFC stack enabled (Phase 0.5+), add `-DCONFIG_NFC_STACK=y`.

## NXP port

Ported sections from NXP-NCI2.0 retain NXP file-header copyright. SPI TML follows
`source/TML/tml.c` (`BOARD_NXPNCI_INTERFACE_SPI` path). See
`docs/nfc/specs/2026-06-14-pn7160-zephyr-driver.md`.
