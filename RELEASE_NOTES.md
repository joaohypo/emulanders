# Emulanders 1.0.1

Maintenance release. No change to the sysmodule's behaviour; the main changes are in Emulanders Portal and the overlay translations.

## Changes

### Emulanders Portal
- Language support expanded from 2 to 10: English, Portuguese (Brazil), Spanish, French, German, Italian, Russian, Korean, Simplified Chinese and Traditional Chinese. The selection is stored in `sdmc:/emulanders/config.json` and shared with the overlay.
- Buttons now size themselves to their label, and the language and theme lists wrap onto a new row, so longer translations no longer overflow.
- Cyrillic and CJK text is rendered with the console's shared system fonts.
- Figure Image Editor: the `Figures / Scans / Favorites` and `Menu / Back` buttons are on a single row, and the step header is one line instead of three. This gives the figure and photo lists roughly 95 px of extra height (about seven visible rows instead of four).
- Figure Image Editor (Step 3 Crop Tool): tuned live preview sizing and button heights so `(B) Back to Photos` has generous vertical clearance and is never clipped by the bottom status bar in 720p handheld mode.
- Download failures (no network, bad URL) are reported instead of silently extracting an empty archive.
- Fixed controller hot-plugging when docking and undocking, and resizing between 720p and 1080p.

### Tesla overlay
- All 10 languages are fully translated (previously only English and Portuguese were).
- Reads the language chosen in the Portal; falls back to the system language otherwise.
- Fixed the version-mismatch warning, which was being overwritten and never shown.

### Sysmodule
- Fixed a possible panic when the debug log was trimmed in the middle of a multi-byte character (for example accented figure names).

## Packages

| File | Contents |
|:--|:--|
| `Emulanders-Bundle-1.0.1.zip` | Everything: sysmodule, overlay, Portal and language files. Extract to the SD card root. |
| `Emulanders-Portal-1.0.1.zip` | Portal only (`switch/EmulandersPortal/`). |
| `Emulanders-Core-1.0.1.zip` | Sysmodule and overlay only. |
| `emulanders_assets_pack.zip` | Portrait pack (unchanged, from 1.0.0). |

## Upgrading from 1.0.0
Extract the bundle over the existing install and restart the console. Figures, assets and settings are kept. The sysmodule and overlay must be updated together; the overlay warns if their versions differ.

## Compatibility
Fully verified and tested on **Firmware 23.0.1 with Atmosphère 1.12.1** (CNX 23.0.1-1) and Firmware 22.1 with Atmosphère 1.11.1. Full hardware validation confirmed for `nfc:mf:u` protocol emulation, physical Joy-Con Mifare tag dumping, Tesla Overlay hot-swapping, and in-game figure write-back. Compatible with all firmwares 4.0.0+.

## Credits
- XorTroll and Emuiibo contributors (Subv, ogniK, averne, spx01, SciresM): original sysmodule architecture and `nfp-mitm` research.
- libtesla developers (AD2076, AmonRaNet): overlay framework.
- Switchbrew and the aarch64-switch-rs team: libnx and the Rust bindings.
- Skylanders community: research on the figure format.
