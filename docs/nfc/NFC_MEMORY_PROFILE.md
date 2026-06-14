# NFC Stack Memory Profile

Memory audit for production NFC stack builds on nRF54L15 (nrf54l15dk/nrf54l15/cpuapp with `--sysbuild`).

## Memory Baseline

| Profile | Overlay(s) | Use Case | RAM | FLASH |
|---------|-----------|----------|-----|-------|
| PN7160 HAL only | `overlay-pn7160-hal.conf` | HAL bring-up baseline | 15,888 B (16 KB) | 86,184 B (86 KB) |
| PN7160 reader | `overlay-pn7160-stack.conf` | Scan/read tags | 54,792 B (55 KB) | 119,244 B (119 KB) |
| PN7160 reader + CE | + `overlay-pn7160-listen.conf` | Full reader + emulate | 57,376 B (57 KB) | 131,032 B (131 KB) |
| NFCT CE only | `overlay-nfct-stack.conf` | nRF native CE | 53,552 B (54 KB) | 101,600 B (102 KB) |

**Board:** nrf54l15dk/nrf54l15/cpuapp  
**Region sizes:** FLASH = 1,404 KB, RAM = 188 KB

## Major RAM Consumers

### PN7160 Reader Profile (54,792 B total)

| Component | Size | % of RAM | Notes |
|-----------|------|----------|-------|
| **Store (total)** | **24,744 B** | **45%** | Dominates RAM |
| → s_slots (RAM store) | 16,480 B | 30% | 4 slots × ~4,120 B each |
| → s_staging_buf | 4,096 B | 7.5% | CONFIG_NFC_STORE_BLOB_SIZE |
| → s_serialize_buf | 4,096 B | 7.5% | CONFIG_NFC_STORE_BLOB_SIZE |
| **Protocol models** | **13,901 B** | **25%** | Listener static data |
| → DESFire s_model | 9,908 B | 18% | Largest single allocation |
| → Ultralight s_model | 1,086 B | 2% | 256 pages × 4 B + metadata |
| → Aliro s_model | 892 B | 1.6% | P256 keys + response buffer |
| → EMV buffers | 784 B | 1.4% | FCI + GPO + records |
| → NDEF s_model | 780 B | 1.4% | 500 B NDEF + response buffer |
| **Thread stacks** | **9,536 B** | **17%** | See thread inventory |
| **PSA crypto** | ~1,750 B | 3% | Key slots + PRNG state |
| **Shell** | ~3,700 B | 7% | UART backend + history |

### Delta: Reader → Reader + CE

Adding listen role (`NFC_ROLE_LISTEN=y`) costs:
- **+2,584 B RAM** (APDU pool, framing, router state)
- **+11,788 B FLASH** (listen stack code)

---

## Work Queue Inventory

| Work Queue | Stack Size | Priority | Location | Purpose |
|------------|-----------|----------|----------|---------|
| `nfc_stack_wq` | 2,048 B | 5 | `run/nfc_stack_workq.c` | HAL poll, reader scan, listen recv |
| `pn7160_work_q` | 1,024 B | (driver default) | `pn7160_driver.c` | PN7160 driver events |
| `k_sys_work_q` | 1,024 B | (system default) | Zephyr kernel | System work items |

**Observation:** The NFC stack uses a dedicated work queue (`nfc_stack_wq`) with a 2 KB stack. This is appropriate for blocking HAL operations (I2C transceive, poll loops). The stack size could potentially be reduced to 1.5 KB if testing confirms no stack overflow during deep protocol exchanges.

---

## Thread Inventory

| Thread | Stack Size | Priority | Purpose |
|--------|-----------|----------|---------|
| `z_main_thread` | 1,024 B | - | Main thread |
| `z_idle_threads` | 320 B | - | Idle thread |
| `shell_uart_thread` | 2,048 B | - | Shell UART backend |
| `z_interrupt_stacks` | 2,048 B | - | ISR stack |
| `nfc_stack_wq` thread | 2,048 B | 5 | NFC work queue |
| `pn7160_work_q` thread | 1,024 B | - | PN7160 driver work queue |

**Total thread/stack overhead:** ~9,536 B

**Optimization:** For headless deployments (no shell), disable `CONFIG_SHELL=n` to save:
- Shell UART stack: 2,048 B
- Shell history: 512 B
- Shell context: ~1,100 B
- **Total savings: ~3,700 B**

---

## Static Buffer Inventory

### Store Buffers

| Buffer | Size | Kconfig | Purpose |
|--------|------|---------|---------|
| `s_staging_buf` | 4,096 B | `NFC_STORE_BLOB_SIZE` | Temporary blob assembly |
| `s_serialize_buf` | 4,096 B | `NFC_STORE_BLOB_SIZE` | Serialization workspace |
| `s_slots` | 16,480 B | 4 slots | In-memory card store |

**Slot breakdown:** Each slot holds one card's serialized blob. Default 4 slots × ~4,120 B = 16,480 B.

### APDU Pool (listen mode only)

| Pool | Count | Size Each | Total | Kconfig |
|------|-------|-----------|-------|---------|
| `nfc_apdu_pool` | 4 | 512 B | 2,048 B | `NFC_APDU_POOL_COUNT`, `NFC_APDU_BUF_SIZE` |

**Note:** APDU pool is only allocated when `NFC_ROLE_LISTEN=y`. Reader-only builds skip this entirely.

### Protocol Response Buffers

| Protocol | Buffer | Size | Notes |
|----------|--------|------|-------|
| NDEF | `s_response_buf` | 255 B | NFC_TRANSPORT_MAX_RESPONSE_LEN |
| DESFire | `s_resp_buf` | 255 B | Response assembly |
| EMV | `s_resp` | 255 B | Response assembly |
| Aliro | `s_resp` | 255 B | Response assembly |
| All | Total | ~1,020 B | 4 × 255 B |

### Protocol Data Models

| Protocol | Structure | Default Size | Kconfig Tunables |
|----------|-----------|--------------|------------------|
| DESFire | `desfire_data_t` | ~9,908 B | MAX_APPS=4, MAX_FILES_PER_APP=8, MAX_FILE_SIZE=256 |
| Ultralight | `ultralight_data_t` | ~1,086 B | MAX_PAGES=256 |
| NDEF | `ndef_data_t` | ~522 B | MAX_SIZE=500 |
| Aliro | `aliro_data_t` | ~584 B | Fixed (P256 keys) |
| EMV | `emv_data_t` | ~212 B | MAX_RECORDS=2, RECORD_SIZE=64 |
| Classic | Service only | 32 B | No data model (poller-only) |
| FeliCa | Service only | 32 B | No data model (poller-only) |
| ISO15693 | Service only | 32 B | No data model (poller-only) |
| SLIX | Service only | 32 B | No data model (poller-only) |

---

## DESFire Memory Deep Dive

The DESFire listener is the largest RAM consumer at **~10 KB**. Breakdown:

```
desfire_data_t (~9,908 B with defaults):
├── hw_version[7] + sw_version[7] + uid[7] + batch[5]    = 26 B
├── master_key[16] + metadata                             = 22 B
└── apps[MAX_APPS=4]:                                     = 9,860 B
    └── desfire_application_t (~2,465 B each):
        ├── app_id[3] + metadata                          = 8 B
        ├── keys[14][16]                                  = 224 B
        ├── file_ids[8] + file_settings[8]                = 232 B
        └── file_data[8][256]                             = 2,048 B
```

**Optimization options:**

| Kconfig | Default | Minimum | RAM Savings |
|---------|---------|---------|-------------|
| `NFC_DESFIRE_MAX_APPS` | 4 | 1 | ~7,400 B |
| `NFC_DESFIRE_MAX_FILES_PER_APP` | 8 | 2 | ~3,000 B |
| `NFC_DESFIRE_MAX_FILE_SIZE` | 256 | 64 | ~6,000 B |

**Minimal DESFire config (1 app, 2 files, 64 B each):** ~700 B vs 9,908 B default.

---

## Optimization Recommendations

### High Impact (>5 KB savings)

1. **Reduce store slot count** (`NFC_STORE_RAM_SLOT_COUNT=1`): Saves ~12 KB if you only need one active card.

2. **Tune DESFire limits**: Set `NFC_DESFIRE_MAX_APPS=1`, `MAX_FILES_PER_APP=2`, `MAX_FILE_SIZE=64` to save ~9 KB.

3. **Reduce store blob size** (`NFC_STORE_BLOB_SIZE=2048`): Saves 4 KB if max blob < 2 KB.

4. **Disable unused protocols**: Each poller-only protocol (Classic, FeliCa, ISO15693, SLIX) costs ~2-4 KB FLASH.

### Medium Impact (1-5 KB savings)

5. **Disable shell** for production: Saves ~3.7 KB RAM, ~15 KB FLASH.

6. **Reduce APDU pool** (`NFC_APDU_POOL_COUNT=2`): Saves 1 KB RAM if chained APDUs are rare.

7. **Reduce work queue stack** (`NFC_STACK_WORKQ_STACK_SIZE=1536`): Saves 512 B if protocol exchanges are shallow.

### Low Impact (<1 KB savings)

8. **Reduce NDEF max size** (`NFC_NDEF_MAX_SIZE=256`): Saves ~250 B.

9. **Reduce Ultralight pages** (`NFC_ULTRALIGHT_MAX_PAGES=64`): Saves ~768 B if only supporting NTAG213.

---

## Preset Configurations

Ready-to-use Kconfig presets in `confs/`:

| Preset | Use Case | RAM | FLASH |
|--------|----------|-----|-------|
| `pn7160-reader.conf` | Read tags only | ~55 KB | ~119 KB |
| `pn7160-ce.conf` | Card emulation only | ~50 KB | ~110 KB |
| `pn7160-reader-ce.conf` | Both reader + CE | ~57 KB | ~131 KB |
| `nfct-ce.conf` | nRF native CE | ~54 KB | ~102 KB |

---

## Summary

**Actual RAM usage** for a full PN7160 reader build: **55 KB** (28% of 188 KB available).

**Major consumers:**
1. Store slots: 45% (~25 KB) — tune `NFC_STORE_RAM_SLOT_COUNT` and `NFC_STORE_BLOB_SIZE`
2. Protocol models: 25% (~14 KB) — DESFire alone is 18%; tune limits or disable if not needed
3. Thread stacks: 17% (~10 KB) — mostly fixed overhead

**Versus expectation:** "Only a work queue or two and maybe a static buffer" — the stack indeed uses 2 work queues (NFC stack + PN7160 driver) with ~3 KB total stack. However, the store and protocol data models consume 70% of RAM. For minimal deployments, aggressively tune the store and DESFire Kconfig options.

---

## Layered Memory Breakdown

This section isolates memory contribution from each architectural layer.

### Layer Model

```
Layer 0: Zephyr base + logging + GPIO/I2C/SPI drivers (baseline)
Layer 1: + HAL (PN7160 module OR NFCT)
Layer 2: + NFC stack core (reader engine, store, work queues)
Layer 3: + Protocols (each protocol's listener/poller)
Layer 4: + Applets (L1 services — verify, compare, policy)
Layer 5: + Shell (L2 commands) — optional
```

### Incremental Layer Build Results

Measured on nrf54l15dk/nrf54l15/cpuapp with `--sysbuild`:

| Layer | Configuration | RAM | FLASH | RAM Δ | FLASH Δ |
|-------|--------------|-----|-------|-------|---------|
| **Layer 1** | PN7160 HAL only (`overlay-pn7160-hal.conf`) | 15,888 B | 86,184 B | baseline | baseline |
| **Layer 2-4** | + Reader engine + Store + Protocols + Applets (`overlay-pn7160-stack.conf`) | 54,792 B | 119,244 B | +38,904 B | +33,060 B |
| **Layer 5** | + Shell (already in L2-4 above) | — | — | ~3,700 B | ~15 KB |
| **+ Listen** | + CE listen role (`overlay-pn7160-listen.conf`) | 57,376 B | 131,032 B | +2,584 B | +11,788 B |

**Observations:**
- HAL-only (Layer 1) provides a minimal 16 KB RAM / 86 KB FLASH baseline for PN7160 bring-up
- The bulk of RAM comes from Layer 2-4: store slots (25 KB) and protocol models (14 KB)
- Shell adds ~3.7 KB RAM and ~15 KB FLASH — disable for headless production builds

---

### HAL Comparison

| HAL | RAM | FLASH | RAM Δ from baseline | FLASH Δ from baseline | Notes |
|-----|-----|-------|---------------------|----------------------|-------|
| **PN7160 module** | 15,888 B | 86,184 B | — | — | I2C/SPI TML, NCI framing, work queue |
| **NFCT (nRF native)** | 53,552 B* | 101,600 B* | — | — | Hardware registers only, nrfxlib T4T |

*NFCT figure includes full CE stack (not HAL-only) since NFCT requires listen stack to function.

**HAL Contribution Breakdown (PN7160):**

| Component | RAM | FLASH | Source |
|-----------|-----|-------|--------|
| PN7160 driver core | ~2 KB | ~8 KB | `pn7160_driver.c`, NCI framing |
| PN7160 work queue | 1,024 B | — | `CONFIG_PN7160_WORKQ_STACK_SIZE` |
| RX buffer | 258 B | — | `CONFIG_PN7160_RX_BUF_SIZE` |
| I2C/SPI TML | ~200 B | ~2 KB | Transport layer |

**NFCT vs PN7160 for CE-only builds:**

| Metric | NFCT CE | PN7160 CE | Difference |
|--------|---------|-----------|------------|
| RAM | 53,552 B | ~50,000 B | ~3.5 KB less with PN7160 |
| FLASH | 101,600 B | ~110,000 B | ~8 KB less with NFCT |
| Hardware | On-die | External I2C/SPI | NFCT = simpler BOM |
| Capabilities | Listen-only | Reader + Listen | PN7160 = more flexible |

---

### Per-Protocol Cost

Measured from `.map` file symbol sizes:

| Protocol | Model RAM | Poller FLASH | Listener FLASH | Notes |
|----------|-----------|--------------|----------------|-------|
| **DESFire** | 9,908 B | ~4 KB | ~6 KB | Largest; heavily tunable |
| **Ultralight** | 1,086 B | ~2 KB | ~3 KB | 256 pages × 4 B |
| **NDEF** | 522 B | ~1.5 KB | ~2 KB | 500 B data + response |
| **Aliro** | 584 B | ~3 KB | ~4 KB | P256 crypto + response |
| **EMV** | 212 B | ~2 KB | ~2.5 KB | FCI/GPO records |
| **Classic** | 32 B | ~3 KB | — | Poller-only; Crypto1 engine |
| **FeliCa** | 32 B | ~2 KB | — | Poller-only |
| **ISO15693** | 32 B | ~1.5 KB | — | Poller-only |
| **SLIX** | 32 B | ~1 KB | — | Poller-only; extends ISO15693 |

**Total protocol model RAM:** ~12,408 B (22.6% of reader build)

**Response buffers:** Each listener protocol allocates a 255 B `s_resp` buffer for APDU assembly.

---

### Kconfig Impact Table

Measured deltas from full reader build (54,792 B RAM, 119,244 B FLASH):

| Kconfig | Default | Change to | RAM Δ | FLASH Δ | Build Variant |
|---------|---------|-----------|-------|---------|---------------|
| `NFC_STORE_RAM_SLOT_COUNT` | 4 | 1 | **−12,360 B** | ~0 | `pn7160-stack-minstore.conf` |
| `NFC_STORE_BLOB_SIZE` | 4096 | 2048 | **−6,144 B** | ~0 | Combined with above |
| `NFC_DESFIRE_MAX_APPS` | 4 | 1 | **−7,395 B** | ~0 | `pn7160-stack-mindesfire.conf` |
| `NFC_DESFIRE_MAX_FILES_PER_APP` | 8 | 2 | **−2,061 B** | ~0 | Combined with above |
| `CONFIG_SHELL` | y | n | **−45,296 B** | **−71,808 B** | `pn7160-stack-noshell.conf` |
| `NFC_ROLE_LISTEN` | n | y | +2,584 B | +11,788 B | `overlay-pn7160-listen.conf` |
| `NFC_ULTRALIGHT_MAX_PAGES` | 256 | 64 | ~−768 B | ~0 | Reduces page array |
| `NFC_NDEF_MAX_SIZE` | 500 | 256 | ~−244 B | ~0 | Reduces NDEF buffer |

**Build measurements:**

| Configuration | RAM | FLASH | Notes |
|--------------|-----|-------|-------|
| Full stack (default) | 54,792 B | 119,244 B | Baseline |
| Minimal store (1 slot, 2KB blob) | 36,288 B | 119,084 B | −18,504 B RAM |
| Minimal DESFire (1 app, 2 files, 64B) | 45,336 B | 119,212 B | −9,456 B RAM |
| No shell | 9,496 B | 47,436 B | −45,296 B RAM, −71,808 B FLASH |

---

### Layer Configuration Files

Layered build configurations in `confs/`:

| File | Purpose | Use Case |
|------|---------|----------|
| `layer0-baseline.conf` | Zephyr base only | Firmware baseline measurement |
| `layer1-pn7160-hal.conf` | PN7160 driver only | HAL bring-up |
| `layer1-nfct-hal.conf` | NFCT T4T library only | nRF native HAL baseline |
| `layer2-core-minimal.conf` | NFC core + minimal store | Reader engine isolation |
| `layer3-protocols.conf` | + All protocols | Protocol cost measurement |
| `layer4-applets.conf` | + Applets | Applet layer cost |
| `layer5-shell.conf` | + Shell commands | Full interactive build |
| `pn7160-stack-noshell.conf` | Stack without shell | Headless production |
| `pn7160-stack-minstore.conf` | Minimal store config | RAM-constrained builds |
| `pn7160-stack-mindesfire.conf` | Minimal DESFire config | DESFire optimization |

**Build command for any layer:**
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp --sysbuild \
  -- -DEXTRA_CONF_FILE=confs/<layer>.conf \
     -DDTC_OVERLAY_FILE=boards/overlays/pn7160_i2c.overlay
```

---

### Minimal Build Recipe

For absolute minimum RAM footprint with reader functionality:

```conf
# confs/minimal-reader.conf
CONFIG_I2C=y
CONFIG_GPIO=y
CONFIG_EMUL=y
CONFIG_PN7160=y
CONFIG_NFC_STACK=y
CONFIG_NFC_HAL_BACKEND_PN7160=y
CONFIG_NFC_PROFILE_READER=y
CONFIG_NFC_ROLE_LISTEN=n
CONFIG_NFC_STORE_RAM_SLOT_COUNT=1
CONFIG_NFC_STORE_BLOB_SIZE=2048
CONFIG_NFC_DESFIRE_MAX_APPS=1
CONFIG_NFC_DESFIRE_MAX_FILES_PER_APP=2
CONFIG_NFC_DESFIRE_MAX_FILE_SIZE=64
CONFIG_NFC_ULTRALIGHT_MAX_PAGES=64
CONFIG_NFC_NDEF_MAX_SIZE=256
CONFIG_SHELL=n
```

**Expected:** ~20 KB RAM, ~50 KB FLASH (vs 55 KB / 119 KB default)
