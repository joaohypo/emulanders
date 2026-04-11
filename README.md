<p align="center">
  <img alt="emulanders" src="res/banner_cut.png" width="777">
</p>

<p align="center">
  <strong>Emulanders: Skylanders NFC (Mifare) emulation system for the Nintendo Switch</strong>
</p>

## ⚠️ Disclaimer & Legal Narrative

**This is a clean-room reverse engineering project for educational and archival purposes.**
Emulanders does **not** provide copyrighted material, proprietary encryption keys, Nintendo SDK code, or Skylander `.dump`/`.bin` files. Users are strictly expected to use dumps extracted from their own legally purchased, physical toys.

---

## 📖 About Emulanders

Emulanders is a custom background sysmodule (and Tesla Overlay) for Atmosphère that allows you to load and hot-swap Skylanders figures directly from your SD card into *Skylanders: Imaginators*.

For the current development status and future plans, please see the [Project Roadmap](ROADMAP.md).

### The Technical Breakthrough
Unlike standard Nintendo Amiibos that use NTAG formats and communicate via the `nfp` (Nintendo Figurine Platform) service, Skylanders portals and characters utilize NXP Mifare Classic 1K tags. *Imaginators* completely bypasses the standard Amiibo parser stack and communicates directly with the low-level **`nfc:mf:u`** (Mifare User) IPC service.

Emulanders functions as a protocol bridge for the `nfc:mf:u` interface. It provides a virtualized data path that remains fully compliant with the system's official communication standards, allowing the game engine to retrieve character information from local digital backups on the SD card.

---

## 🚀 Core Features

- **Virtualized Mifare Bridge:** Full emulation of the `nfc:mf:u` protocol used by Skylanders figurines and portals.
- **Real-time Hot-swapping:** Change characters instantly via the Tesla Overlay without leaving the game.
- **Visual Identification:** Support for `.png` preview images to easily identify figures in the menu.
- **State Persistence:** Remembers your emulation status (ON/OFF) and active figure across console reboots.
- **Harmony with Amiibo:** Designed to run alongside Amiibo emulators (like Emuiibo) without service conflicts.
- **Optimized Performance:** RAM-based circular log buffer and on-demand image loading to preserve system resources.

---

## 📂 Installation & Directory Structure

You will need the following files from a compiled release:
- **Sysmodule**: `sd:/atmosphere/contents/420000000000E311/exefs.nsp`
- **Tesla Overlay**: `sd:/switch/.overlays/emulanders.ovl`

### The SD Card Layout
Emulanders uses the `sdmc:/emulanders/` directory at the root of your SD card.

1. **`sdmc:/emulanders/figures/` (Your Skylanders)**
   Place your raw Skylander `.dump` or `.bin` files here. Although **Skylanders Imaginators** is the only title in the franchise available on the Nintendo Switch, you can still organize your collection into subfolders (e.g., by Element or Series) for easier navigation.
   *Example:* `sd:/emulanders/figures/Senseis/King_Pen.dump`

2. **`sdmc:/emulanders/overlay/lang/` (Localization)**
   Contains JSON files for all supported languages. The overlay automatically matches your Switch's system language.

3. **`sdmc:/emulanders/flags/` (State Persistence)**
   Automatically managed folder that preserves your emulation settings across reboots.

---

## 🎮 Usage & Tips

To use Emulanders in-game:
1. Open the Tesla menu (usually **L + D-Pad Down + Right Stick Click**).
2. Select **emulanders** and **Turn Emulation ON**.
3. Navigate to **View Figures Folder** and select a Skylander figure.
4. The overlay will show `>> ACTIVE`. The sysmodule will fire the `TagFound` event, and the character will spawn!
5. **To change characters:** Select a different figure. The system handles the swap automatically.
6. **To remove a character:** Select the active figure again or use the **Clear active Skylander** option.

### 💡 Pro Tips
- **Resource Management:** Once a character is fully loaded, it is recommended to **Clear active Skylander**. This stops high-frequency IPC polling and saves CPU/Battery.
- **Visual Icons:** To see character portraits in the menu, place a `.png` file with the exact same name as your `.dump` file in the same folder. *(Recommended size: ~150x200px).*
- **Swap Force Compatibility:** Skylanders Imaginators on Switch supports Swap Force figures but does **not** support the "mixing parts" mechanic. To load a Swap Force character, you only need to select a single `.dump` file (either the top or bottom half); the game will load the full character automatically.

---

## 🤝 Parallel Operation
Emulanders is designed to work harmoniously alongside other NFC utilities, including Amiibo emulators like **Emuiibo**. 
- **Distinct Domains:** Emuiibo handles `nfp` (Amiibo), while Emulanders handles `nfc:mf:u` (Mifare).
- **Simultaneous Use:** You can keep both installed. They do not compete for handles, as they operate on entirely different logical interfaces.

---

## 🐛 Troubleshooting & Debug Logging
If a specific `.dump` fails or the game acts unexpectedly:
1. Open the overlay and go to **Logs Manager**.
2. Toggle **Debug Logging** to `On`.
3. Reproduce the bug in-game.
4. Select **Extract to SD** to save the log to `sdmc:/emulanders/debug_log_dump.txt`.
5. Attach this file when opening an issue on GitHub.

---

## 🛠️ Compiling
Requires [Rust for Nintendo Switch](https://github.com/aarch64-switch-rs/setup-guide) and devkitPro (devkitA64).
Clone the repo and run `make dist` for release or `make dist-dev` for debug.

---

## ❤️ Credits & Acknowledgments

**Emulanders was originally born from the incredible work of XorTroll and the Emuiibo project.**
While Emulanders has been completely refactored to facilitate the `nfc:mf:u` service for Skylanders (Mifare) instead of `nfp` (Amiibos), this project would not have been possible without the foundation laid by Emuiibo.

Special thanks to:
- **Switchbrew**: For extensive documentation on Switch IPC services.
- **nx (aarch64-switch-rs)**: For the Rust bindings.
- **libtesla / nx-ovlloader**: For the overlay framework.
- **SkylandersNFC GitHub**: For invaluable knowledge on Skylanders NFC structures.
- **The Skylanders Community**: For keeping the magic of the game alive!

---

## 📜 License
Licensed under GPLv2/GPLv3 where applicable. See the `LICENSE` file for more details.
*Note: The Ryujinx project/team is exempt from GPLv2 licensing.*