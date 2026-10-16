# AGENTS.md — Emulanders Project Schema & Wiki Maintenance Protocol

This document is the **Schema** for the Emulanders repository, structured after Andrej Karpathy's [LLM Wiki Architecture](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f). It defines the repository's three-layer knowledge system, non-negotiable hardware and cryptographic invariants, and the operational workflows (`Ingest`, `Query`, `Lint`) that keep the codebase and its living specification (`wiki/`) synchronized.

---

## 1. Three-Layer Knowledge Architecture

| Layer | Location | Role & Mutability |
| :--- | :--- | :--- |
| **Layer 1: Codebase & Hardware Truth** | `emulanders/`, `overlay/`, `EmulandersPortal/`, `.github/workflows/` | Immutable hardware protocol constraints (Horizon OS `nfc:mf:u`, Mifare Classic 1K layout) and the active source code implementing them. |
| **Layer 2: The Living Wiki** | [`wiki/`](wiki/index.md) | Cross-referenced technical specifications, hardware protocol tables, UI/display rules, and release checklists maintained continuously alongside code changes. |
| **Layer 3: The Schema** | [`AGENTS.md`](AGENTS.md) | This file. Defines architectural invariants, quality gates, and maintenance workflows. |

---

## 2. Core Engineering Philosophy & Quality Gate

1. **Zero-Churn Rule:** Never modify source code for cosmetic reasons or arbitrary refactoring. Every code diff must deliver a verifiable improvement in **memory safety**, **hardware compatibility**, **cryptographic accuracy**, or **UI layout precision** on Nintendo Switch hardware.
2. **Read Before Edit:** Before modifying any subsystem (Sysmodule IPC, Mifare cryptography, Tesla Overlay, or Emulanders Portal UI), read [`wiki/index.md`](wiki/index.md) and the corresponding specification page in `wiki/`.
3. **Sync Wiki With Code:** Any change to IPC command IDs, file paths, UI layout rules, or version numbers must update the relevant page in `wiki/` and append an entry to [`wiki/log.md`](wiki/log.md) in the same session.

---

## 3. Non-Negotiable Domain Invariants

### 3.1 Cryptographic & Mifare Classic 1K Invariants
- **Exact Dump Size:** Every Skylander figure image (`.dump`, `.sky`, `.bin`, `.dmp`) is strictly `1024` bytes (`16` sectors × `4` blocks × `16` bytes/block).
- **AArch64 32-Bit Word Invariant:** On Nintendo Switch (`aarch64-none-elf`), `unsigned long` is 64-bit (`8` bytes). All AES-128 Rijndael state tables and key schedules in [`aes.cpp`](EmulandersPortal/source/crypto/aes.cpp) and [`aes.h`](EmulandersPortal/source/crypto/aes.h) MUST use fixed-width `uint32_t`. Never use `unsigned long` in cryptographic routines.
- **Zero-Block Preservation Invariant:** Uninitialized all-zero 16-byte data blocks (`0x00 * 16`) in blank or newly created `.dump` files MUST remain unencrypted (`0x00 * 16`) until the game writes non-zero data to them. Encrypting zero blocks on a blank tag corrupts game initialization.
- **Sector Trailer Integrity:** Block 3 of every sector (`block_idx % 4 == 3`) is the Mifare Sector Trailer (`Key A [6B] | Access Bits [4B] | Key B [6B]`). Never encrypt Sector 0 (`blocks 0..3`) or any Sector Trailer block (`block_idx % 4 == 3`) with AES-128.
- **Sector 0 Access Bits:** Block 3 of Sector 0 must hold access bits `0x7F 0x0F 0x08 0x69`. Sectors `1..15` trailers hold `0x7F 0x0F 0x08 0x69` during active gameplay.

### 3.2 Horizon OS `nfc:mf:u` Sysmodule Invariants
- **Service Scope:** `emulanders` intercepts **`nfc:mf:u` only** (Mifare NFC user service used by *Skylanders Imaginators*, Title ID `0100F35000A56000`). It does NOT intercept `nfc:am:u` or `nfp:user`, guaranteeing zero-conflict coexistence with `emuiibo`.
- **14-Command IUser Interface:** `IUser` (`emulanders/src/mifare.rs`) must implement all 14 Horizon OS `nfc:mf:u` commands (`[0..13]`), returning valid kernel handles for `AttachAvailabilityChangeEvent` (`[4]`), `AttachActivateEvent` (`[11]`), and `AttachDeactivateEvent` (`[12]`).
- **UTF-8 Log Truncation Safety:** In `no_std` Rust (`emulanders/src/emu.rs`), slicing `G_DEBUG_LOG` must always advance to `log.is_char_boundary(idx)` before slicing so multi-byte UTF-8 filenames never trigger a panic/abort in the sysmodule.

### 3.3 Nintendo Switch Display & Input Invariants
- **Resolution & Aspect Ratio (16:9):**
  - **Handheld Mode:** `1280 × 720` logical/physical pixels (`ui_scale = 1.0`).
  - **Docked Mode:** `1920 × 1080` TV output (`ui_scale = win_h / 720.0f` when dynamically resized, or hardware-scaled from `1280 × 720` by `nvdrv`/`vi`).
- **Dynamic Vertical Sizing Law:** Inside `Content` (`EmulandersPortal/source/screens.cpp`), child tables (`BeginTable`), scroll containers (`BeginChild`), and the interactive crop viewport MUST compute their height from `ImGui::GetContentRegionAvail().y` rather than hardcoded `content_h - N` offsets. Hardcoded offsets ignore preceding headers/toolbars and cause bottom rows to clip behind the `Footer` bar.
- **ImGui Navigation Layer Lock:** Every frame in `EmulandersPortal/source/main.cpp` must enforce `g_ctx.NavLayer = ImGuiNavLayer_Main` and `g_ctx.NavWindowingToggleLayer = false`. Without this, pressing `[Y]` (`ImGuiKey_GamepadFaceLeft`) switches Dear ImGui to an empty menu layer and kills D-Pad navigation.
- **Controller Hotplugging:** `SDL_CONTROLLERDEVICEADDED` and `SDL_CONTROLLERDEVICEREMOVED` must be handled in the main event loop so docking/undocking the Switch (switching between attached Joy-Cons and wireless Pro Controllers) never breaks analog stick scrolling or crop zooming.
- **Portrait Artwork Specification:** Custom and community figure artwork is standardized at **`175 × 275` pixels** (`RGBA32` `.png`, aspect ratio `0.6363636`). Thumbnail loading in tables must be throttled to **3–4 textures per frame** to maintain a locked 60 FPS.

---

## 4. LLM Wiki Workflows

### Workflow A: Ingest (Adding New Findings or Features)
When new hardware behavior, bug root causes, or features are implemented:
1. Inspect the relevant wiki page(s) in `wiki/`.
2. Update the technical specification in `wiki/` to reflect the new implementation or invariant.
3. Update [`wiki/index.md`](wiki/index.md) if a new page was added or its scope changed.
4. Append a timestamped entry to [`wiki/log.md`](wiki/log.md) using the strict header format:
   ```markdown
   ## [YYYY-MM-DD] <type> | <Short Title>
   ```
   where `<type>` is one of `ingest`, `fix`, `release`, `lint`, or `arch`.

### Workflow B: Query (Answering Questions or Planning Changes)
1. Read [`wiki/index.md`](wiki/index.md) first to locate relevant architectural specs.
2. Read the linked `wiki/*.md` pages before grepping the entire codebase from scratch.
3. If a deep investigation uncovers new non-obvious relationships, document them back into `wiki/` so future sessions inherit the knowledge.

### Workflow C: Lint (Project & Wiki Health Check)
When performing a project-wide review ("Pente Fino") or preparing a release:
1. Verify version string synchronization across all 8 versioned manifests (listed in [`wiki/release-and-packaging.md`](wiki/release-and-packaging.md)).
2. Verify all `wiki/` links resolve and match the current source tree.
3. Compile all three targets (`emulanders` Rust sysmodule, `overlay` `.ovl`, and `EmulandersPortal` `.nro`) and verify zero build errors.
4. Record the audit outcome in [`wiki/log.md`](wiki/log.md).
