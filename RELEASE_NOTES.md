# Emulanders 1.0.0

Initial release of the Emulanders suite for Nintendo Switch, featuring the standalone Emulanders Portal manager alongside the background sysmodule and Tesla overlay.

<div align="center">
  <video src="https://github.com/user-attachments/assets/7e61b20a-e117-4c3a-9585-9310cc56cbc0" autoplay="autoplay" loop="loop" muted="muted" playsinline width="480"></video>
  <br>
  <em><a href="https://youtu.be/oYJi1xWd5xI">Video demonstration on YouTube</a></em>
</div>

---

## What's Included

### Emulanders Portal (`EmulandersPortal.nro`)
Standalone homebrew application for managing your collection directly on the console:

- **Joy-Con NFC Dumper**: Dump physical Skylanders tags using the Right Joy-Con or Pro Controller NFC reader directly to `sdmc:/emulanders/figures/`.
- **Figure Cloning**:
  - 1:1 duplicate for simple backups.
  - New Identity cloning: generates a new UID, rederives sector Key A values, re-encrypts data blocks, and recalculates CRC16 checksums so duplicate figures can be used simultaneously without in-game collisions.
- **Screenshot Cropper**: Load 16:9 console captures from the Switch Album (`Nintendo/Album`) and crop them to 175×275 figure portraits (`figure.png`).
- **OTA Downloaders**: Download community figure dump sets and asset packs over Wi-Fi.
- **Themes & Languages**: 7 built-in themes, custom RGB color picker, and native English / Portuguese (PT-BR) support.

### Tesla Overlay (`emulanders.ovl`) & Sysmodule
- In-game figure mounting, unmounting, and hot-swapping (`L + D-Pad Down + R3`).
- Mifare Classic 1K protocol emulation intercepting `nfc:mf:u`.
- Low memory footprint with lazy thumbnail rendering.
- Operates alongside Amiibo tools (Emuiibo) without conflict.

---

## Packages

| File | Target | Contents |
|:--|:--|:--|
| **`Emulanders-Bundle-1.0.0.zip`** | Recommended | Complete suite: Sysmodule (`atmosphere/`), Tesla Overlay (`switch/.overlays/`), Portal app (`switch/EmulandersPortal/`), and language files |
| **`Emulanders-Portal-1.0.0.zip`** | Standalone App | Emulanders Portal application only (`switch/EmulandersPortal/`) |
| **`Emulanders-Core-1.0.0.zip`** | Sysmodule & Overlay | Background sysmodule (`exefs.nsp`) and Tesla overlay (`emulanders.ovl`) |
| **`emulanders_assets_pack.zip`** | Asset Library | Ready-to-use portrait pack for all characters and variants |

---

## Installation
1. Extract `Emulanders-Bundle-1.0.0.zip` to the root of your SD card.
2. *(Optional)* Extract `emulanders_assets_pack.zip` to the SD root for character portraits.
3. Restart the console.
4. Manage figures with **Emulanders Portal** from the Homebrew Menu, and mount them in-game via the Tesla overlay.

---

## Credits
- **XorTroll & Emuiibo Contributors** (*Subv, ogniK, averne, spx01, SciresM*): Original sysmodule architecture and nfp-mitm research.
- **libtesla Developers** (*AD2076, AmonRaNet*): Overlay framework.
- **Switchbrew & aarch64-switch-rs team**: libnx and Rust Switch bindings.
- **Skylanders Community**: Research into figure structures and preservation.
