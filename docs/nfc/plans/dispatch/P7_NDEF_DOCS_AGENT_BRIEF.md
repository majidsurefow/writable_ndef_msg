# P7 — NDEF T2/T4 Documentation Split — Agent Dispatch Brief

**Priority:** #7  
**Goal:** Documentation-only boundary between NDEF Type-4 (ISO-DEP poller/listener) and Type-2 (ultralight poll + T4 adapter emulate). **No code changes, no new fixtures, no Flipper imports.**  
**NDEF T4 test infra is reference-complete (87 cases, full TX asserts).**

---

## Research verdict

| Question | Answer |
|----------|--------|
| **Flipper NDEF poller (T4T)?** | **NO** — no `protocols/ndef/` in Flipper |
| **Flipper NDEF support?** | **Parse-only** — `applications/main/nfc/plugins/supported_cards/ndef.c` (T2 UL, Classic MAD, SLIX content) |
| **Flipper NDEF `.nfc`?** | **NO** — NTAG/Ultralight `.nfc` are raw pages; NDEF extracted post-read |
| **Our T4 tests?** | **Reference tier** — `tests/unit/nfc_ndef/` 3 configs, 87 cases, `test_read_tx_sequence` with 6 TX asserts |
| **Our T2 path?** | **Not in `protocols/ndef/`** — ultralight poller + TLV parse; emulate via T4 adapter → `ndef_listener` |
| **P7 work type?** | **Docs maintenance only** |

---

## Exact files to edit (documentation only)

| File | Edit |
|------|------|
| `docs/nfc/NFC_PROTOCOLS_COOKBOOK.md` §5.1 | Rename/clarify **"NDEF Type-4 (ISO-DEP / T4T)"**; add "Not Type-2" banner → pointer to §5.2 |
| `docs/nfc/NFC_PROTOCOLS_COOKBOOK.md` §5.2 | Ultralight page READ, CC page 3, TLV page 4+; cross-link Flipper `NDEF_PROTO_UL` as **content parse ref only** |
| `src/nfc/protocols/ndef/CONTEXT.md` | T4 poller/listener scope; cross-link: "T2 NDEF → ultralight + adapter" |
| `src/nfc/protocols/ultralight/CONTEXT.md` (if exists) | T2 poll path; T4 adapter handoff to `ndef_listener` |
| `docs/nfc/NFC_APPLETS_AND_TESTING.md` | T2/T4 routing table: which suite runs which path |
| `docs/nfc/NFC_STACK_ARCHITECTURE.md` §4 | NDEF row = T4 emulatable; ultralight = via adapter |
| Optional new section | `docs/nfc/NDEF_T2_T4_ROUTING.md` (~1 page) — when PN7160 uses `RW_NDEF_T4T` vs page poller |

### Do NOT edit (reference only)

- `tests/unit/nfc_ndef/**` — already complete unless doc cites wrong test names
- `scripts/nfc/ndef_to_fixture.py` — sole T4 golden generator
- `src/nfc/protocols/ndef/ndef_poller.c`, `ndef_listener.c` — no P7 code changes
- Flipper tree

---

## Wire format / test parity targets (for doc accuracy)

### T4 poller TX sequence (document as canonical — already tested)

From `tests/unit/nfc_ndef/src/test_poller.c` L20–34, L189–201:

| Step | TX | Purpose |
|------|-----|---------|
| 0 | `00 A4 04 00 07 D2 76 00 00 85 01 01 00` | SELECT NDEF v2 AID |
| 1 | `00 A4 00 0C 02 E1 03` | SELECT CC file |
| 2 | `00 B0 00 00 0F` | READ CC (15 B) |
| 3 | `00 A4 00 0C 02 E1 04` | SELECT NDEF file |
| 4 | `00 B0 00 00 02` | READ NLEN |
| 5 | `00 B0 00 02 [Le]` | READ NDEF body |

**6 TX steps** asserted in `test_read_tx_sequence` — use as template reference in docs for other protocols.

### T4 golden variants (document in testing table)

| Variant | Fixture prefix | Loopback test |
|---------|----------------|---------------|
| Empty T4T | `tests/fixtures/ndef/empty.*` | `test_virtual_loopback_empty_card` |
| URI 5 B | `tests/fixtures/ndef/uri_5byte.*` | `test_virtual_loopback_uri_5byte` |
| Chunk NLEN=300 | `tests/fixtures/ndef/chunk_255.*` | `test_virtual_loopback_chunk_255` |

Generator: `scripts/nfc/ndef_to_fixture.py` (NXP `RW_NDEF_T4T` + cookbook §5.1) — **not** `flipper_nfc_to_fixture.py`.

### T2 path (document, do not conflate with T4)

| Phase | Module | Flipper analog |
|-------|--------|----------------|
| Poll/read pages | `src/nfc/protocols/ultralight/` | `mf_ultralight` poller |
| Parse CC/TLV | ultralight extract | `NDEF_PROTO_UL` in `ndef.c` |
| Emulate | ultralight → **T4 adapter** → `ndef_listener` | No native T2 listener in our v1 |
| Tests | `tests/unit/nfc_ultralight/` | Flipper `.nfc` → ultralight fixtures |

---

## Current documentation bugs (file:line areas)

| Issue | Location | Fix in P7 |
|-------|----------|-----------|
| §5.1 title implies generic "NDEF" | `NFC_PROTOCOLS_COOKBOOK.md` ~§5.1 | Clarify T4T only |
| T2 and T4 conflated in architecture row | `NFC_STACK_ARCHITECTURE.md` ~§4 | Split rows / cross-links |
| `ndef/CONTEXT.md` may not state T2 lives elsewhere | `src/nfc/protocols/ndef/CONTEXT.md` | Add routing cross-link |
| Testing doc lacks T2 vs T4 matrix | `NFC_APPLETS_AND_TESTING.md` | Add 1-page table |
| Flipper `ndef.c` cited as wire ref | Various | Reframe as **content parse only** |
| Ultralight F1 adapter Tier C pending | `NFC_APPLETS_AND_TESTING.md` | Document under T2 emulate, not `nfc_ndef` |

**Not bugs:** T4 test coverage — `testcase.yaml` configs `sample.nfc.unit.nfc_ndef.{model,poller,listener}` — **do not rebuild**.

---

## DO NOT touch

- Any `src/**/*.c` or `tests/**/*.c` — **P7 is docs-only**
- `src/nfc/protocols/ultralight/**` product code — P1 may be editing
- Add Flipper NDEF `.nfc` fixtures
- Link `nfc_ndef` tests to Flipper converter
- `ndef_to_fixture.py` logic
- CI / Twister configs (no test changes)

---

## Staged commits (exact message templates)

### Commit 1 — Cookbook split

```
docs/nfc: clarify cookbook §5.1 as NDEF T4T with T2 cross-link to §5.2

Explicit "Not Type-2" banner; remove implied T2 scope from T4 recipe.
```

### Commit 2 — CONTEXT cross-links

```
docs/nfc: split NDEF T4 and T2 paths in protocol CONTEXT files

ndef/CONTEXT.md → ultralight + adapter; ultralight CONTEXT → T4 adapter handoff.
```

### Commit 3 — Testing + architecture tables

```
docs/nfc: add T2/T4 routing table to NFC_APPLETS_AND_TESTING

Update NFC_STACK_ARCHITECTURE NDEF vs ultralight rows; note Flipper ndef.c parse-only.
```

### Commit 4 — Optional routing page

```
docs/nfc: add NDEF_T2_T4_ROUTING.md one-pager

When poller runs T4 ISO-DEP vs T2 page poller; PN7160 RW_NDEF_T4T note.
```

---

## Twister verify commands

**No test changes expected.** Run smoke to confirm docs-only PR did not accidentally touch code:

```bash
REPO=/Users/majidfaroud/writable_ndef_msg

# Baseline — should be unchanged pre/post P7
west twister -T "$REPO/tests/unit/nfc_ndef" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

west twister -T "$REPO/tests/unit/nfc_ultralight" \
  -p qemu_cortex_m3 --no-sysbuild -t ci_unit -v

west twister -T "$REPO/tests/unit/nfc_reader" \
  -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t sample.nfc.unit.nfc_reader.shell_off -v \
  --testcase-filter "*loopback*ndef*"
```

**Pass criteria:** Same green counts as before P7; doc link spot-check manually.

---

## Caller sweep checklist (doc link integrity)

After editing docs, verify these cross-references resolve and remain accurate:

- [ ] `docs/nfc/NFC_PROTOCOLS_COOKBOOK.md` §5.1 ↔ §5.2 links
- [ ] `src/nfc/protocols/ndef/CONTEXT.md` → ultralight path
- [ ] `docs/nfc/NFC_APPLETS_AND_TESTING.md` → `tests/unit/nfc_ndef/testcase.yaml` variant table
- [ ] `docs/nfc/CI_TESTING.md` — 87 NDEF cases mention still accurate
- [ ] `docs/nfc/NFC_STACK_ARCHITECTURE.md` capability matrix
- [ ] No doc claims Flipper provides T4 NDEF wire goldens
- [ ] No doc routes T2 tests to `nfc_ndef` listener suite

---

## Deferred scope

- Ultralight F1 — Tier C via T4 adapter (code task, not P7)
- Native T2 listener (`nfc_t2t_lib`) — explicitly deferred
- Flipper `NDEF_PROTO_MFC` / `NDEF_PROTO_SLIX` doc appendices (optional later)
- Merging §5.1/§5.2 into single mega-section (avoid — split is the goal)
- Any new NDEF test cases or fixture generation

---

## Reference map (for doc authors)

```
Flipper                              Our stack                         Doc home
─────────────────────────────────────────────────────────────────────────────────
supported_cards/ndef.c (UL/MFC/SLIX) → ultralight poller + adapter   §5.2 / ultralight CONTEXT
(no T4 NDEF poller)                  → ndef_poller/listener (T4)     §5.1 / ndef CONTEXT
iso14443_4a (parent)                 → HAL fold / reader engine      NFC_STACK_ARCHITECTURE
NTAG .nfc raw pages                  → ultralight fixtures           flipper README → ultralight/
                                     → ndef_to_fixture.py (T4 only)  tests/fixtures/ndef/
```

**Template protocol for TX asserts in docs:** cite `tests/unit/nfc_ndef/src/test_poller.c` `test_read_tx_sequence` as the cookbook §14.3 exemplar other protocols (P2, P6) should mirror.
