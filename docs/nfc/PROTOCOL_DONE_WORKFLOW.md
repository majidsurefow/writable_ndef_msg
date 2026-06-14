# Protocol DONE Workflow — No Deferrals

**Rule:** A protocol is **SHIPPED** only when every checkbox below is green.  
**No** “deferred scope” sections in CONTEXT.md for shipped protocols.  
**Commits** are titled `feat(nfc/<proto>): ship <name> — Flipper parity` (one protocol per commit series).

Reference tree: `flipperzero/lib/nfc/protocols/<flipper_name>/` (behavior only; cite paths, no GPL paste).

---

## Why things were deferred before (do not repeat)

| Cause | What happened |
|-------|----------------|
| Agent brief scope cuts | P1–P7 briefs had explicit “Deferred scope” to land fast |
| Capability matrix v1 | Classic/FeliCa/SLIX marked clone-only → listener/E+ skipped |
| Test infra gaps | Mocks without CRC/framing; loopback used wrong path (T4 vs T2) |
| RAM on QEMU | `nfc_reader` overflow → protocols dropped in overlays instead of fixing root cause |
| Missing CMake objects | `*_poller_i.c` not linked in aggregate tests |

**New policy:** QEMU + `.card` format + Flipper wire parity is **always** in scope.  
HIL is **additional** sign-off, not a substitute for unit/E+ coverage.

---

## Per-protocol gate (execute in order P1 → P8)

### Phase R — Research (read-only, write findings to CONTEXT.md)

1. List every Flipper file: poller, poller_i, listener, listener_i, model `.c/.h`
2. List every local Flipper `.nfc` under `tests/fixtures/nfc/flipper/`
3. Diff: Flipper poller states/commands vs our poller SM
4. Diff: Flipper listener vs ours (or document HAL gap if product cannot RF-emulate)
5. Update parity matrix row in `docs/nfc/plans/NFC_HARMONIZATION_FINDINGS.md`

### Phase F — Fixtures (card files visible in tree)

1. Every `.nfc` → `tests/fixtures/<proto>/` with:
   - `*_model.inc` (binary model)
   - `*_read_steps.inc` / `*_tx.inc` (wire goldens from Flipper poller TX)
   - `*.card.bin` via `scripts/nfc/protocol_to_card_bin.py` or `flipper_nfc_to_fixture.py`
2. Row in `tests/fixtures/nfc/flipper/README.md` — **SHIPPED**, not “pending”

### Phase W — Wire (Tier B)

1. Port missing commands to `*_poller_i.c` (CRC, framing, auth)
2. Tier B tests: **TX assert on every poller transceive** (or documented count with `NFC_SESSION_MOCK_MAX_TX` raised)
3. Mock RX matches Flipper block/page walk (not shortcut sysinfo-only paths)

### Phase M — Model (Tier A)

1. Serialize format version matches Flipper
2. Golden roundtrip all fixtures

### Phase L — Listener (Tier C) — required when Flipper has listener

| Product category | Requirement |
|------------------|-------------|
| **Emulatable** (NDEF, UL, DESFire, EMV, Aliro) | Port listener + Tier C unit tests + E+ loopback in `nfc_reader` |
| **Clone-only** (Classic, FeliCa, SLIX, ISO15693) | Port listener for **virtual loopback test tag** (QEMU proof); product emulate may still `-ENOTSUP` on NFCT with doc note |

### Phase E — Store & reader aggregate

1. `test_store_<proto>_roundtrip.c` — golden `.card.bin` load/save
2. `nfc_reader` CMakeLists includes all `*_poller_i.c`, CRC helpers, listener if loopback
3. E+ loopback with **deep compare** (not envelope-only)

### Phase V — Verify

```bash
west twister -T tests/unit/nfc_<proto> -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v
west twister -T tests/unit/nfc_reader -p qemu_cortex_m3 --no-sysbuild -v
```

All cases pass. Re-run profile builds if CMake/Kconfig touched.

### Phase D — Done docs

1. `src/nfc/protocols/<proto>/CONTEXT.md` — remove Deferred section; set `Status: SHIPPED`
2. Update `docs/nfc/PROTOCOL_SHIP_STATUS.md`
3. Commit; no WIP flags

---

## Ship order (sequential — do not start Pn+1 until Pn is SHIPPED)

| P | Protocol | Flipper dir | Emulate |
|---|----------|-------------|---------|
| P1 | Ultralight / NTAG | `mf_ultralight/` | T4 adapter (+ native T2 listener tests) |
| P2 | FeliCa | `felica/` | Virtual listener + poller |
| P3 | ISO15693 + SLIX | `iso15693_3/`, `slix/` | Virtual listener |
| P4a | EMV | `emv/` | Listener + deep loopback compare |
| P4b | Aliro | N/A (wave5) | Listener AUTH chain + EXCHANGE |
| P5 | DESFire | `mf_desfire/` | Listener file types |
| P6 | Classic | `mf_classic/` | Virtual listener + Crypto1 loopback |
| P8 | NDEF T4 | (NXP T4T ref) | Already reference — lock goldens |

P7 (NDEF docs split) merges into P8.

---

## DONE definition (program level)

All rows in `PROTOCOL_SHIP_STATUS.md` = **SHIPPED**; twister 475 ci_unit cases green (12 dirs) + ci_build integration + pn7160 tag + nrf54l15 profile matrix; every Flipper `.nfc` in tree has `.card.bin`; no `Deferred` sections in protocol CONTEXT files.
