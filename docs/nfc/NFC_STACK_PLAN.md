# NFC Stack — Implementation Plan

**Status:** LOCKED — single linear execution plan (replaces wave sequencing)  
**Date:** 2026-06-14  
**Authority:** Module contracts in [`NFC_STACK_CONVENTIONS.md`](NFC_STACK_CONVENTIONS.md).
Where conventions say `services/`, implement as `protocols/` in code.

**Design:** [`specs/2026-06-13-nfc-final-design.md`](specs/2026-06-13-nfc-final-design.md),
[`specs/2026-06-13-nfct-pn7160-capability-matrix.md`](specs/2026-06-13-nfct-pn7160-capability-matrix.md)  
**Driver:** Frozen @ `21bdd71` —
[`specs/2026-06-14-pn7160-zephyr-driver.md`](specs/2026-06-14-pn7160-zephyr-driver.md)  
**Archived specs:** [`archive/specs/`](archive/specs/README.md) (superseded wave-era docs)

---

## Source tree (`src/nfc/`)

```
hal/           nfc_transport + PN7160/NFCT backends
reader/        nfc_reader_engine — poll, clone, verify
nfc_stack/     card-role orchestrator (listen path)
protocols/     per-card data model + *_poller.c + *_listener.c
applets/       end-to-end workflows: clone, emulate, verify shells
store/         .card envelope + live persist
run/           init/start wiring, profile selection
```

## Threading

One application work queue: `nfc_stack_wq`. PN7160 driver owns `pn7160_wq`
(IRQ → NCI drain). No per-protocol threads.

## Gated commits (5)

| # | Scope | Gate (must pass before next) |
|---|-------|------------------------------|
| 1 | `hal/` PN7160 poll + `reader/` scan | `nfc reader scan` detects tag; driver unit tests green |
| 2 | `store/` minimal + `reader/` clone (NDEF) | `nfc reader clone` produces valid `.card` blob |
| 3 | `protocols/ndef` + framing/router + `nfc_stack` on PN7160 listen | Type-4 CE responds to SELECT / READ BINARY |
| 4 | `applets/` emulate + verify on PN7160 | clone → emulate → verify **PASS** on same chip |
| 5 | NFCT backend + port proven `protocols/` listeners | same `.card` on NFCT emulate; PN7160 verify **PASS** |

## Backlog (per protocol — surgical review locked 2026-06-14)

Add when the gated path above is green. Eleven per-protocol reviews complete. See [`NFC_PROTOCOLS_COOKBOOK.md`](NFC_PROTOCOLS_COOKBOOK.md).

| Protocol | Module? | Poller | Listener | NFCT emulate | When |
|----------|---------|--------|----------|--------------|------|
| **ndef (Type-4)** | `protocols/ndef/` | ✓ Gate 2 (ISO-DEP SELECT/READ) | ✓ Gate 3 (T4 + router) | ✓ Gate 5 (`nfc_t4t_lib` raw) | Gate 2 |
| **mf_ultralight** | `protocols/ultralight/` | ✓ page READ (Flipper port) | skip — via `ndef` T4 adapter | partial (T4 NDEF RW or backlog `nfc_t2t_lib` READ-only) | post–Gate 2 |
| **mf_classic** | `protocols/classic/` | ✓ TAG-CMD + crypto1 | skip | no | post–Gate 4, clone-only |
| **mf_desfire** | `protocols/desfire/` | ✓ Flipper poller SM; partial without keys | partial — auth `desfire_auth_state_t` enum | partial T4 raw PICC | post–Gate 5 |
| **emv** | `protocols/emv/` | PN7160 ISO-DEP transcript; READ_ONLY_PARTIAL | static TLV caches; 2 AIDs | partial raw PICC | post–Gate 5 |
| **aliro** | `protocols/aliro/` | public AUTH transcript; READ_ONLY_PARTIAL | PSA crypto; 2 AIDs; `submit_work` | partial raw PICC + PSA | post–Gate 5 |
| **mf_plus** | — | skip v1 | — | — | ID-only; use ndef/desfire |
| **felica** | `protocols/felica/` | ✓ Flipper poller | skip | no | defer, clone-only |
| **iso15693_3** | `protocols/iso15693_3/` | ✓ raw tag_transceive | skip | no | defer, clone-only |
| **slix** | `protocols/slix/` child of iso15693_3 | ✓ NXP SLIX detect | skip | no | post–Gate 5 optional |
| **st25tb** | `protocols/st25tb/` | ✓ NFC-B | skip | no | low priority |
| **iso14443 (3a/4a/3b/4b)** | no — HAL fold | PN7160 NCI + session transceive | 4a: nrfx T4T + router; 3b/4b clone-only | partial 4a only | implicit Gate 1–3 |

**Product-only modules (no Flipper folder):** `ndef`, `emv`, `aliro` — ISO-DEP lane above transport.

### HAL capability gaps (to close)

- **`tech_mask` ignored in `discover_start`** — PN7160 backend ARG_UNUSED; always default table (NFC-A + NFC-B + 15693).
- **`tag_info` missing ATQA/SAK/ATS/ATQB** — doc/impl mismatch; `pn7160_nci_rf_intf` and `nfc_transport_tag_info_t` expose UID + protocol/interface/mode only.
- ~~**Reader engine stops discovery + clears session after scan**~~ — **closed Step A1** (`688c370`).
- ~~**PN7160 listen recv drops APDUs**~~ — **closed Step A2** (`cfd30b0`); router wiring remains Gate 3.
- **`NFC_TECH_TYPE3_FELICA` missing; NFC-F not in default discovery** — FeliCa poller needs custom discovery table.
- **`NFC_TRANSPORT_MAX_RESPONSE_LEN` = 255 B** — chunk READ BINARY in protocol layer; T4T lib allows 0xFFF0.
- ~~**NFCT `NFC_T4T_EMUMODE_PICC` not explicitly set in nrfx HAL**~~ — **closed Step A3** (`646c5ab`).
- **No `protocols/` tree yet** — greenfield; Gate 2 lands `protocols/ndef/` first.

### NFCT vs PN7160 roles

- **PN7160** — sole **reader**: poll NFC-A/B/F + ISO15693, raw transceive; optional ISO-DEP card emulation (Gate 3+).
- **NFCT (nrfx)** — **listen-only** card emulation via `nfc_t4t_lib` (v1 default); never polls.
- **Product loop:** clone/verify on PN7160 → emulate on NFCT → re-verify on PN7160 (Gates 4–5).
- **Single-flight:** one poll session **or** one listen session per controller; T2T ↔ T4T switch requires stop + re-init.
- **Protocol modules** talk to reader session or `aid_router`, never vendor HAL directly.

## Locked decisions

- **Driver frozen** @ `21bdd71` — no NCI API changes without a spec revision.
- **Source tree:** `hal/`, `reader/`, `nfc_stack/`, `protocols/`, `applets/`, `store/`, `run/` (see above).
- **Naming:** `protocols/` in code, not `services/`; `applets/` in code, not `apps/`.
- **HAL split:** poll (reader) + listen (card) sub-APIs on `nfc_transport`; protocols talk to a session object, not HAL directly.
- **Coupling:** downward = direct calls; upward = registered callbacks; cross-layer wiring only in `reader/` and `nfc_stack/`.
- **Threading:** `pn7160_wq` (driver IRQ → NCI drain) + `nfc_stack_wq` (stack work); **single-flight** — one poll or listen session at a time.
- **Memory:** static buffers and FIXED pools only; [`NFC_STACK_CONVENTIONS.md`](NFC_STACK_CONVENTIONS.md) is binding law.
- **Execution:** five gated commits on greenfield `src/nfc/` (no carry-forward from prototype).
- **Backends:** PN7160 = reader + optional card emulation; NFCT = listen-only; P2P out of scope for v1.
- **PN7160 listen HAL:** poll complete (Gates 1–2); listen path finishes in Gate 3 — see [`NFC_HAL_GUIDE.md`](NFC_HAL_GUIDE.md) §6.
- **Shell:** `pn7160 *` (driver/debug NCI) vs `nfc *` (stack/applets) — separate command trees.

## Before any module work

Read `NFC_STACK_CONVENTIONS.md` §1–12 and [`NFC_PROTOCOLS_COOKBOOK.md`](NFC_PROTOCOLS_COOKBOOK.md)
before any `protocols/` work — lifecycle, coupling, file layout, Gate 2/3 recipes.

## Next steps

1. ~~**[`NFC_HAL_GUIDE.md`](NFC_HAL_GUIDE.md)**~~ — **done** (`52f3136` + nrfxlib NFCT §6 lock).
2. ~~**[`NFC_PROTOCOLS_COOKBOOK.md`](NFC_PROTOCOLS_COOKBOOK.md)**~~ — **done** (`0a81476`).
3. ~~**Gate 1 HAL + reader**~~ — **done** (`b37c559`).
4. ~~**NFCT HAL**~~ — **done** (`428fa52`).
5. ~~**net_buf / `nfc_apdu_pool`**~~ — **done** (`266ef13`).
6. ~~**Step A — HAL unblock**~~ — **done** (`688c370`, `cfd30b0`, `646c5ab`); plan: [`plans/2026-06-14-nfc-sequential-execution.md`](plans/2026-06-14-nfc-sequential-execution.md).
7. **Gate 2** — `protocols/ndef` poller + `reader/` clone (+ save when poller works).
