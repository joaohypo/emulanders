<p align="center">
  <img alt="emulanders" src="res/logo1.png" width="400">
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

### The Technical Breakthrough
Unlike standard Nintendo Amiibos that use NTAG formats and communicate via the `nfp` (Nintendo Figurine Platform) service, Skylanders portals and characters utilize NXP Mifare Classic 1K tags. *Imaginators* completely bypasses the standard Amiibo parser stack and communicates directly with the low-level **`nfc:mf:u`** (Mifare User) IPC service.

Emulanders works by performing a Man-in-the-Middle (MitM) attack on the `nfc:mf:u` interface. It spoofs the official Switch driver, intercepts the game's low-level block read/write requests, and natively serves raw encrypted blocks from your `.dump` files on the SD card back to the game engine without causing Kernel Panics.

### Heritage & Credits
*Emulanders was originally born from the incredible work of XorTroll and the Emuiibo project.*
While Emulanders has been completely refactored to intercept the `nfc:mf:u` service for Skylanders (Mifare) instead of `nfp` (Amiibos), this project would not have been possible without the foundation laid by Emuiibo.

Special thanks to the open-source libraries and documentation that made this possible:
- [**Switchbrew**](https://switchbrew.org/): For extensive documentation on Switch IPC services (`nfc:mf:u`).
- [**nx (aarch64-switch-rs)**](https://github.com/aarch64-switch-rs/nx): The Rust bindings used to build the safe, native sysmodule.
- [**libtesla / nx-ovlloader**](https://github.com/WerWolv/libtesla): The C++ UI framework used to power the overlay.

---

## 📂 Installation & Directory Structure

You will need the following files from a compiled release:
- **Sysmodule**: `sd:/atmosphere/contents/<TitleID>/exefs.nsp`
- **Tesla Overlay**: `sd:/switch/.overlays/emulanders.ovl`

### The SD Card Layout
Emulanders uses the `sdmc:/emulanders/` directory at the root of your SD card.

1. **`sdmc:/emulanders/skylanders/` (Your Figures)**
   Place your raw Skylander `.dump` or `.bin` files here. You can organize them into subfolders (e.g., by Element or Game).
   *Example:* `sd:/emulanders/skylanders/Magic/Spyro.dump`

2. **`sdmc:/emulanders/flags/` (State Persistence)**
   This folder is automatically managed by the sysmodule. It contains files like `status_on.flag`. If you enable emulation in the Tesla menu, this flag is created so that Emulanders remembers to stay ON even after you reboot your Switch.

---

## 🎮 Usage (Tesla Overlay)

To use Emulanders in-game:
1. Open the Tesla menu (usually **L + D-Pad Down + Right Stick Click**).
2. Select **emulanders**.
3. **Turn Emulation ON** (if it isn't already).
4. Navigate to your `skylanders` folder and select a figure.
5. The overlay will show `>> ACTIVE` next to the chosen figure. The sysmodule will automatically fire the `TagMounted` NFC state machine event, and the game will spawn your character!
6. To switch characters, simply open the menu and select a different one. The sysmodule handles the `Deactivate` and `Activate` events seamlessly.

---

## 🛠️ Compiling

In order to compile Emulanders you need to setup [Rust for Nintendo Switch development](https://github.com/aarch64-switch-rs/setup-guide). You'll also need devkitPro (devkitA64 specifically) to compile the C++ Tesla overlay.

With these requirements satisfied, simply clone this repo and hit `make` or `make dist-dev`.

---

## 📜 License

Emulanders is licensed under the same terms as the original Emuiibo project (GPLv2/GPLv3) where applicable. See the `LICENSE` file for more details.