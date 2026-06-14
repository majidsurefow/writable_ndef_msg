# NFC Shell & Applet Command Reference

**Branch:** `nfc-stack` · **Audience:** bench operators, integrators, HIL runners.
**Authority:** source of truth is the registered shell tables in
`src/nfc/**/​*_shell_cmds.c` and `modules/nfc_pn7160/src/pn7160_shell.c`.
This guide is generated from those files — if a command here does not match the
build, the source wins.

This document inventories **every** NFC shell command in the tree and splits them
into two groups:

- **Product applets** (`nfc scan|read|emulate|verify|loop`) — the high-level
  read/clone/emulate/verify workflow a product or HIL operator uses.
- **Debug primitives** (`nfc reader`, `nfc stack`, `nfc store`, `nfc_transport`,
  `pn7160`) — lower-level DK / bring-up commands that expose one engine, store,
  HAL, or controller operation each.

---

## 1. Quick map — which commands exist in which build

Every shell surface is gated by a `*_SHELL` Kconfig that `depends on SHELL`. When
`CONFIG_SHELL=n` (or the feature is off), the whole command group disappears.

| Command root | Source file | Kconfig gate | Present in profile |
|--------------|-------------|--------------|--------------------|
| `nfc scan/read/verify` | `applets/nfc_applet_shell_cmds.c` | `NFC_APPLETS_SHELL` | Reader, CE |
| `nfc emulate/loop` | `applets/nfc_applet_shell_cmds.c` | `NFC_APPLETS_SHELL` **+** `NFC_LISTEN_STACK` | CE (or reader+listen overlay) |
| `nfc reader scan/detect/clone` | `reader/nfc_reader_shell_cmds.c` | `NFC_READER_SHELL` | Reader, CE |
| `nfc stack init/start/stop/load` | `nfc_stack/nfc_stack_shell_cmds.c` | `NFC_LISTEN_STACK_SHELL` | CE (or reader+listen overlay) |
| `nfc store list/dump/import/save/load` | `store/nfc_store_shell_cmds.c` | `NFC_STORE_RAM_SHELL` | Reader, CE |
| `nfc_transport config/stats/state` | `hal/nfc_transport_shell_cmds.c` | `NFC_HAL_SHELL` | all NFC builds |
| `pn7160 …` | `modules/nfc_pn7160/src/pn7160_shell.c` | `PN7160_SHELL` | PN7160 builds (reader, CE-PN7160, lab) |

The top-level `nfc` command is registered by `nfc_reader_shell_cmds.c` when
`NFC_READER_SHELL` is set; on a **listen-only** build (no reader engine) it is
registered by `nfc_stack_shell_cmds.c` instead and only exposes `nfc stack`.

`nfc_transport` and `pn7160` are **separate top-level commands**, not subcommands
of `nfc`.

---

## 2. Product applets — `nfc <verb>`

These are the commands a product / operator drives. They wrap the headless
engine + store + listen APIs and print human-readable results. Each prints
`Usage:` on bad args and returns a negative errno on failure.

### 2.1 `nfc scan [timeout_ms]`

- **What it does:** starts an async discovery (`nfc_applet_scan_start`), waits
  (`nfc_applet_scan_wait`), then renders the active session (UID, NCI protocol,
  interface, mode/tech) and reports the session is held open for a follow-on
  `nfc reader clone` / `nfc read`.
- **Requires:** `NFC_APPLETS_SHELL`, reader engine (`NFC_READER`), an HAL with a
  reader role. Default timeout = `CONFIG_NFC_READER_SCAN_TIMEOUT_MS` (10000).
- **Returns / errors:** `-EBUSY` (scan already running), `-ETIMEDOUT`, `-ENOENT`
  (no tag detected).
- **Example:**
  ```
  uart:~$ nfc scan
  UID (7): 04 a2 24 ...
  Protocol: 0x.. Interface: 0x.. Tech: 0x..
  Scan complete — session active for nfc reader clone
  ```

### 2.2 `nfc read <slot> [timeout_ms]`

- **What it does:** one-shot scan + clone of the active tag into a named store
  slot (`nfc_applet_read_start` → `nfc_applet_read_wait`), then prints the slot's
  protocol via `nfc_store_peek_entry_meta`. On reader-only builds without RAM
  store it registers the shell hex-dump save callback (`@@NFCDUMP@@`).
- **Alias (LOCKED):** `nfc reader clone <slot>` is identical to `nfc read <slot>`.
- **Requires:** `NFC_APPLETS_SHELL`, `NFC_STORE`. RAM persistence needs
  `NFC_STORE_RAM`.
- **Example:** `nfc read tag1` → `Read complete for slot "tag1"` + `Protocol: …`.

### 2.3 `nfc emulate <slot> [profile]`  ·  `nfc emulate golden <name>`

- **What it does:** loads a stored slot into the listeners and starts card
  emulation (`nfc_applet_emulate`). With `golden <name>` it looks up a compiled-in
  golden blob (`nfc_store_golden_lookup`) and imports it first
  (`nfc_store_ram_import`). Enforces the clone-only policy via
  `nfc_applet_check_emulate` — clone-only cards (e.g. Classic, FeliCa, SLIX,
  ISO15693) are refused with a `clone-only` warning.
- **Profiles:** only `ndef` is currently accepted as the optional profile arg
  (default `NFC_PROFILE_NDEF`).
- **Requires:** `NFC_APPLETS_SHELL` **and** `NFC_LISTEN_STACK`. `golden` also needs
  `NFC_STORE_RAM` + `NFC_STORE_GOLDENS`.
- **Errors:** `-ENOTSUP` (clone-only or emulate unsupported), `-EINVAL` (bad
  profile name).

### 2.4 `nfc verify <slot> [timeout_ms]`

- **What it does:** polls the present tag and diffs it against the stored slot
  (`nfc_applet_verify_start` → `nfc_applet_verify_wait` →
  `nfc_applet_verify_last_result`). Prints `PASS` or `FAIL (n)`.
- **Requires:** `NFC_APPLETS_SHELL`, reader engine, the stored slot.

### 2.5 `nfc loop <slot> [timeout_ms]`

- **What it does:** the orchestrated HIL sign-off path — read → emulate → verify
  on one card (`nfc_applet_loop`). Prints stage progress and a final PASS/FAIL.
- **Requires:** `NFC_APPLETS_SHELL` **and** `NFC_LISTEN_STACK`.
- **Single-flight:** stop the listen stack between the emulate and verify phases;
  RW+CE concurrency on one PN7160 is deferred by design.

---

## 3. Debug / DK primitives

### 3.1 `nfc reader …` (`NFC_READER_SHELL`)

| Command | Action |
|---------|--------|
| `nfc reader scan [timeout_ms]` | Async discovery start (`nfc_reader_scan_start`); watch logs for UID/protocol. Non-blocking, unlike `nfc scan`. |
| `nfc reader detect` | Run the poller registry against the active session (`nfc_reader_pollers_detect`); prints the matched poller name. No save. |
| `nfc reader clone <slot> [timeout_ms]` | Clone active tag to a `.card` blob. Alias of `nfc read` when applets shell is present; otherwise a standalone `nfc_reader_clone_start`. |

### 3.2 `nfc stack …` (`NFC_LISTEN_STACK_SHELL`)

| Command | Action |
|---------|--------|
| `nfc stack init` | Initialize the listen stack (`nfc_stack_init`). |
| `nfc stack start [uid_hex]` | Start listening, optionally with a forced UID (`nfc_stack_start`). |
| `nfc stack stop` | Stop listening (`nfc_stack_stop`). Use this between poll and listen sessions. |
| `nfc stack load <tag>` | Load a `.card` slot into the listeners before `start` (`nfc_stack_load`). |

### 3.3 `nfc store …` (`NFC_STORE_RAM_SHELL`)

| Command | Action |
|---------|--------|
| `nfc store list` | List occupied RAM slots (`cmd_nfc_store_ram_list`, in `nfc_store_ram.c`). |
| `nfc store dump <slot>` | Hex-dump a RAM slot blob (`cmd_nfc_store_ram_dump`). |
| `nfc store import <slot> <hex>` | Import a raw hex blob into a slot (`nfc_store_ram_import`). |
| `nfc store import golden <name>` | Import a compiled-in golden by name (needs `NFC_STORE_GOLDENS`). |
| `nfc store save <slot>` | Serialize the active listeners to a slot (`nfc_stack_save`). Reader-only builds fall back to re-exporting the RAM slot. `-EBUSY` if listen is active. |
| `nfc store load <slot>` | Load a slot into the listeners (`nfc_stack_load`), or peek metadata on a reader-only build. |

### 3.4 `nfc_transport …` (`NFC_HAL_SHELL`, top-level)

| Command | Action |
|---------|--------|
| `nfc_transport config` | Print HAL config (`fwi`). |
| `nfc_transport stats` | Print HAL counters — `discover_start`, `tag_detect`, `transceive`, `field_on/off`, `fragment_rx`, `apdu_assembled`, `frag_drop_buf`, `apdu_oversized`, `apdu_drop_cons`, `response_tx`, `errors`. These are the HIL acceptance counters. |
| `nfc_transport state` | Print HAL state + capabilities (roles bitmask, technologies, tier). |

### 3.5 `pn7160 …` (`PN7160_SHELL`, top-level)

Raw NXP PN7160 controller access for bring-up and protocol probing.

| Command | Action |
|---------|--------|
| `pn7160 probe` | CORE_RESET presence probe. |
| `pn7160 fwver` | Read firmware version from CORE_RESET_NTF. |
| `pn7160 reset` | VEN hard reset (10 ms / 10 ms). |
| `pn7160 init` | CORE_RESET probe + CORE_INIT connect. |
| `pn7160 settings` | Apply `Nfc_settings.h` RF/core config blobs. |
| `pn7160 discovery start\|stop\|status` | RF discovery (reader/poll mode); `status` shows last detected UID/protocol/interface/mode-tech. |
| `pn7160 listen start\|stop\|status` | Card-emulation listen mode (ConfigureMode CARDEMU). |
| `pn7160 card recv\|send <hex…>` | Card-mode DATA_PACKET receive/send (after `listen start`). |
| `pn7160 tagcmd <hex…>` | Raw tag DATA_PACKET (`ReaderTagCmd`); use after discovery. |
| `pn7160 xcv <hex…>` | Raw NCI transceive. |
| `pn7160 dwl enter\|leave\|status` | Firmware-download mode via DWL GPIO (TML framing). |

---

## 4. Typical bench workflows

**Read / clone a tag (reader build, `overlay-pn7160-stack.conf`):**
```
nfc scan
nfc read tag1
nfc store list
nfc store load tag1
```

**Emulate from a golden (CE build, `overlay-nfct-stack.conf` or reader+listen):**
```
nfc emulate golden ndef_url
nfc_transport stats        # apdu_assembled_count > 0, apdu_dropped_no_consumer == 0
```

**Full applet loop (single PN7160, listen overlay layered):**
```
nfc read tag1
nfc stack stop             # single-flight: drop listen before re-polling
nfc loop tag1              # read → emulate → verify, single PASS line
```

**Controller bring-up (any PN7160 build):**
```
pn7160 probe
pn7160 fwver
pn7160 init
pn7160 settings
pn7160 discovery start
pn7160 discovery status
```

---

## 5. Notes & invariants

- **Shell never lives below Layer 2.** Every command here is in a
  `*_shell_cmds.c` (or the PN7160 module shell). The shell→applet decoupling work
  (harmonization Part C / consolidated execution Phases C1–C2) moves the last
  rendering bodies (`nfc_applet_scan_print`, `nfc_applet_loop`) out of the
  service files into the adapter so a `CONFIG_SHELL=n` reader build links with
  zero shell symbols below L2.
- **Single-flight:** never poll and listen on the same controller concurrently.
  Use `nfc stack stop` (or let `nfc loop` sequence it) between sessions.
- **Clone-only vs emulatable:** `nfc emulate` refuses clone-only cards (Classic,
  FeliCa, SLIX, ISO15693-3) per `nfc_applet_check_emulate`. Emulatable set: NDEF,
  Ultralight, DESFire, EMV, Aliro (`nfc_applet_persist_emulatable`).
- See [`NFC_HIL_PROTOCOL_GUIDE.md`](NFC_HIL_PROTOCOL_GUIDE.md) for the exact flash
  commands, per-protocol PASS criteria, and gate mapping.
