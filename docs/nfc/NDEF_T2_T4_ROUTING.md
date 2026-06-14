# NDEF Type-2 vs Type-4 routing

**Status:** LOCKED — docs-only boundary (P7)  
**Normative recipes:** [NFC_PROTOCOLS_COOKBOOK.md](NFC_PROTOCOLS_COOKBOOK.md) [§5.1](NFC_PROTOCOLS_COOKBOOK.md#51-ndef-type-4-iso-dep--t4t) (T4T) · [§5.2](NFC_PROTOCOLS_COOKBOOK.md#52-mf_ultralight-type-2-ndef-poll-path) (T2 poll)  
**Testing matrix:** [NFC_APPLETS_AND_TESTING.md](NFC_APPLETS_AND_TESTING.md#ndef-type-2-vs-type-4-routing-locked)

This page answers: **when does the reader use ISO-DEP T4T vs Ultralight page READ**, and **where does emulate land**.

---

## Decision tree

```
Tag in field (NFC-A)
  │
  ├─ ISO-DEP active (SAK / ATS → ISO14443-4)
  │     └─► ndef_poller_detect / ndef_poller_read
  │           SELECT NDEF AID → SELECT CC → READ CC → SELECT NDEF file → READ NLEN → READ body
  │           Module: protocols/ndef/
  │           Tests: tests/unit/nfc_ndef/ (87 cases)
  │
  └─ ISO14443-3A only (Ultralight / NTAG — ATQA 44 00, SAK 00)
        └─► ultralight_poller_detect / ultralight_poller_read
              Page READ (0x30); CC on page 3; NDEF TLV from page 4+
              Module: protocols/ultralight/
              Tests: tests/unit/nfc_ultralight/ (32 cases)
              Emulate: T4 adapter → ndef_listener (no native T2 RF in v1)
```

**Never conflate:** T4 poller scripts live in `nfc_ndef`; T2 page scripts live in `nfc_ultralight`.

---

## Side-by-side

| | **Type-4 (T4T)** | **Type-2 (Ultralight/NTAG)** |
|--|------------------|------------------------------|
| **Wire** | ISO7816 APDU on ISO-DEP | NFC-A `READ` / `FAST_READ` |
| **Detect** | SELECT NDEF AID (`D2 76 00 00 85 01 01`) | ATQA/SAK + READ page 0 |
| **NDEF location** | CC file `E103`, NDEF file `E104` | CC page 3 (`0xE1`), TLV page 4+ |
| **Poll module** | `ndef_poller.c` | `ultralight_poller.c` |
| **Listen module** | `ndef_listener.c` (native T4T) | `ultralight_listener.c` adapter → `ndef_listener.c` |
| **Store persist ID** | `NFC_PERSIST_ID_NDEF` (`0x01`) | `NFC_PERSIST_ID_ULTRALIGHT` (`0x03`) |
| **Golden generator** | `scripts/nfc/ndef_to_fixture.py` | `scripts/nfc/flipper_nfc_to_fixture.py` |
| **Flipper upstream** | **None** (no T4 poller) | `mf_ultralight` poller + `ndef.c` **`NDEF_PROTO_UL`** (parse only) |

---

## PN7160 / NXP `RW_NDEF_T4T`

Product T4 poller behavior matches NXP SW6705 `RW_NDEF_T4T.c` (cookbook §5.1 import table). Use that table — **not** Flipper — for T4 wire goldens.

Canonical 6-step read TX (asserted in `tests/unit/nfc_ndef/src/test_poller.c` `test_read_tx_sequence`):

| Step | TX (hex) | Purpose |
|------|----------|---------|
| 0 | `00 A4 04 00 07 D2 76 00 00 85 01 01 00` | SELECT NDEF v2 AID |
| 1 | `00 A4 00 0C 02 E1 03` | SELECT CC file |
| 2 | `00 B0 00 00 0F` | READ CC (15 B) |
| 3 | `00 A4 00 0C 02 E1 04` | SELECT NDEF file |
| 4 | `00 B0 00 00 02` | READ NLEN |
| 5 | `00 B0 00 02 [Le]` | READ NDEF body |

T2 path has **no** equivalent NXP table — page layout comes from Ultralight/NTAG datasheets and Flipper page dumps (`.nfc` → fixtures).

---

## Emulate path (Ultralight → T4 adapter)

NFCT v1 is ISO-DEP / T4T only (`nfc_t4t_lib`). Native Type-2 commands (`0x30`, `0xA2`, …) are not exposed on the listen path.

1. **Clone:** `ultralight_poller_read` → store blob (`NFC_PERSIST_ID_ULTRALIGHT`).
2. **Load:** `deserialize` pages → adapter extracts NDEF TLV from CC/TLV layout.
3. **Handoff:** `ndef_service_set_content(msg, len)` before `nfc_stack_start()`.
4. **RF listen:** `ndef_listener.c` — same NDEF AID as native T4 tags.

Tier C for Ultralight emulate is owned by **`nfc_ndef` listener** tests after F1 adapter lands — not a separate native T2 listener suite.

---

## Flipper `ndef.c` — parse only

`applications/main/nfc/plugins/supported_cards/ndef.c` implements **`NDEF_PROTO_UL`** (and MFC/SLIX variants) for **message extraction after a card-specific read**. It is **not**:

- a T4 ISO-DEP poller reference,
- a source of T4 `.nfc` wire goldens, or
- linked by CI (`flipper_nfc_to_fixture.py` handles Ultralight **pages**, not T4 APDUs).

---

## Related CONTEXT files

- [`src/nfc/protocols/ndef/CONTEXT.md`](../src/nfc/protocols/ndef/CONTEXT.md) — T4 poller/listener scope
- [`src/nfc/protocols/ultralight/CONTEXT.md`](../src/nfc/protocols/ultralight/CONTEXT.md) — T2 poll + adapter handoff
