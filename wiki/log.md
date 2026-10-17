# Emulanders Engineering Log

This file is an append-only chronological record of significant architectural changes, bug root causes, releases, and wiki updates.
Format: `## [YYYY-MM-DD] <type> | <Title>` (`<type>` $\in$ `{release, fix, arch, ingest, lint}`).

---

## [2026-04-14] release | Emulanders v1.0.0 Initial Stable Release
- Tagged and published `1.0.0` across all three components (`emulanders` sysmodule, `overlay.ovl`, `EmulandersPortal.nro`).
- Synchronized version strings across all 8 build manifests (`Cargo.toml`, `emulanders/Makefile`, `overlay/Makefile`, `overlay/source/main.cpp`, `EmulandersPortal/Makefile`, `EmulandersPortal/source/app.hpp`, `README.md`, `RELEASE_NOTES.md`).
- Published dual release archives on GitHub Releases: `emulanders-v1.0.0.zip`, `EmulandersPortal-hbappstore.zip`, and `emulanders_assets_pack.zip`.

## [2026-10-16] release | Published on Nintendo Switch Homebrew App Store (4TU)
- Pull Request [`fortheusers/switch-hbas-repo#938`](https://github.com/fortheusers/switch-hbas-repo/pull/938) merged by maintainer `vgmoose`.
- Package `Emulanders` (`1.0.0`, `sha256: 783d5a1339150d267c148394cfed5b4db92a17399dd8e63fa0d14474e55008f3`) verified live on `https://switch.cdn.fortheusers.org/repo.json`.
- Documented the 4TU Spinarak / DragoniteBot packaging contract in [`wiki/release-and-packaging.md`](release-and-packaging.md).

## [2026-10-16] arch | Established Karpathy LLM Wiki & Spec-Driven Schema
- Created [`AGENTS.md`](../AGENTS.md) at repository root defining the three-layer knowledge architecture, domain invariants, and `Ingest` / `Query` / `Lint` workflows.
- Created [`wiki/index.md`](index.md) and 5 deep technical specification pages:
  - [`wiki/architecture.md`](architecture.md)
  - [`wiki/mifare-crypto-spec.md`](mifare-crypto-spec.md)
  - [`wiki/nfc-ipc-protocol.md`](nfc-ipc-protocol.md)
  - [`wiki/ui-display-and-input.md`](ui-display-and-input.md)
  - [`wiki/release-and-packaging.md`](release-and-packaging.md)

## [2026-10-16] fix | Post-1.0 Comprehensive Audit ("Pente Fino") Fixes
- **Sysmodule UTF-8 Log Slice Safety ([`emulanders/src/emu.rs`](../emulanders/src/emu.rs)):** Fixed a potential panic in `log_debug()` where truncating `G_DEBUG_LOG` at `log.len() / 2` could slice mid-codepoint on multi-byte UTF-8 figure names (e.g., Portuguese accented characters), which would call `abort::abort()` in `no_std`. Added a `log.is_char_boundary(new_start)` loop before slicing.
- **Tesla Overlay Version Check ([`overlay/source/main.cpp`](../overlay/source/main.cpp)):** Fixed a bug in `EmulandersOverlay::initServices()` where `g_InitializationStatus = InitializationStatus::OkVersionMismatch` was unconditionally overwritten on the next line by `g_InitializationStatus = InitializationStatus::Ok`.
- **Tesla Overlay Language Sync & Locale Mapping ([`overlay/source/tr/tr_Translation.cpp`](../overlay/source/tr/tr_Translation.cpp)):** Mapped `"pt"` and `"pt-PT"` to `"pt-BR"` (and `"zh-CN"`/`"zh-TW"`/`"zh-HK"` to `"zh-Hans"`/`"zh-Hant"`), and added `GetConfiguredLanguage()` to read the user's `"language"` preference from `sdmc:/emulanders/config.json` before falling back to the OS locale.
- **Portal UI Dynamic Height & 720p/1080p Viewport Fit ([`EmulandersPortal/source/screens.cpp`](../EmulandersPortal/source/screens.cpp)):** Replaced hardcoded `content_h - N` heights in `RenderScreenFileManager`, `RenderScreenImageEditor` (Steps 1, 2, and 3), `RenderScreenDownloaders` (`URLList`), and `RenderScreenSettings` (`SettingsScroll`) with `ImGui::GetContentRegionAvail().y`, eliminating ~55–75px vertical overflow behind the footer bar.
- **Portal Docked/Handheld Controller Hotplugging & Resize ([`EmulandersPortal/source/main.cpp`](../EmulandersPortal/source/main.cpp)):** Added `SDL_CONTROLLERDEVICEADDED`, `SDL_CONTROLLERDEVICEREMOVED`, and `SDL_WINDOWEVENT_SIZE_CHANGED` handlers in the event loop so transitioning between handheld Joy-Cons and docked Pro Controllers mid-session works seamlessly, and added texture cleanup for `G.sd_files` on exit.
- **Portal OTA Downloader Network Error Verification ([`EmulandersPortal/source/screens.cpp`](../EmulandersPortal/source/screens.cpp)):** Added `CURLcode` and HTTP status code verification (`http_code == 200`) to both figure dump and asset pack downloaders so offline or failed requests clean up temporary files and display a clear localized error message instead of extracting an empty archive.

## [2026-10-16] release | Emulanders v1.0.1
- Portal and overlay support 10 languages (en, pt-BR, es, fr, de, it, ru, ko, zh-Hans, zh-Hant), stored in `sdmc:/emulanders/config.json`. New `translations.cpp/hpp` in the Portal; `AutoBtnSize` sizes buttons from their label and language/theme lists wrap.
- Portal loads Cyrillic and CJK glyphs from the shared system fonts (Standard, ChineseSimplified, ChineseTraditional, KO).
- Figure Image Editor: toolbar buttons on one row and a single-line step header, about 95 px more list height at 720p.
- Version bumped to 1.0.1 in all manifests. Sysmodule and overlay are versioned together because the overlay compares them.
- Added [`wiki/switch-upgrade-playbook.md`](switch-upgrade-playbook.md) for the 22.1 -> 23.0 / AMS 1.12.1 / CNX 23.0.1-1 upgrade. Versioning policy: `1.0.x` for compatible fixes, `1.1.0` for firmware-driven changes that stay backwards compatible, `1.2.0+` if older firmware or the NFC IPC surface changes.

## [2026-10-17] verify & release | Firmware 23.0.1 Validation, Crop Layout Polish & HBAS PR #942
- **Horizon OS 23.0.1 & Atmosphère 1.12.1 Hardware Verification:** Performed full end-to-end upgrade on Nintendo Switch hardware via Daybreak. Confirmed zero breaking changes to `nfc:mf:u` IPC service interface. Interception, Joy-Con physical dumping, in-game figure summoning, and savegame write-back work out-of-the-box.
- **Cropper Button Clearance Fix ([`EmulandersPortal/source/screens.cpp`](../EmulandersPortal/source/screens.cpp)):** Adjusted live preview height cap to `125px` and button height to `38px` in Step 3 of the Figure Image Editor, restoring `190px` of vertical breathing room and preventing `(B) Back to Photos` from being clipped by the footer status bar in 720p handheld mode. Recompiled Portal and refreshed release zip bundles.
- **Homebrew App Store Pull Request ([`fortheusers/switch-hbas-repo#942`](https://github.com/fortheusers/switch-hbas-repo/pull/942)):** Submitted official 1.0.1 update PR containing the new 4:1 `848×208` widescreen banner (`screen.png`) and `256×150` non-square icon (`icon.png`), 10-language metadata, and bundle download URL.
- **Applet Mode Out-of-Memory Documentation:** Documented Horizon 23.x Applet Mode ~7–16 MB memory restrictions (`0xD401 / 2001-0106 Kernel OOM`) and established Title Override (holding `R` on any installed title) as the mandatory launch procedure.

