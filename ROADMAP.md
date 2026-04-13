# Project Roadmap 🚀

This roadmap outlines the development objectives for the entire Emulanders ecosystem (Sysmodule, Tesla Overlay, and the Standalone App). Tasks are listed as independent goals and may be addressed in any order.

## 🟢 Completed Milestones
* **[COMPLETED] Figure Discovery & Asset Management:** Development of an automated identification tool that parses raw dump bytes to extract internal character IDs, enabling dynamic retrieval of corresponding 175x275 `.png` visual assets for the user's collection.

---

## 🛠️ Sysmodule & Tesla Overlay (Core)

### 🔨 Persistent Progress (NFC Writing)
Expansion of the current Mifare `write` stub within the Rust sysmodule to support full data persistence. This feature will enable the game to save character progression (experience, currency, items) directly back to the `.dump` files on the SD card in real-time.

### 🔨 Transparent Proxy (No-Reboot Toggle)
Implementation of an IPC relay mechanism where the sysmodule maintains a constant interception of the `nfc:mf:u` service. When emulation is disabled, the module will function as a transparent bridge, forwarding game commands to the physical USB Portal. This enables switching between physical and virtual figures without requiring a game or console restart.

### 🔨 Auto-Dismount Timer
Implementation of a background worker thread within the sysmodule to automatically unmount virtual figures after a period of inactivity (e.g., 60 seconds). This objective aims to reduce background IPC polling overhead and clear the "Figure on Portal" UI from the game screen.

### 🔨 Community Localization
Coordination of native translations for all 10 supported languages within the Tesla Overlay to ensure linguistic accuracy. Currently, only English and Brazilian Portuguese are fully translated, with the remaining languages temporarily falling back to English.

### 🔨 Panic Handling & Resilience
Implementation of a custom panic hook to flush the RAM debug buffer to the SD card (`last_crash_log.txt`) upon fatal execution errors. This is intended to facilitate community-driven bug reporting and diagnostics.

---

## 🌟 PortalMasterNX (Standalone Homebrew App)
*A dedicated full-screen application providing a safe environment for heavy collection management. While designed as a standard Homebrew App, efforts will be made to ensure **Applet Mode compatibility** so it can run while the game remains suspended in the background.*

### 🔨 Visual Collection Gallery
A full-screen, visually rich interface built with the Borealis UI framework. It consumes the generated `/emulanders/assets/` directory to display the user's entire SD card collection as a digital shelf. Allows the user to select the active Skylander for the sysmodule.

### 🔨 Smart Figure Cloning
A utility to duplicate an existing `.dump` file on the SD card while intelligently randomizing its Block 0 Serial ID (UID) and updating the internal cryptography checksums. This allows the game to recognize the clone as a physically distinct figure, enabling multiple upgrade paths for the same character.

### 🔨 Save Data Editor
An interface to parse and modify the internal save blocks of a `.dump` file (e.g., Gold, XP, Nicknames, Upgrade Paths). Requires deep implementation of Skylanders' specific CRC checksum algorithms to prevent data corruption.

### 🔨 Physical NFC Dumping (Scanning)
Utilization of the Switch's native `nfp:user` API (Right Joy-Con) to scan physical Skylander toys and dump their raw Mifare 1K block data directly to the SD card as a `.dump` file for digital preservation.

### 🔨 Physical NFC Writing (Restoring)
The ability to select a `.dump` from the SD card and write it back into a blank, compatible NFC tag (Magic Tag with writable Block 0), effectively creating a playable physical backup of a digital figure.

---

### 📦 Homebrew Distribution
Preparation of project metadata (`.nro`, icons, and release structures) for official submission to the Homebrew Details and HBMenu application stores.