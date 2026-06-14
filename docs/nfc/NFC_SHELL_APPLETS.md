# NFC Shell & Applet Command Reference

**Branch:** `nfc-stack` · **Audience:** bench operators, integrators, HIL runners.
**Applet model: LOCKED 2026-06-14 (Phase A landed).**
**Authority:** source of truth is the registered shell tables in
`src/nfc/**/*_shell_cmds.c` and `modules/nfc_pn7160/src/pn7160_shell.c`.
This guide is generated from those files — if a command here does not match the
build, the source wins.

---

## 0. The layer model (read this first)

The NFC stack is split into four strict layers. The single most important
invariant: **`struct shell *` and `shell_print` live ONLY at Layer 2.**

| Layer | Name | What lives here | `struct shell *`? |
|-------|------|-----------------|-------------------|
| **L0** | Engines | `reader_engine`, `nfc_stack` (listen), `store`, HAL/transport, controller driver | No |
| **L1** | Applets (headless) | Business logic: scan/read/emulate/loop/check/policy. Callbacks, errno, result structs. `nfc_applet_service.h` | **No — forbidden** |
| **L2** | Shell adapters | Thin `*_shell_cmds.c` adapters: parse `argv`, call L1, render result/errno | **Yes — only here** |
| **L3** | App / SMF | Future product consumer of L1 (no shell) | No |

An **applet** is L1 business logic with no I/O surface. A **shell command** is
the L2 adapter that exposes an applet (or an engine primitive) to a human at the
UART. The store, reader engine, listen stack, HAL, and golden import are **not**
applets — they are L0 engines that applets orchestrate.

### Locked applet inventory (L1)

| Applet | Module | One sentence |
|--------|--------|--------------|
| **Scan** | `nfc_applet_scan` | Start/stop continuous reader discovery; invokes a callback on each detected tag |
| **Read** | `nfc_applet_read` | Detect + poller clone of the present tag into a RAM store slot |
| **Emulate** | `nfc_applet_emulate` | Load a slot, enforce policy, start the listen (card-emulation) session |
| **Loop** | `nfc_applet_loop` | Orchestrates read → emulate → check on one card |
| **Check** | `nfc_applet_verify` | Field diff of the present tag vs a stored slot — **internal to Loop; DK shell only** |
| **Policy** | `nfc_applet_policy` | Emulate eligibility (clone-only vs emulatable) — internal helper |

> **Renamed / dropped 2026-06-14:** the product-level `nfc verify` command is
> gone; the same logic is now reached only via `nfc check` (DK) and inside
> `nfc loop`. `nfc reader clone` is gone; use `nfc read`. The old blocking
> `nfc scan` is replaced by `nfc scan start` / `nfc scan stop`.

---

## 1. Quick map — which commands exist in which build

Every shell surface is gated by a `*_SHELL` Kconfig that `depends on SHELL`. When
`CONFIG_SHELL=n` (or the feature is off), the whole command group disappears, and
because shell never lives below L2 the engine/applet objects still link with zero
shell symbols.

| Command root | Source file (L2 adapter) | Kconfig gate | Present in profile |
|--------------|--------------------------|--------------|--------------------|
| `nfc scan start\|stop` | `applets/nfc_applet_shell_cmds.c` | `NFC_APPLETS_SHELL` | Reader, CE |
| `nfc read <slot>` | `applets/nfc_applet_shell_cmds.c` | `NFC_APPLETS_SHELL` | Reader, CE |
| `nfc check <slot>` (DK) | `applets/nfc_applet_shell_cmds.c` | `NFC_APPLETS_SHELL` | Reader, CE |
| `nfc emulate / loop` | `applets/nfc_applet_shell_cmds.c` | `NFC_APPLETS_SHELL` **+** `NFC_LISTEN_STACK` | CE (or reader+listen overlay) |
| `nfc reader scan/detect` | `reader/nfc_reader_shell_cmds.c` | `NFC_READER_SHELL` | Reader, CE |
| `nfc stack init/start/stop/load` | `nfc_stack/nfc_stack_shell_cmds.c` | `NFC_LISTEN_STACK_SHELL` | CE (or reader+listen overlay) |
| `nfc store list/dump/import/save/load` | `store/nfc_store_shell_cmds.c` | `NFC_STORE_RAM_SHELL` | Reader, CE |
| `nfc_transport config/stats/state` | `hal/nfc_transport_shell_cmds.c` | `NFC_HAL_SHELL` | all NFC builds |
| `pn7160 …` | `modules/nfc_pn7160/src/pn7160_shell.c` | `PN7160_SHELL` | PN7160 builds |

The top-level `nfc` command is registered by `nfc_reader_shell_cmds.c` when
`NFC_READER_SHELL` is set; on a **listen-only** build (no reader engine) it is
registered by `nfc_stack_shell_cmds.c` instead and only exposes `nfc stack`.
`nfc_transport` and `pn7160` are **separate top-level commands**, not subcommands
of `nfc`.

---

## 2. Product applets — `nfc <verb>` (L2 surface)

These are the commands a product / operator drives. Each L2 adapter parses
`argv`, calls the headless L1 service (`nfc_applet_service.h`), and renders the
returned result struct / errno. It owns **no** discovery or orchestration logic.
Each prints `Usage:` on bad args and returns a negative errno on failure.

### 2.1 `nfc scan start` · `nfc scan stop`

- **L1:** `nfc_applet_discover_start(cb, ctx)` / `nfc_applet_discover_stop()` /
  `nfc_applet_discover_active()`.
- **What it does:** `start` arms **continuous** reader discovery on the
  `nfc_stack_wq`; for every detected tag the registered
  `nfc_applet_tag_cb_t` fires. The default shell adapter callback prints the UID,
  NCI protocol, interface, and mode/tech for each tag. `stop` ends discovery.
- **Requires:** `NFC_APPLETS_SHELL`, reader engine (`NFC_READER`), an HAL with a
  reader role.
- **Returns / errors:** `start` → `-EBUSY` if already running; `stop` →
  `-EALREADY` if not running.
- **Example:**
  ```
  uart:~$ nfc scan start
  Scan started — discovering tags; run "nfc scan stop" to end
  UID (7): 04 a2 24 ...
  Protocol: 0x.. Interface: 0x.. Tech: 0x..
  uart:~$ nfc scan stop
  Scan stopped
  ```
- **Single-flight note:** stop discovery before `nfc read`/`nfc emulate` — never
  poll and clone/listen concurrently on one controller.

### 2.2 `nfc read <slot> [timeout_ms]`

- **L1:** `nfc_applet_read_start()` → `nfc_applet_read_wait()`, then
  `nfc_applet_get_card_meta()` for the protocol line.
- **What it does:** one-shot scan + clone of the present tag into a named store
  slot, then prints the slot's protocol/persist-id. On reader-only builds without
  a RAM store it registers the shell hex-dump save callback (`@@NFCDUMP@@`).
- **Requires:** `NFC_APPLETS_SHELL`, `NFC_STORE`. RAM persistence needs
  `NFC_STORE_RAM`.
- **Errors:** `-EBUSY` (read in progress), `-ETIMEDOUT`, plus engine errnos.
- **Example:** `nfc read tag1` → `Read complete for slot "tag1"` + `Protocol: …`.

### 2.3 `nfc emulate <slot> [profile]` · `nfc emulate golden <name>`

- **L1:** `nfc_applet_get_card_meta()` + `nfc_applet_check_emulate()` (policy) →
  `nfc_applet_emulate()`. `golden` first does `nfc_store_golden_lookup()` +
  `nfc_store_ram_import()`.
- **What it does:** loads a stored slot into the listeners and starts card
  emulation. The policy applet refuses clone-only cards (Classic, FeliCa, SLIX,
  ISO15693) with a `clone-only` warning.
- **Profiles:** only `ndef` is currently accepted (default `NFC_PROFILE_NDEF`).
- **Requires:** `NFC_APPLETS_SHELL` **and** `NFC_LISTEN_STACK`. `golden` also
  needs `NFC_STORE_RAM` + `NFC_STORE_GOLDENS`.
- **Errors:** `-ENOTSUP` (clone-only / emulate unsupported), `-EINVAL` (bad
  profile name).

### 2.4 `nfc loop <slot> [timeout_ms]`

- **L1:** `nfc_applet_loop_run(slot, timeout, log_fn, ctx, &result)` — fully
  headless; the shell passes an `nfc_applet_log_fn` callback that prints stage
  progress.
- **What it does:** the orchestrated HIL sign-off path — read → emulate → check
  on one card. Prints per-stage lines and a final `PASS` / `FAIL (n)`.
- **Requires:** `NFC_APPLETS_SHELL` **and** `NFC_LISTEN_STACK`.
- **Single-flight:** the applet sequences listen-stack stop between the emulate
  and check phases; RW+CE concurrency on one PN7160 is deferred by design.

### 2.5 `nfc check <slot> [timeout_ms]` — **DK only** (was `nfc verify`)

- **L1:** `nfc_applet_verify_start()` → `nfc_applet_verify_wait()` →
  `nfc_applet_verify_last_result()`.
- **What it does:** polls the present tag and diffs it against the stored slot.
  Prints `PASS` or `FAIL (n)`. This is the DK-only surface for the Check applet —
  in product flows the same logic runs **inside** `nfc loop`.
- **Requires:** `NFC_APPLETS_SHELL`, reader engine, the stored slot.

---

## 3. Debug / DK primitives (L2 over L0 engines)

These expose **one engine / store / HAL / controller operation each**. They are
not applets — they are direct windows into an L0 engine for bring-up.

### 3.1 `nfc reader …` (`NFC_READER_SHELL`)

| Command | Action |
|---------|--------|
| `nfc reader scan [timeout_ms]` | Async discovery start (`nfc_reader_scan_start`); watch logs for UID/protocol. Single-shot, unlike continuous `nfc scan start`. |
| `nfc reader detect` | Run the poller registry against the active session (`nfc_reader_pollers_detect`); prints the matched poller name. No save. |

> `nfc reader clone` was **DROPPED 2026-06-14** — use `nfc read <slot>`.

### 3.2 `nfc stack …` (`NFC_LISTEN_STACK_SHELL`)

| Command | Action |
|---------|--------|
| `nfc stack init` | Initialize the listen stack (`nfc_stack_init`). |
| `nfc stack start [uid_hex]` | Start listening, optionally with a forced UID (`nfc_stack_start`). |
| `nfc stack stop` | Stop listening (`nfc_stack_stop`). Use between poll and listen sessions. |
| `nfc stack load <tag>` | Load a `.card` slot into the listeners before `start` (`nfc_stack_load`). |

### 3.3 `nfc store …` (`NFC_STORE_RAM_SHELL`)

| Command | Action |
|---------|--------|
| `nfc store list` | List occupied RAM slots (`cmd_nfc_store_ram_list`). |
| `nfc store dump <slot>` | Hex-dump a RAM slot blob (`cmd_nfc_store_ram_dump`). |
| `nfc store import <slot> <hex>` | Import a raw hex blob into a slot (`nfc_store_ram_import`). |
| `nfc store import golden <name>` | Import a compiled-in golden by name (needs `NFC_STORE_GOLDENS`). |
| `nfc store save <slot>` | Serialize the active listeners to a slot (`nfc_stack_save`). Reader-only builds re-export the RAM slot. `-EBUSY` if listen active. |
| `nfc store load <slot>` | Load a slot into the listeners (`nfc_stack_load`), or peek metadata on a reader-only build. |

> Store shell lives only here. `nfc_store.c` carries no shell include and its
> default save callback is an inert stub (`-ENODEV`); the hex-dump save callback
> is owned by the applet shell adapter (`@@NFCDUMP@@`). (LANDED in Phase A.)

### 3.4 `nfc_transport …` (`NFC_HAL_SHELL`, top-level)

| Command | Action |
|---------|--------|
| `nfc_transport config` | Print HAL config (`fwi`). |
| `nfc_transport stats` | HAL counters: `discover_start`, `tag_detect`, `transceive`, `field_on/off`, `fragment_rx`, `apdu_assembled`, `frag_drop_buf`, `apdu_oversized`, `apdu_drop_cons`, `response_tx`, `errors`. (HIL acceptance counters.) |
| `nfc_transport state` | HAL state + capabilities (roles bitmask, technologies, tier). |

### 3.5 `pn7160 …` (`PN7160_SHELL`, top-level)

Raw NXP PN7160 controller access for bring-up and protocol probing.

| Command | Action |
|---------|--------|
| `pn7160 probe` | CORE_RESET presence probe. |
| `pn7160 fwver` | Read firmware version from CORE_RESET_NTF. |
| `pn7160 reset` | VEN hard reset (10 ms / 10 ms). |
| `pn7160 init` | CORE_RESET probe + CORE_INIT connect. |
| `pn7160 settings` | Apply `Nfc_settings.h` RF/core config blobs. |
| `pn7160 discovery start\|stop\|status` | RF discovery (reader/poll mode); `status` shows last UID/protocol/interface/mode-tech. |
| `pn7160 listen start\|stop\|status` | Card-emulation listen mode (ConfigureMode CARDEMU). |
| `pn7160 card recv\|send <hex…>` | Card-mode DATA_PACKET receive/send (after `listen start`). |
| `pn7160 tagcmd <hex…>` | Raw tag DATA_PACKET (`ReaderTagCmd`); use after discovery. |
| `pn7160 xcv <hex…>` | Raw NCI transceive. |
| `pn7160 dwl enter\|leave\|status` | Firmware-download mode via DWL GPIO (TML framing). |

---

## 4. Overlay / profile matrix

Which command groups light up depends on the engine + shell Kconfigs pulled in by
the build overlay.

| Overlay / profile | Engines on | Applet cmds available | DK primitives |
|-------------------|-----------|------------------------|---------------|
| **Reader** (`overlay-pn7160-stack.conf`) | reader + store | `nfc scan start/stop`, `nfc read`, `nfc check` | `nfc reader`, `nfc store`, `nfc_transport`, `pn7160` |
| **CE / listen** (`overlay-nfct-stack.conf`) | listen stack + store | `nfc emulate`, `nfc loop` (+ scan/read/check if reader also on) | `nfc stack`, `nfc store`, `nfc_transport` |
| **Reader + listen** (reader overlay + listen overlay) | reader + listen + store | full set: scan/read/emulate/loop/check | all of the above |
| **`CONFIG_SHELL=n`** (any) | as configured | none (no L2 surface) | none — engines/applets still link clean |

---

## 5. Typical bench workflows

**Read / clone a tag (reader build, `overlay-pn7160-stack.conf`):**
```
nfc scan start
nfc scan stop
nfc read tag1
nfc store list
nfc store load tag1
```

**Emulate from a golden (CE build, `overlay-nfct-stack.conf` or reader+listen):**
```
nfc emulate golden ndef_url
nfc_transport stats        # apdu_assembled > 0, apdu_drop_cons == 0
```

**Full applet loop (single PN7160, listen overlay layered):**
```
nfc read tag1
nfc stack stop             # single-flight: drop listen before re-polling
nfc loop tag1              # read → emulate → check, single PASS line
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

## 6. Notes & invariants

- **Shell never lives below Layer 2.** Every command here is in a
  `*_shell_cmds.c` (or the PN7160 module shell). As of Phase A the last rendering
  bodies (`nfc_applet_scan_print`, the interleaved `nfc_applet_loop` prints) were
  moved out of the L1 service files into the L2 adapter, so a `CONFIG_SHELL=n`
  reader build links with zero shell symbols below L2.
- **Single-flight:** never poll and listen on the same controller concurrently.
  Stop discovery / `nfc stack stop` (or let `nfc loop` sequence it) between
  sessions.
- **Clone-only vs emulatable:** `nfc emulate` refuses clone-only cards (Classic,
  FeliCa, SLIX, ISO15693-3) per the policy applet. Emulatable set: NDEF,
  Ultralight, DESFire, EMV, Aliro.
- See [`NFC_HIL_PROTOCOL_GUIDE.md`](NFC_HIL_PROTOCOL_GUIDE.md) for exact flash
  commands, per-protocol PASS criteria, and gate mapping; see
  [`plans/NFC_APPLET_API_LAYERING.md`](plans/NFC_APPLET_API_LAYERING.md) for the
  L1 service API contract.
