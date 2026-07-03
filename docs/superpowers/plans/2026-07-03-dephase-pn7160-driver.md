# Dephase in-tree PN7160 driver → consume external `nfc-pn7160` module — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the app build against the proper external `nfc-pn7160` Zephyr module and remove the outdated in-tree `modules/nfc_pn7160/` driver, with no changes to app source code.

**Architecture:** The only app code touching the driver is the HAL adapter `src/nfc/hal/nfc_transport_pn7160.c`, whose entire call surface is byte-compatible with the new driver (same module name `nfc_pn7160`, same headers `nfc/pn7160.h`, same DT compat `nxp,pn7160`, same Kconfig, identical function signatures). So this is a **module-wiring swap plus deletion**, not a code rewrite. The new driver is already a west project in the local NCS workspace (`file://`), so local builds resolve it automatically once the old in-tree injection is removed; CI resolves it via a new pinned entry in the app's own `west.yml`.

**Tech Stack:** Zephyr RTOS / nRF Connect SDK v3.2.4, west manifests, Zephyr modules (`zephyr/module.yml`), Twister, GitHub Actions.

## Global Constraints

- NCS/Zephyr version: **v3.2.4** (do not change the `nrf` pin).
- Board target for build verification: **`nrf54l15dk/nrf54l15/cpuapp`**.
- New driver source of truth: **`https://github.com/majidsurefow/nfc-pn7160`** (private repo).
- CI pin revision (verbatim): **`faef0be3070fce5db8f7289466243b65961e0b79`**.
- Zephyr module name is **`nfc_pn7160`** for both drivers — only one may be active at a time (name collision otherwise).
- **No changes to any file under `src/`** (adapter and stack stay as-is). If a build error implicates `src/`, STOP — the assumption that the API is compatible is wrong; report rather than edit.
- Every build verification must be run in a shell that has sourced the NCS env:
  `source scripts/env/ncs-env.sh` (sets `ZEPHYR_BASE`, toolchain, `west` on PATH).
- Documentation prose updates (CI_TESTING, ZEPHYR_WORKSPACE, CONTEXT.md, rst, historical specs/plans) are **out of scope** — see "Deferred" at the end.

---

## File map

| Action | Path | Responsibility |
|--------|------|----------------|
| Modify | `CMakeLists.txt` | Remove `ZEPHYR_EXTRA_MODULES` injection of the old driver |
| Delete | `modules/nfc_pn7160/` (whole tree) | Old hand-written driver |
| Delete | `doc/` (whole dir) | App-level Doxygen of the old driver headers |
| Delete | `.github/workflows/doc-build.yml` | Builds old-driver Doxygen |
| Delete | `.github/workflows/doc-publish.yml` | Publishes old-driver Doxygen to Pages |
| Delete | `.github/workflows/doc-publish-pr.yml` | PR comment for old-driver Doxygen |
| Delete | `.github/workflows/doxygen-coverage-delta.yml` | Doc-coverage delta on old-driver headers |
| Delete | `scripts/ci/doxygen_coverage_diff.py` | Helper used only by the doc workflows |
| Modify | `west.yml` | Add pinned `nfc-pn7160` project for CI; rewrite stale header comment |
| Modify | `.github/workflows/twister.yaml` | Drop old-driver unit-test dir from `UNIT_DIRS` |
| Modify | `.github/workflows/codecov.yaml` | Repoint `UNIT_TEST_DIR` to an app-owned unit test |
| Modify | `.github/workflows/devicetree.yml` | Remove the old-driver DT-bindings validation job |

---

## Task 1: Switch the active driver (remove the old-driver injection)

**Files:**
- Modify: `CMakeLists.txt:9-11`

**Interfaces:**
- Consumes: nothing (first task).
- Produces: a build in which the Zephyr module `nfc_pn7160` resolves to the west-discovered external repo (`modules/nfc-pn7160` under the NCS workspace), not the in-tree tree.

- [ ] **Step 1: Remove the ZEPHYR_EXTRA_MODULES block**

In `CMakeLists.txt`, delete these three lines (currently lines 9–11):

```cmake
list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/nfc_pn7160
)
```

Also remove the now-orphaned blank line so the file reads:

```cmake
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(writable_ndef_msg)
```

- [ ] **Step 2: Verify a PN7160 config builds against the NEW driver**

Run:

```bash
source scripts/env/ncs-env.sh
west twister -T . -t pn7160 -p nrf54l15dk/nrf54l15/cpuapp \
  --build-only --inline-logs -v --clobber-output -j "$(sysctl -n hw.ncpu)"
```

Expected: both `sample.nfc.writable_ndef_msg.pn7160.i2c` and `...pn7160.spi` report **PASSED** (build-only). No CMake error about a duplicate `nfc_pn7160` module.

- [ ] **Step 3: Confirm the new driver sources were actually compiled (not the old tree)**

Run:

```bash
grep -rl "modules/nfc-pn7160/src/pn7160_driver.c" twister-out*/ 2>/dev/null | head
grep -rl "writable_ndef_msg/modules/nfc_pn7160" twister-out*/ 2>/dev/null | head
```

Expected: the first grep prints at least one match (compile db / ninja rules reference `modules/nfc-pn7160`, the external repo). The second grep prints **nothing** (the in-tree tree was not compiled).

- [ ] **Step 4: Commit**

```bash
git add CMakeLists.txt
git commit -m "build(nfc): stop injecting in-tree pn7160 driver via ZEPHYR_EXTRA_MODULES

Resolves the nfc_pn7160 module-name collision so the west-discovered
external nfc-pn7160 module is used instead of the outdated in-tree copy.

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Task 2: Delete the old driver tree

**Files:**
- Delete: `modules/nfc_pn7160/` (entire directory)

**Interfaces:**
- Consumes: Task 1 (build already switched to the external module).
- Produces: a repo with no in-tree PN7160 driver.

- [ ] **Step 1: Remove the tree**

```bash
git rm -r modules/nfc_pn7160
```

(If `modules/` is now empty, git will stop tracking it automatically — no action needed.)

- [ ] **Step 2: Verify the build is still green**

```bash
source scripts/env/ncs-env.sh
west twister -T . -t pn7160 -p nrf54l15dk/nrf54l15/cpuapp \
  --build-only --inline-logs -v --clobber-output -j "$(sysctl -n hw.ncpu)"
```

Expected: both pn7160 configs **PASSED**. (Proves nothing in the app referenced the deleted tree.)

- [ ] **Step 3: Confirm no build/source references to the deleted path remain**

```bash
grep -rn "modules/nfc_pn7160" CMakeLists.txt src/ boards/ confs/ sample.yaml *.conf 2>/dev/null
```

Expected: **no output**.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "chore(nfc): remove outdated in-tree pn7160 driver (modules/nfc_pn7160)

Superseded by the external nfc-pn7160 Zephyr module.

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Task 3: Retire the app-level driver Doxygen pipeline

The `doc/` pipeline documents **only** the deleted driver headers (`DOC_SRC := ../modules/nfc_pn7160/include`) and four workflows drive it. The external repo owns its own Doxygen (`Doxyfile.ci`), so this app infra is now obsolete.

**Files:**
- Delete: `doc/` (Doxyfile, Makefile, README.md, requirements.txt)
- Delete: `.github/workflows/doc-build.yml`
- Delete: `.github/workflows/doc-publish.yml`
- Delete: `.github/workflows/doc-publish-pr.yml`
- Delete: `.github/workflows/doxygen-coverage-delta.yml`
- Delete: `scripts/ci/doxygen_coverage_diff.py`

**Interfaces:**
- Consumes: Task 2 (driver headers already deleted).
- Produces: no app-level doc build; `scripts/ci/footprint_report.py` remains untouched.

- [ ] **Step 1: Remove doc dir, doc workflows, and the doc-only helper script**

```bash
git rm -r doc
git rm .github/workflows/doc-build.yml \
       .github/workflows/doc-publish.yml \
       .github/workflows/doc-publish-pr.yml \
       .github/workflows/doxygen-coverage-delta.yml
git rm scripts/ci/doxygen_coverage_diff.py
```

- [ ] **Step 2: Verify no dangling references to the retired doc infra**

```bash
grep -rn "doxygen_coverage_diff\|doc-build\.yml\|doxygen-coverage-delta\|make -C doc\|make doxygen" \
  .github/ scripts/ 2>/dev/null
```

Expected: **no output**.

- [ ] **Step 3: Confirm the unrelated footprint script survived**

```bash
test -f scripts/ci/footprint_report.py && echo "footprint_report.py present"
```

Expected: `footprint_report.py present`.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "ci(docs): retire app-level pn7160 Doxygen pipeline

Doc responsibility moved with the driver to the external nfc-pn7160 repo
(which ships its own Doxyfile.ci). Removes doc/, the four documentation
workflows, and the doc-only coverage-diff helper.

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Task 4: Wire the new driver into the app `west.yml` for CI

Local builds resolve the driver via the NCS workspace manifest (`file://`); CI uses the app's own `west.yml`. Add a pinned GitHub entry so a fresh CI checkout resolves the driver reproducibly, and fix the now-false header comment.

**Files:**
- Modify: `west.yml`

**Interfaces:**
- Consumes: nothing from prior tasks (independent).
- Produces: an app manifest with a `nfc-pn7160` project pinned to `faef0be…`, cloned to `modules/nfc-pn7160` in CI.

- [ ] **Step 1: Rewrite the stale header comment and add the project**

Replace the top comment block (lines 1–4) with:

```yaml
# West manifest for writable_ndef_msg (NCS workspace root, T2 topology).
# CI and local Multipass use the same NCS pin as .github/workflows/twister.yaml.
# The PN7160 driver is the external nfc-pn7160 module, pinned below; CI clones it
# from GitHub. Locally it is resolved by the NCS workspace manifest. See docs/CI_TESTING.md.
```

Then add the `nfc-pn7160` project to the `projects:` list, immediately after the `nrf` entry, so the list reads:

```yaml
  projects:
    - name: nrf
      repo-path: sdk-nrf
      revision: v3.2.4
      import: true

    - name: nfc-pn7160
      url: https://github.com/majidsurefow/nfc-pn7160
      revision: faef0be3070fce5db8f7289466243b65961e0b79
      path: modules/nfc-pn7160
```

- [ ] **Step 2: Validate the manifest YAML and the new entry**

Run:

```bash
python3 - <<'PY'
import yaml, sys
d = yaml.safe_load(open("west.yml"))
projs = {p["name"]: p for p in d["manifest"]["projects"]}
assert "nfc-pn7160" in projs, "nfc-pn7160 project missing"
p = projs["nfc-pn7160"]
assert p["revision"] == "faef0be3070fce5db8f7289466243b65961e0b79", p.get("revision")
assert p["url"] == "https://github.com/majidsurefow/nfc-pn7160", p.get("url")
assert p["path"] == "modules/nfc-pn7160", p.get("path")
assert projs["nrf"]["revision"] == "v3.2.4", "nrf pin changed"
print("west.yml OK")
PY
```

Expected: `west.yml OK`.

- [ ] **Step 3: Commit**

```bash
git add west.yml
git commit -m "west(nfc): add pinned nfc-pn7160 module project for CI

CI resolves the PN7160 driver from GitHub at a pinned SHA; local builds
still use the NCS workspace file:// entry. Also fixes the stale header
comment that described the removed in-tree module.

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

> **Note (non-blocking for this task):** `nfc-pn7160` is a private repo, so CI needs read access (deploy key or PAT wired into the checkout/west step). That credential provisioning is a repo-settings action outside this plan; CI driver-clone will fail until it's in place.

---

## Task 5: Migrate remaining CI workflows off the in-tree driver paths

**Files:**
- Modify: `.github/workflows/twister.yaml:150`
- Modify: `.github/workflows/codecov.yaml:26`
- Modify: `.github/workflows/devicetree.yml` (remove the `devicetree-bindings` job + its `needs` reference + a stale path filter)

**Interfaces:**
- Consumes: Task 2 (in-tree tests/bindings deleted).
- Produces: CI that references no `modules/nfc_pn7160` path.

- [ ] **Step 1: twister.yaml — drop the old-driver unit test dir**

In `.github/workflows/twister.yaml`, in the `UNIT_DIRS=( … )` array, delete this line:

```
            modules/nfc_pn7160/tests/unit/pn7160_tml
```

Leave the app's own `tests/unit/nfc_*` and `tests/unit/nfc_apdu_asm` entries intact.

- [ ] **Step 2: codecov.yaml — repoint the coverage target to an app-owned test**

In `.github/workflows/codecov.yaml`, change line 26 from:

```yaml
  UNIT_TEST_DIR: modules/nfc_pn7160/tests/unit/pn7160_tml
```

to (the HAL model test, which is app-owned and directly exercises the transport layer):

```yaml
  UNIT_TEST_DIR: tests/unit/nfc_apdu_asm
```

- [ ] **Step 3: devicetree.yml — remove the old-driver DT bindings job**

The bindings validated by the `devicetree-bindings` job lived under `modules/nfc_pn7160/dts/bindings`, which no longer exists in this repo (they belong to the external driver). Remove the whole `devicetree-bindings:` job (currently lines 30–68).

Then update the `devicetree-status-check` job's `needs:` list — remove the `- devicetree-bindings` line so it reads:

```yaml
  devicetree-status-check:
    name: Check Devicetree Status
    if: always()
    needs:
      - devicetree-build
    uses: ./.github/workflows/ready-to-merge.yml
    with:
      needs_context: ${{ toJson(needs) }}
```

Finally, in the `on.pull_request.paths:` filter, remove the now-irrelevant line:

```yaml
      - 'modules/**/dts/**'
```

(Keep `boards/**`, `sample.yaml`, `overlay*.conf`, and the workflow's own path. The `devicetree-build` job — `west twister -T . -t pn7160 --build-only` — stays; it exercises the new driver through the app overlays.)

- [ ] **Step 4: Verify no workflow references the old driver path, and YAML is valid**

```bash
grep -rn "modules/nfc_pn7160" .github/workflows/ ; echo "grep-exit=$?"
python3 - <<'PY'
import yaml, glob
for f in glob.glob(".github/workflows/*.yml") + glob.glob(".github/workflows/*.yaml"):
    yaml.safe_load(open(f))
    print("ok", f)
PY
```

Expected: the grep prints **no path lines** and `grep-exit=1` (no matches); every workflow file prints `ok`.

- [ ] **Step 5: Verify the repointed codecov target actually runs**

```bash
source scripts/env/ncs-env.sh
west twister -T tests/unit/nfc_apdu_asm -t ci_unit -p qemu_cortex_m3 \
  --no-sysbuild --inline-logs -v --clobber-output -j "$(sysctl -n hw.ncpu)"
```

Expected: the `nfc_apdu_asm` unit test config reports **PASSED**. (Confirms the new `UNIT_TEST_DIR` is a real, runnable ci_unit target.)

- [ ] **Step 6: Commit**

```bash
git add .github/workflows/twister.yaml .github/workflows/codecov.yaml .github/workflows/devicetree.yml
git commit -m "ci(nfc): migrate workflows off the removed in-tree pn7160 paths

- twister: drop modules/nfc_pn7160 unit test dir
- codecov: repoint UNIT_TEST_DIR to app-owned tests/unit/nfc_apdu_asm
- devicetree: remove old-driver DT-bindings validation job (bindings now
  live in the external nfc-pn7160 repo); keep overlay build job

Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
```

---

## Task 6: Final verification sweep

**Files:** none (verification only).

**Interfaces:**
- Consumes: all prior tasks.
- Produces: confirmation that no build/CI/source reference to the old driver remains and both transports build.

- [ ] **Step 1: Repo-wide check — no live (non-doc) references to the old driver path**

```bash
grep -rn "modules/nfc_pn7160" . \
  --exclude-dir=.git --exclude-dir=build --exclude-dir=twister-out \
  2>/dev/null | grep -v "^\./docs/"
```

Expected: **no output**. (Remaining matches are confined to `docs/` — see Deferred — and are intentional for now.)

- [ ] **Step 2: Clean build of both PN7160 transports against the external module**

```bash
source scripts/env/ncs-env.sh
west twister -T . -t pn7160 -p nrf54l15dk/nrf54l15/cpuapp \
  --build-only --inline-logs -v --clobber-output -j "$(sysctl -n hw.ncpu)"
```

Expected: `sample.nfc.writable_ndef_msg.pn7160.i2c` and `...pn7160.spi` both **PASSED**; no duplicate-module error.

- [ ] **Step 3: Report result**

Summarize: both configs built, codecov target runs, zero live references to `modules/nfc_pn7160`. Note the two outstanding external prerequisites for CI green — (a) private-repo read credential for the `nfc-pn7160` clone, (b) the deferred documentation pass below.

---

## Deferred (explicitly NOT in this plan)

Per the approved scope, these are left for a follow-up documentation pass. They reference the old path but do **not** break the build:

- **Live wiring docs (stale, should be updated soon):** `docs/CI_TESTING.md`, `docs/nfc/CI_TESTING.md`, `docs/ZEPHYR_WORKSPACE.md`, `docs/nfc/ZEPHYR_WORKSPACE.md`, `docs/nfc/pn7160-driver.rst`, `docs/nfc/NFC_HAL_GUIDE.md`, `docs/nfc/PN7160_SHELL_AND_EXAMPLES.md`, `docs/nfc/NFC_STACK_ARCHITECTURE.md`, `docs/nfc/NFC_MEMORY_STACK.md`, and `src/nfc/hal/CONTEXT.md:34` (one-line path fix `nfc_pn7160` → `nfc-pn7160`).
- **Historical records (leave as-is):** everything under `docs/nfc/archive/`, `docs/nfc/specs/`, `docs/nfc/plans/`, and `docs/superpowers/specs/` — dated design/plan/review documents that record the state at their time of writing.
- **Provision CI read access** to the private `majidsurefow/nfc-pn7160` repo (deploy key / PAT) — a repo-settings action, blocking for CI-green but not a code change.
- Adopting the new driver's added capabilities (`pn7160_nci_discovery_start_preserve_mode`, the `pn7160_nci_rf_intf.info` union) in the adapter/stack.
