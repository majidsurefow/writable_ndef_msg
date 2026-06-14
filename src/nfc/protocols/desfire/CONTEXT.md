# protocols/desfire — CONTEXT

**Layer:** L0 (protocol model) · **Lifecycle:** full

## Purpose

MIFARE DESFire protocol: application/file-based data model (PICC info, applications, files), serialize/deserialize, poller (reader partial clone), and ISO-DEP listener (card emulation). **Partially emulatable**—public file contents can be emulated; encrypted auth not replayed.

## Key files

| File | Owns |
|------|------|
| `desfire.c` | Data model: `desfire_t` struct, PICC info, app/file trees, serialize/deserialize |
| `desfire.h` | Public types, model API, capacity symbols |
| `desfire_poller.c` | Reader poller: GetVersion, GetApplicationIDs, SelectApplication, GetFileIDs, ReadData |
| `desfire_poller.h` | Poller API: `desfire_poller_read` |
| `desfire_listener.c` | ISO-DEP listener: responds to DESFire APDU commands |
| `desfire_listener.h` | Listener API: `desfire_listener_load`, `desfire_listener_on_apdu` |

## Kconfig

| Symbol | Default | Effect |
|--------|---------|--------|
| `NFC_PROTOCOL_DESFIRE` | n | Gate model + poller + listener |
| `NFC_DESFIRE_MAX_APPS` | 4 | Max applications in model |
| `NFC_DESFIRE_MAX_FILES_PER_APP` | 8 | Max files per application |
| `NFC_DESFIRE_MAX_FILE_SIZE` | 256 | Max file payload bytes |
| `NFC_DESFIRE_FREE_MEMORY` | 7520 | GetFreeMemory response |

## Deps

- **Upstream (calls):** None (standalone)
- **Downstream (called by):** `nfc_reader` (poller registry), `nfc_router` (AID dispatch), `nfc_store`

## Invariants & safety

- **Buffers:** App/file counts validated against Kconfig limits
- **Error propagation:** Functions return negative errno; file not found → `-ENOENT`

## Roles

- **Poller:** `desfire_poller.c` (Tier B) — reader ISO-DEP
- **Listener:** `desfire_listener.c` (Tier C) — ISO-DEP emulation

## Wire/spec

- NXP MIFARE DESFire EV1/EV2/EV3 datasheet
- **Emulatable:** Yes (partial; public files only, no encrypted auth replay)

## Capacity symbols

| CONFIG_NFC_DESFIRE_* | Default | Bound it protects |
|----------------------|---------|-------------------|
| `MAX_APPS` | 4 | Application array size |
| `MAX_FILES_PER_APP` | 8 | File array per app |
| `MAX_FILE_SIZE` | 256 | File data buffer |

## Fixtures ↔ goldens

- **Flipper:** No dedicated Flipper fixture (reader-captured `.card`)
- **Generated:** Reader-captured card via `nfc read`
- **Loopback:** `tests/common/nfc_virtual_loopback` (poller↔listener)

## Profile membership

- **Reader (`NFC_PROFILE_READER`):** Yes (poller compiled)
- **CE (`NFC_PROFILE_CARD_EMULATION`):** Yes (emulate subset)

## Tests that prove it

| Test | Tier | Profile gate | Proves |
|------|------|--------------|--------|
| `sample.nfc.unit.nfc_desfire.model` | A (model) | `NFC_PROTOCOL_DESFIRE` | Serialize/deserialize, app/file bounds |
| `sample.nfc.unit.nfc_desfire.poller` | B (poller) | `+NFC_DESFIRE_TEST_TIER_POLLER` | Poller command sequence, file read |
| `sample.nfc.unit.nfc_desfire.listener` | C (listener) | `+NFC_DESFIRE_TEST_TIER_LISTENER` | Listener APDU responses |

**Tier counts:** 3 configs, 30 cases (model + poller + listener).
**Twister dir:** `tests/unit/nfc_desfire`

## Live HIL

- **Tag:** MIFARE DESFire EV1/EV2
- **Golden:** Reader-captured `.card`
- **Overlay:** `overlay-pn7160-stack.conf` (reader); `+overlay-pn7160-listen.conf` (emulate)
- **Shell sequence:**
  ```
  nfc read tag1 → nfc stack stop → nfc emulate tag1
  nfc_transport stats  # apdu_assembled > 0
  nfc check tag1  # PASS
  ```
- **PASS criteria:** External reader performs app/file SELECT; emulated responses match stored

## Shell

Pointer: `src/nfc/applets/nfc_applet_shell_cmds.c` under `NFC_APPLETS_SHELL`. DESFire via `nfc read`, `nfc emulate`, `nfc loop`.
