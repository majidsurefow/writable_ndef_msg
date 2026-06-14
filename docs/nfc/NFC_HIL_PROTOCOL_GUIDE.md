# NFC Hardware-in-the-Loop (HIL) Protocol Guide

**Branch:** `nfc-stack` · **Audience:** an engineer at the bench with real silicon.
**Companion runbook:** [`plans/NFC_HIL_AND_POLISH_PLAN.md`](plans/NFC_HIL_AND_POLISH_PLAN.md) (Gates 2–5).
**Shell reference:** [`NFC_SHELL_APPLETS.md`](NFC_SHELL_APPLETS.md).

QEMU + unit tests prove the **logic** (parsers, store envelope, framing, router,
applet result structs). They cannot prove the **RF + controller** path: real
field on/off, anticollision, transceive timing, NCI activation fields, and the
NFCT ISR. This guide is the bench procedure that closes that gap, per hardware
target and per protocol.

---

## 0. Ultra-easy quick-start (Gate 2, 5 minutes)

You have a PN7160 reader board (nRF54L15DK + PN7160 over I2C) and one NDEF tag.

```bash
export REPO=/Users/majidfaroud/writable_ndef_msg
source "$REPO/scripts/env/ncs-env.sh"

west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf"
west flash
```

Open the serial console, hold the tag on the antenna, and run:

```
uart:~$ pn7160 probe          # PN7160 present (CORE_RESET OK)
uart:~$ nfc scan              # prints UID / protocol / tech
uart:~$ nfc read tag1         # clone into slot "tag1"
uart:~$ nfc store list        # tag1 listed
```

If `nfc scan` prints a UID and `nfc read tag1` reports a non-zero stored length,
Gate 2 reader read/clone is **PASS**. Everything below is the same idea, scaled
to each protocol and hardware role.

---

## 1. Hardware targets

| Role | Hardware | Backend | Overlay | Gates |
|------|----------|---------|---------|-------|
| **PN7160 reader (poll)** | nRF54L15DK + PN7160 shield/click on I2C (`boards/overlays/pn7160_i2c.overlay`) or SPI (`pn7160_spi.overlay`) | `NFC_HAL_BACKEND_PN7160` | `overlay-pn7160-stack.conf` | 2, 4-verify, 5-verify |
| **PN7160 listen (CE)** | same board | PN7160 | `overlay-pn7160-stack.conf;overlay-pn7160-listen.conf` | 3, 4-emulate |
| **NFCT PICC (listen-only)** | nRF54L15DK on-board NFCT antenna (`nfc_t4t_lib`) | `NFC_HAL_BACKEND_NRFX` | `overlay-nfct-stack.conf` | 5-emulate |
| **HAL / lab scaffold** | PN7160 board, no reader engine | PN7160 | `overlay-pn7160-hal.conf` | bring-up only |

**Single-flight rule (binding):** never poll and listen on the same controller
concurrently. RW+CE on one PN7160 is **DEFERRED**. Always `nfc stack stop`
between a poll and a listen session, or cross to a second board.

**Toolchain:** NCS v3.2.4. `export REPO=/Users/majidfaroud/writable_ndef_msg`,
then `source "$REPO/scripts/env/ncs-env.sh"`.

---

## 2. Per-hardware flash commands (copy/paste)

### 2.1 PN7160 reader (Gate 2 image)
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf"
west flash
```
SPI variant: swap `pn7160_i2c.overlay` → `pn7160_spi.overlay` and add
`overlay-pn7160-spi.conf` to `EXTRA_CONF_FILE`.

### 2.2 PN7160 reader + listen (Gate 3/4 image)
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-stack.conf;$REPO/overlay-pn7160-listen.conf"
west flash
```

### 2.3 NFCT PICC emulator (Gate 5 emulator board)
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-nfct-stack.conf"
west flash
```

### 2.4 PN7160 HAL/lab scaffold (controller bring-up only)
```bash
west build -b nrf54l15dk/nrf54l15/cpuapp "$REPO" --sysbuild \
  -DOVERLAY_FILE="$REPO/boards/overlays/pn7160_i2c.overlay" \
  -- -DEXTRA_CONF_FILE="$REPO/overlay-pn7160-hal.conf"
west flash
```

---

## 3. Gate ladder (each gate is a prerequisite for the next)

| Gate | Proves | Image | Boards |
|------|--------|-------|--------|
| **2** | Poll session + poller walk + `.card` store on silicon | §2.1 | 1 (reader) |
| **3** | Listen `on_apdu` → assembler → router → NDEF listener path | §2.2 | 1 (reader+listen) + external reader |
| **4** | Full applet loop (read→emulate→verify), sequential single-flight | §2.2 | 1 |
| **5** | Cross-backend: NFCT emulate ↔ PN7160 verify | §2.1 + §2.3 | 2 |

### Gate 2 — PN7160 reader: read / clone a real tag
```
uart:~$ nfc scan
uart:~$ nfc read tag1
uart:~$ nfc store list
uart:~$ nfc store load tag1
```
**PASS:** `discover_wait` returns a UID (len > 0); `nfc read` produces a `.card`
that round-trips through `nfc store load`; no `-EBUSY`/timeout on a present tag;
session held open across transceives.

### Gate 3 — PN7160 card emulation: SELECT + READ BINARY
```
uart:~$ nfc store load tag1     # or: nfc emulate golden <name>
uart:~$ nfc emulate tag1
```
Drive it with an external reader (phone NFC Tools / second PN7160):
SELECT NDEF Tag App AID → SELECT CC → READ BINARY → SELECT NDEF file → READ BINARY.
**PASS:** `nfc_transport stats` shows `apdu_assembled > 0`, `apdu_drop_cons == 0`;
external reader reads back NDEF matching the stored `.card`; SELECT returns `90 00`.

### Gate 4 — emulate → verify → loop (sequential, RW+CE deferred)
```
uart:~$ nfc read tag1           # poll session
uart:~$ nfc stack stop          # single-flight before re-using the chip
uart:~$ nfc emulate tag1        # listen session
uart:~$ nfc stack stop
uart:~$ nfc verify tag1         # poll session, diff vs stored .card
uart:~$ nfc loop tag1           # orchestrated read→emulate→verify
```
**PASS:** `nfc verify tag1` → PASS; `nfc loop tag1` → single PASS line; no
`-EBUSY` from store while listen STARTED.

### Gate 5 — NFCT emulate + PN7160 cross-verify
Emulator board (§2.3): `nfc store load tag1 ; nfc emulate tag1`.
Reader board (§2.1), held over the emulator antenna: `nfc verify tag1`.
**PASS:** PN7160 `nfc verify tag1` → PASS against the NFCT-emulated card;
`apdu_assembled > 0`, `apdu_drop_cons == 0`; no concurrent poll+listen on either.

---

## 4. Per-protocol HIL matrix

The product ships 9 protocols. They split by **emulatable** (full PICC listener
exists → can run the whole read→emulate→verify loop) vs **clone-only** (poller +
`.card` capture only; no native listener — verify reads back the stored blob, no
emulate). The clone-only set still proves the **read/clone** half of the loop on
silicon.

| Protocol | Class | Tag / golden source | Emulate? | What QEMU proves | What HIL must prove | Gate |
|----------|-------|---------------------|----------|------------------|---------------------|------|
| **NDEF** | emulatable | any NTAG / Type-4 tag; `nfc emulate golden <ndef>` | yes | model + poller + listener decode, store roundtrip | RF poll, real SELECT/READ BINARY, field on/off | 2,3,4,5 |
| **Ultralight** | emulatable | `Ultralight_11/21/C.nfc`, `Ntag213/215/216.nfc` | yes (via NDEF-T4 adapter) | model + poller decode | page read over RF, UID/SAK from `RF_INTF_ACTIVATED_NTF` | 2,4 |
| **DESFire** | emulatable | reader-captured `.card` | yes | model + poller + listener APDU decode | ISO-DEP transceive, app/file SELECT on RF | 2,3,4,5 |
| **EMV** | emulatable | reader-captured `.card` | yes | model + poller + listener decode | PPSE/AID SELECT + READ RECORD on RF | 2,3,4,5 |
| **Aliro** | emulatable | reader-captured `.card` (pubkey+config; no private key) | yes | model + poller + listener; AID router (two AIDs) | ISO-DEP AUTH transcript exchange on RF | 2,3,4,5 |
| **MIFARE Classic** | clone-only | `MfClassic_1K_4b.nfc` (generator golden) | no | model + poller, crypto1, CRC | Classic anticollision + auth + block read over RF | 2 (read), 4-verify |
| **FeliCa** | clone-only | `Felica.nfc` | no | model + poller decode | NFC-F polling + block read (tech_mask gap, deferred) | 2 (read) |
| **ISO15693-3** | clone-only | (SLIX shares model) | no | model + poller decode | NFC-V inventory + block read over RF | 2 (read) |
| **SLIX** | clone-only | `Slix_cap_default/missed/accept_all_pass.nfc` | no | model + poller, CAP variants | NFC-V SLIX read over RF | 2 (read) |

Flipper reference fixtures live in `tests/fixtures/nfc/flipper/`; derived
`*_model.bin` / `*_read.inc` / `.card.bin` artifacts are regenerated via
`scripts/nfc/flipper_nfc_to_fixture.py --all --card-bin`. They are **behavioral
references only** (Flipper is GPL; protocol code is reimplemented).

### 4.1 Per-protocol bench sequence (emulatable, e.g. NDEF/DESFire/EMV/Aliro)
```
# reader board
nfc read <slot>
nfc store list
# reader+listen board (or NFCT for Gate 5)
nfc stack stop
nfc emulate <slot>
# external reader exercises the AID/SELECT path
nfc_transport stats        # apdu_assembled > 0, apdu_drop_cons == 0
# reader board
nfc verify <slot>          # PASS
```

### 4.2 Per-protocol bench sequence (clone-only, e.g. Classic/FeliCa/SLIX/ISO15693)
```
nfc scan                   # confirm UID + tech for the card class
nfc read <slot>            # capture .card
nfc store dump <slot>      # inspect captured blob
# no emulate — verify reads back the stored blob only
```

---

## 5. Acceptance counters (read after every gate)

`nfc_transport stats` exposes the canonical HIL acceptance counters. Capture them
into the gate sign-off:

| Counter | Gate 2 (poll) | Gate 3/5 (listen) |
|---------|---------------|-------------------|
| `discover_start`, `tag_detect`, `transceive` | increment on poll | — |
| `field_on` / `field_off` | per session | per external-reader pass |
| `fragment_rx`, `apdu_assembled` | — | `apdu_assembled > 0` |
| `apdu_drop_cons` (no consumer) | — | **must be 0** |
| `apdu_oversized`, `frag_drop_buf` | should be 0 | should be 0 |
| `errors` (+ last code) | should be 0 | should be 0 |

---

## 6. Session rules (do not skip)

1. **Stop between poll and listen.** `nfc stack stop` (or `nfc loop`, which
   sequences internally) before switching the PN7160 between reader and CE.
2. **No concurrent RW+CE** on one PN7160 — deferred (NXP combined-discovery
   sequencing not implemented).
3. **Store is busy-guarded** while listen is STARTED — `nfc store save` returns
   `-EBUSY`; stop the stack first.
4. **Clone-only cards never emulate** — `nfc emulate` returns the clone-only
   warning; that is expected, not a failure.
5. **Capture logs + stats** per gate; a green QEMU run is not a substitute for a
   captured silicon log.

---

## 7. Known HIL gaps / deferred (not blockers, document at sign-off)

- `set_uid` live rotation + synthetic field on/off on PN7160 listen.
- `tech_mask` honoring in PN7160 `discover_start` (FeliCa/NFC-F discovery).
- `nfc_t2t_lib` native Type-2 listener (T4 adapter covers v1).
- `tag_info` ATQA/SAK/ATS extension (poller-detect fidelity) — spec'd in
  `NFC_HIL_AND_POLISH_PLAN.md` §5 P1-c.
