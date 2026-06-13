# NFC reader applet unit tests (Tier E)

**Status:** Scaffold — placeholder Twister entry (`skip: true`)  
**Normative:** [NFC_PROTOCOLS_COOKBOOK.md](../../../docs/nfc/NFC_PROTOCOLS_COOKBOOK.md) §14.6a, §14.11 · [NFC_APPLETS_AND_TESTING.md](../../../docs/nfc/NFC_APPLETS_AND_TESTING.md)

## Scope (Tier E)

QEMU tests for **applet orchestration** — not protocol poller/listener logic (those are Tier A/B/C in `tests/unit/nfc_<proto>/`).

| Planned test | Applet / alias |
|--------------|----------------|
| `test_store_save_envelope` | `nfc read <slot>` / `nfc reader clone` |
| `test_store_load_roundtrip` | `nfc store *` |
| `test_reader_clone_slot` | poller walk → save |
| `test_verify_diff_pass` / `test_verify_diff_fail` | `nfc verify` |

## When to enable

1. Gate 2: `store/` minimal envelope + `nfc read` applet wired.
2. Replace `src/test_placeholder.c` with real tests.
3. Set `skip: false` in `testcase.yaml`.

## Twister (when enabled)

```bash
west twister -T "$REPO/tests/unit/nfc_reader" -p qemu_cortex_m3 --no-sysbuild \
  -t ci_unit -t writable_ndef.nfc.reader -v
```

HIL sign-off remains Tier D (`nfc loop` on PN7160) — not this suite.
