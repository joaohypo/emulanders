# Project Roadmap

This roadmap outlines development milestones and planned features across the Emulanders ecosystem (Sysmodule, Tesla Overlay, and Emulanders Portal).

---

## Completed Milestones

- **Figure Discovery & Asset Management**: Automated parser extracting character IDs from dump bytes to resolve corresponding 175×275 portrait assets.
- **NFC Writing & Save Persistence**: Full `nfc:mf:u` interception writing modifications back to 1KB `.dump` files on SD without blocking threads or causing aborts.
- **Physical NFC Dumper (Portal)**: Native Joy-Con and Pro Controller NFC (`nfp:user`) reading, art detection, custom filename entry, and Mifare 1K raw dump saving.
- **Sandboxed SD File Management (Portal)**: Controller- and touch-friendly file browser locked to `/figures/` with copy, cut, paste, rename, and recursive deletion.
- **Screenshot to Asset Cropper (Portal)**: Album browser, 16:9 previews, and interactive cropping tool generating portraits for the Tesla overlay.
- **OTA Downloaders (Portal)**: In-app asset pack and figure dump downloaders with real-time extraction.
- **Embedded Binary Assets (Portal)**: Zero-dependency pipeline embedding UI icons directly into the `.nro` executable.
- **Figure Cloning Engine (Portal)**: 1:1 exact figure cloning and New Identity cloning with UID randomization, sector trailer Key A calculation, Header CRC16 recalculation, and data re-encryption.
- **Lazy Thumbnail Rendering (Portal & Overlay)**: Throttled thumbnail loading with low memory footprint to avoid out-of-memory errors during gameplay.
- **Settings Persistence & Themes (Portal)**: Configuration stored in `sdmc:/emulanders/config.json`, 7 color presets, RGB fine-tuning sliders, and live sysmodule IPC status dashboard.
- **Bilingual Localization (EN & PT-BR)**: Native English and Brazilian Portuguese support across both Portal and Tesla Overlay with system language detection and runtime switching.
- **Docked Navigation & Controller Bindings (Portal)**: Full gamepad support (`[Y]` Favorites, `[L]`/`[R]`, `[X]`), smooth right-stick scrolling in docked mode, and capacitive multi-touch support.

---

## Planned Features

### Save Data Editor (Runes Port - Phase 5)
*(Future Goal)* An on-device interface to inspect and modify internal save blocks of a `.dump` file (Gold, XP, Nicknames, Upgrade Paths). Requires implementing game-specific checksum algorithms to ensure save validity.

---

## Scrapped Objectives

- **Auto-Dismount Timer**: Originally considered to unmount figures after inactivity. The sysmodule was refactored into a passive architecture with zero-cost RAM caching, consuming 1KB RAM and 0% CPU when idle. An extra timer thread is unnecessary.
- **Custom Panic Handler**: Atmosphère catches system-level disruptions and generates complete crash dumps. Adding a custom interceptor would add unnecessary complexity.
- **Transparent Proxy (No-Reboot Toggle)**: A forced raw IPC kernel relay was evaluated, but the game already provides a native Sync-to-Toy NPC menu for copying digital saves back to physical toys, and playing with physical toys is safest with emulation disabled.