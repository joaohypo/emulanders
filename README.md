<p align="center">
  <img alt="emulanders" src="res/banner_cut.png" width="777">
</p>

<p align="center">
  <strong>Emulanders: Skylanders NFC (Mifare) Emulation for Nintendo Switch</strong>
</p>

<p align="center">
  <a title="Release" href="https://github.com/joaohypo/emulanders/releases/latest">
    <img alt="Latest Release" src="https://img.shields.io/github/v/release/joaohypo/emulanders?label=Latest%20Release&style=for-the-badge&color=blue">
  </a>
  <a title="Downloads" href="https://github.com/joaohypo/emulanders/releases">
    <img alt="Downloads" src="https://img.shields.io/github/downloads/joaohypo/emulanders/total?longCache=true&style=for-the-badge&label=Downloads&logoColor=fff&logo=GitHub">
  </a>
  <a title="License" href="https://github.com/joaohypo/emulanders/blob/main/LICENSE">
    <img alt="License" src="https://img.shields.io/github/license/joaohypo/emulanders?style=for-the-badge">
  </a>
</p>

## Disclaimer

This project does not distribute copyrighted game files, encryption keys, or proprietary Nintendo SDK code. Users must supply their own figure backups (`.dump` / `.bin`) dumped from physical toys they legally own.

---

## Overview

Emulanders is an ecosystem for Nintendo Switch (Atmosphère CFW) that emulates Skylanders NFC figures directly from the SD card in *Skylanders: Imaginators*.

It consists of three main components:
- **Sysmodule**: Background service intercepting `nfc:mf:u` IPC calls.
- **Tesla Overlay**: In-game menu to select, swap, and unmount figures on-the-fly.
- **Emulanders Portal**: Standalone Homebrew Menu app to dump physical toys via Joy-Con NFC, clone dumps, crop screenshot portraits, and download community assets.

Development status and future objectives are tracked in [ROADMAP.md](ROADMAP.md).

### Technical Background
Standard Nintendo Amiibos use NTAG215 tags through the high-level `nfp` service. Skylanders figures use NXP Mifare Classic 1K tags. *Skylanders: Imaginators* bypasses the standard Amiibo stack and communicates directly with the console's low-level `nfc:mf:u` (Mifare User) interface. Emulanders intercepts these calls and serves figure data from the SD card (`sdmc:/emulanders/figures/`).

---

## Features

- **Mifare Protocol Emulation**: Implements the `nfc:mf:u` service interface for in-game toy detection.
- **Tesla Overlay**: Mount, unmount, and hot-swap figures during gameplay without closing the title.
- **Joy-Con NFC Dumper**: Dump physical toys using the Right Joy-Con or Switch Pro Controller NFC reader.
- **Figure Cloning**: 1:1 exact duplication or New Identity cloning (randomized UID, recomputed checksums, and recalculated sector keys) to avoid duplicate figure collisions in-game.
- **Screenshot Cropper**: Crop screenshots directly from the Switch Album (`Nintendo/Album`) to generate figure portraits (`figure.png`).
- **OTA Downloaders**: In-app downloaders for community asset packs and dump archives over Wi-Fi.
- **Localization**: 10 languages (English, Portuguese, Spanish, French, German, Italian, Russian, Korean, Simplified and Traditional Chinese), switchable at runtime and shared between the Portal and the overlay.
- **Custom Themes**: 7 color presets and RGB sliders for custom UI styling.
- **State Persistence**: Retains active figure and emulation toggle across console reboots.
- **Emuiibo Coexistence**: Runs alongside Emuiibo without service conflicts.

---

## Compatibility

- **Target Game**: *Skylanders: Imaginators* (Nintendo Switch version)
- **Firmware Support**: 4.0.0+ (minimum required by `nfc:mf:u`)
- **Tested On**: Firmware 22.1 with Atmosphère 1.11.1 (firmware 23.0 / Atmosphère 1.12.x not yet tested)

---

## Installation

### Quick Install (Recommended)
1. Download `Emulanders-Bundle-1.0.1.zip` from the [Releases](https://github.com/joaohypo/emulanders/releases/latest) page.
2. Extract the archive directly to the root of your SD card (copies `atmosphere`, `switch`, and `emulanders` folders).
3. *(Optional)* Download `emulanders_assets_pack.zip` and extract to the SD card root for full figure portrait artwork.
4. Restart your console.

### Manual Placement
- **Sysmodule**: `sd:/atmosphere/contents/420000000000E311/exefs.nsp`
- **Tesla Overlay**: `sd:/switch/.overlays/emulanders.ovl`
- **Portal App**: `sd:/switch/EmulandersPortal/EmulandersPortal.nro`
- **Overlay Translations**: `sd:/emulanders/overlay/lang/`

### SD Card Layout
```text
sdmc:/
├── atmosphere/contents/420000000000E311/
│   ├── exefs.nsp
│   ├── toolbox.json
│   └── flags/boot2.flag
├── switch/
│   ├── .overlays/
│   │   └── emulanders.ovl
│   └── EmulandersPortal/
│       ├── EmulandersPortal.nro
│       └── dumps_urls.txt
└── emulanders/
    ├── figures/        # Place your .dump / .bin backups here (subfolders supported)
    ├── assets/         # Character portrait PNGs ({CharID}.png)
    ├── overlay/lang/   # Language JSON files
    └── flags/          # System-managed state flags
```

---

## Usage

<div align="center">
  <video src="https://github.com/user-attachments/assets/7e61b20a-e117-4c3a-9585-9310cc56cbc0" autoplay="autoplay" loop="loop" muted="muted" playsinline width="480"></video>
  <br>
  <em><a href="https://youtu.be/oYJi1xWd5xI">Video demonstration on YouTube</a></em>
</div>

### Prerequisites
- The sysmodule must be running in the background (enabled by default via `boot2.flag`).
- **Turn emulation ON before launching the game**. The game initializes NFC during the boot screen. If emulation is off at boot, the sysmodule will not intercept the session.
- Wait until you are inside the game world before opening the Tesla overlay. The initial loading screen is memory-intensive and may cause overlay stutter if accessed during boot.

### Basic Workflow
1. Open the Tesla menu: `L + D-Pad Down + R3` (Right Stick click).
2. Select **emulanders**.
3. Toggle Emulation to **ON**.
4. Launch *Skylanders: Imaginators*.
5. Open the overlay in-game, choose **View Figures Folder**, and select a `.dump` file to mount.
6. The `>> ACTIVE` indicator confirms the figure is loaded.
7. To swap figures, select another file. To unmount, select the active file again or press `X`.

### Overlay Controls
- Enable Emulation: `ZR`
- Disable Emulation: `ZL`
- Select Figure / Folder: `A`
- Toggle Favorite: `Y`
- Clear Active Figure: `X`
- Help: `+`

### Technical Notes
- **Polling Overhead**: Unmounting the figure after loading a character in-game stops NFC polling and reduces background CPU overhead.
- **Swap Force**: *Skylanders: Imaginators* on Switch provides compatibility for Swap Force figures. Because the game does not implement the top/bottom mixing mechanic on this platform, selecting either the top or base dump loads the complete character.

### Image Resolution

The overlay displays portraits using a 3-tier fallback. Character ID and Variant ID are parsed directly from the dump file.

| Priority | Path Pattern | Example |
|:--|:--|:--|
| 1. Same-name PNG | `<figure_path_without_ext>.png` | `figures/King_Pen.png` |
| 2. Shared asset (variant) | `assets/{CharID}_{VariantID}.png` | `assets/000259_5000.png` |
| 3. Shared asset (base) | `assets/{CharID}.png` | `assets/000259.png` |

If no local PNG is found next to the dump, the overlay reads bytes `16-18` (Character ID) and `28-29` (Variant ID) from the file and looks in `sd:/emulanders/assets/`.

An **Assets Pack** with portraits for every Skylander and variant is available on the [Releases](https://github.com/joaohypo/emulanders/releases) page. Recommended image resolution: **RGBA PNG, ≤200×275px**.

---

## Emulanders Portal

<p align="center">
  <img alt="Emulanders Portal Feature Showcase" src="res/portal_showcase.png" width="850">
</p>

Emulanders Portal (`switch/EmulandersPortal/EmulandersPortal.nro`) is the collection manager app for the console. Launch it from the Homebrew Menu (`hbmenu`).

### Joy-Con NFC Dumper

<p align="center">
  <img alt="Joy-Con Physical NFC Dumper" src="res/screenshots/04_nfc_dumper.jpg" width="580">
</p>

Dump physical figures directly using the console:
- Navigate to **Dump Figure** and place a toy on the Right Joy-Con analog stick (or Pro Controller NFC sweet spot).
- Reads the Mifare Classic 1K tag through `nfp:user`, resolves Character/Variant IDs, and displays portrait artwork.
- Press `A` to save a 1KB `.dump` to `sdmc:/emulanders/figures/`.

### File Manager & Figure Cloning

<p align="center">
  <img alt="File Manager & Dump Cloning" src="res/screenshots/03_file_manager.jpg" width="580">
</p>

- **File Browser**: Browse folders, preview figures, organize directories, delete old dumps, and mark favorites for quick access in the overlay.
- **1:1 Clone**: Byte-for-byte duplicate of the dump file for simple backups.
- **New Identity Clone**: Generates a randomized 4-byte UID, updates Sector 0 BCC, calculates sector Key A values, re-encrypts data blocks under the new keys, and recalculates the Header CRC16 checksum. This allows running multiple copies of the same character simultaneously or leveling duplicate Creation Crystals independently without UID conflicts.

### Screenshot Cropper

<p align="center">
  <img alt="Figure Image Editor & Album Cropper" src="res/screenshots/02_image_cropper.jpg" width="580">
</p>

- Opens console screenshots directly from the Switch Album (`sdmc:/Nintendo/Album/` or custom folders).
- Preview 16:9 images, adjust the crop box with the D-Pad / left stick or drag via touchscreen, and zoom with right stick / shoulder buttons.
- Saves a 175×275 portrait alongside the dump (`figure.png`), immediately updating thumbnails in both Portal and overlay.

### OTA Downloaders

<p align="center">
  <img alt="OTA Dump Downloader" src="res/screenshots/05_dump_downloader.jpg" width="580">
</p>

- **Asset Pack Downloader**: Downloads and extracts the community portrait library directly to `sdmc:/emulanders/assets/` over Wi-Fi.
- **Figure Dumps Downloader**: Downloads figure packs directly to the SD card.

### Settings & Themes

<p align="center">
  <img alt="Settings, Themes and Bilingual Support" src="res/screenshots/06_settings_themes.jpg" width="580">
</p>

- **Themes**: 7 color presets (Emulanders Blue, Portal Purple, Magma Red, Life Green, Tech Orange, Undead Violet, Dark Void) or custom RGB adjustment.
- **Localization**: 10 languages, selectable in Settings. The overlay follows the same choice.
- **Input**: Supports Joy-Cons, Pro Controllers, and capacitive touchscreen.

---

## Known Issues

### Docked Mode UI Disappearance
In Docked Mode, if the console's **Adjust Screen Size** setting is set below 100%, selecting emulanders from the Tesla menu may cause the overlay window to disappear while the base Tesla menu remains responsive. This is a system compositor limitation with overscan rescaling.

**Workaround**: Set your console's screen scaling to 100% (**System Settings > TV Settings > Adjust Screen Size**). If the image is clipped on your TV, adjust the overscan or aspect ratio setting on your TV itself (e.g. "Just Scan", "1:1 Pixel Mapping", or "Fit to Screen").

### Handheld Mode Joy-Con Attachment
When Joy-Cons are physically attached in handheld mode, the game may display a notice stating "Controller not compatible with reading toys." Detach the Joy-Cons to continue. This is an internal limitation of *Skylanders: Imaginators* and not an issue with the sysmodule.

---

## Coexistence
Emulanders and Emuiibo target different system interfaces (`nfc:mf:u` and `nfp`, respectively). Both can remain installed and active simultaneously without service conflicts.

---

## Debugging
If a specific dump fails to load:
1. Open the overlay and navigate to **Logs Manager**.
2. Enable **Debug Logging**.
3. Reproduce the issue in-game.
4. Select **Extract to SD** to write the log buffer to `sdmc:/emulanders/debug_log_dump.txt`.
5. Attach the file when opening an issue.

---

## Building
Requires devkitPro (devkitA64) and the [Switch Rust Toolchain](https://github.com/aarch64-switch-rs/setup-guide).
- Release build: `make dist`
- Debug build: `make dist-dev`

---

## Credits
Emulanders builds upon the research, sysmodule architecture, and UI concepts originally created for the **Emuiibo** project by XorTroll and its contributors.

- [**XorTroll**](https://github.com/XorTroll): Original Emuiibo project, sysmodule framework, and Switch homebrew tooling.
- **Emuiibo Contributors**: *Subv, ogniK, averne, spx01, SciresM* and everyone involved in early `nfp-mitm` research.
- **libtesla Developers**: *AD2076* and *AmonRaNet* for the Tesla overlay framework.
- [**Switchbrew**](https://switchbrew.org/): Technical documentation and libnx.
- [**nx (aarch64-switch-rs)**](https://github.com/aarch64-switch-rs/nx): Native Rust bindings.
- [**SkylandersNFC**](https://github.com/skylandersNFC): Mifare figure format research.

---

## License

Emulanders is licensed under **GNU GPLv3**. See [LICENSE](LICENSE) for full details.

*Ryujinx project exemption: The Ryujinx team is exempt from GPLv3 licensing for this codebase.*