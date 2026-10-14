# 🚀 Emulanders 1.0.0: The Complete Suite

We are thrilled to officially announce **Emulanders 1.0.0**! 

What began as a low-level Mifare sysmodule and Tesla overlay has grown into a comprehensive, standalone ecosystem for preserving, managing, and emulating Skylanders collections natively on the Nintendo Switch.

<div align="center">
  <video src="https://github.com/user-attachments/assets/7e61b20a-e117-4c3a-9585-9310cc56cbc0" autoplay="autoplay" loop="loop" muted="muted" playsinline width="480"></video>
  <br>
  <em><a href="https://youtu.be/oYJi1xWd5xI">Watch the Video Demonstration on YouTube</a></em>
</div>

---

## 🌟 What's New in 1.0.0

### 📱 Emulanders Portal (Standalone App)
The highlight of 1.0.0 is **Emulanders Portal** (`sd:/switch/EmulandersPortal/EmulandersPortal.nro`), a full-screen homebrew application that lets you manage your entire collection without ever needing a PC:

- **Joy-Con Physical NFC Dumper:** Place any physical Skylander on the Right Joy-Con analog stick (or Pro Controller NFC reader). The app communicates with `nfp:user` to read the Mifare Classic 1K tag, identifies the Character and Variant IDs, displays official artwork, and dumps a clean 1KB `.dump` file to SD with a single button press.
- **Cryptographic Figure Cloning Engine:** 
  - **1:1 Clone:** Exact byte-for-byte duplication for quick backups.
  - **New Identity Clone:** Generates a new unique 4-byte Mifare UID, recalculates Sector 0 BCC, derives all 16 sector trailer Key A values using Mifare cryptographic algorithms, decrypts and re-encrypts figure data, and recalculates the Header CRC16. This eliminates in-game duplicate UID errors and lets you use multiple identical figures or level duplicate Creation Crystals independently!
- **Interactive Screenshot Cropper:** Directly integrates with your Nintendo Switch Album (`sdmc:/Nintendo/Album/` and emuMMC). Browse full 16:9 game captures, use D-Pad / left stick / touchscreen to pan and zoom, and crop directly to the standard 175×275 vertical portrait format.
- **Over-The-Air (OTA) Downloaders:** Built-in downloaders to fetch community figure archives and the complete portrait asset pack directly over Wi-Fi.
- **Themes & UI Customization:** 7 built-in color themes (Emulanders Blue, Portal Purple, Magma Red, Life Green, Tech Orange, Undead Violet, Dark Void), RGB fine-tuning sliders, and an interactive color palette picker.
- **Full Bilingual Localization:** Complete native support for English and Brazilian Portuguese (PT-BR) across the entire application and overlay, with automatic system language detection.

### 🎮 Tesla Overlay & Core Sysmodule
- **In-Game Hot-Swapping:** Mount, switch, and unmount figures on-the-fly inside *Skylanders: Imaginators* via the Tesla overlay (**L + D-Pad Down + R3**).
- **Optimized Memory Heap:** Lazy-loading thumbnail engine designed for minimal memory footprint to prevent out-of-memory errors during intensive game scenes.
- **Mifare Protocol Emulation:** Background sysmodule intercepting `nfc:mf:u` service calls, coexisting seamlessly alongside Amiibo emulators (like Emuiibo) without conflict.

---

## 📦 Downloads & Packages

| Package | Recommended For | Contents |
|:--|:--|:--|
| **`Emulanders-Bundle-1.0.0.zip`** | **⭐ All Users** | Complete suite: Sysmodule (`atmosphere/`) + Tesla Overlay (`switch/.overlays/`) + Emulanders Portal (`switch/EmulandersPortal/`) + languages |
| **`Emulanders-Portal-1.0.0.zip`** | Portal Standalone | Just the Emulanders Portal application (`switch/EmulandersPortal/`) |
| **`Emulanders-Core-1.0.0.zip`** | Sysmodule & Overlay | Background sysmodule (`exefs.nsp`) and Tesla overlay (`emulanders.ovl`) only |
| **`emulanders_assets_pack.zip`** | Shared Asset Library | Pre-made portraits for every known Skylander, variant, and accessory across all generations |

---

## 🚀 Quick Start Installation

1. Download **`Emulanders-Bundle-1.0.0.zip`** and extract all folders (`atmosphere`, `switch`, `emulanders`) directly to the root of your SD card.
2. (Optional but recommended) Download **`emulanders_assets_pack.zip`** and extract to the root of your SD card to get instant portrait art for all characters.
3. Restart your Nintendo Switch.
4. Launch **Emulanders Portal** from the Homebrew Menu (`hbmenu`) to manage, dump, and clone your collection.
5. In-game, open the Tesla Overlay to mount and play with your figures!

---

## ❤️ Credits & Acknowledgments

- **XorTroll & Emuiibo Contributors** (*Subv, ogniK, averne, spx01, SciresM*): For the foundational sysmodule architecture and research.
- **Tesla Overlay Developers** (*AD2076, AmonRaNet*): For the overlay framework.
- **Switchbrew & aarch64-switch-rs team**: For libnx and the Rust bindings.
- **Skylanders Community**: For keeping the magic of the game alive!

*@joaohypo*
