# Project Roadmap 🚀

This roadmap outlines the development objectives for the entire Emulanders ecosystem (Sysmodule, Tesla Overlay, and the Standalone App). Tasks are listed as independent goals and may be addressed in any order.

## 🟢 Completed Milestones
* **[COMPLETED] Figure Discovery & Asset Management:** Development of an automated identification tool that parses raw dump bytes to extract internal character IDs, enabling dynamic retrieval of corresponding 175x275 `.png` visual assets for the user's collection.
* **[COMPLETED] Persistent Progress (NFC Writing):** Safely mutated the `nfc:mf:u` interception sequence to physically save `VirtualSkylander` modifications back to the 1KB `.dump` SD Card format instantaneously without generating busy-wait threading or Data Aborts.

---

## 🛠️ Sysmodule & Tesla Overlay (Core)




### 🔨 Community Localization
Coordination of native translations for all 10 supported languages within the Tesla Overlay to ensure linguistic accuracy. Currently, only English and Brazilian Portuguese are fully translated, with the remaining languages temporarily falling back to English.


---

## 🌟 Emulanders Portal (Standalone Homebrew App)
*A dedicated full-screen application providing a safe environment for heavy collection management. While designed as a standard Homebrew App, efforts will be made to ensure **Applet Mode compatibility** so it can run while the game remains suspended in the background.*

### 🔨 Physical NFC Scanning & Virtualization
Utilization of the Switch's native `nfp:user` API (Right Joy-Con) to scan physical Skylander toys. Once scanned, the app will display the corresponding visual asset, prompt the user for a custom filename, and dump the raw Mifare 1K block data directly to the SD card as a `.dump` file.

### 🔨 Figure Cloning Engine
- **1:1 Clone:** Duplicate an existing `.dump` file identical to the source, prompting for a new filename.
- **New Identity Clone:** Duplicate an existing `.dump` file but intelligently randomize its internal Block 0 Serial ID (UID) and update internal cryptography checksums. This allows the game to recognize the clone as a physically distinct figure, enabling multiple independent upgrade paths for the same base character.

### 🔨 SD Directory Management
A dedicated sub-menu strictly sandboxed to manage files within the `/figures/` directory. Enables creating folders, moving `.dump` files between folders, and deleting dumps safely via a controller-friendly UI.

### 🔨 Screenshot to Asset Cropper
An on-device image utility. Select a screenshot taken from the Nintendo Switch Album, crop it automatically to the native Emulanders Tesla Overlay resolution, and dynamically compound visual overlay elements (Element Icon, Class Icon [optional], and customized Name text) before saving it to `/emulanders/assets/`.

### 🔨 Asset OTA Downloader
A convenience feature for users who only have the basic sysmodule installed. Downloads a hardcoded `.zip` file containing all Skylander visual assets directly from GitHub, extracts its contents (`/emulanders/assets/`) to the root of the SD card, and automatically cleans up the `.zip` archive upon completion.

### 🔨 Save Data Editor (Runes Port)
*(Future Goal)* An interface to parse and modify the internal save blocks of a `.dump` file (e.g., Gold, XP, Nicknames, Upgrade Paths). Requires deep implementation of Skylanders' specific CRC checksum algorithms to prevent data corruption. Will be based heavily on the architectural reverse-engineering from the `Runes` auxiliary repository.

---

### 📦 Homebrew Distribution
Preparation of project metadata (`.nro`, icons, and release structures) for official submission to the Homebrew Details and HBMenu application stores.

---

## ❌ Scrapped Objectives (Design Triage)

### 🔨 Auto-Dismount Timer
*Removed:* Originally planned to aggressively dismount idle figures due to extreme IPC polling lags (Data Aborts). Since the code was successfully refactored into a passive architecture with zero-cost RAM interactions, a mounted virtual figure consumes precisely 1KB of RAM and 0% CPU. Introducing a worker thread purely to count 60 idle seconds contradicts our minimalist vision.

### 🔨 Panic Handling & Resilience
*Removed:* Dumping custom Crash Logs overcomplicates the module. Atmosphère inherently catches any kernel disruptions and generates extensive baremetal crash dumps on its own. Re-implementing a fatal error interceptor inside our own module would be redundant feature creep.

### 🔨 Transparent Proxy (No-Reboot Toggle)
*Removed:* Decided against implementing a forced raw IPC kernel relay. The game already provides a native Sync-to-Toy NPC menu for copying digital saves back to physical toys, and playing with physical toys is most safely done with Emulation fully disabled (via game reboot). Pursuing hot-swapping introduces extreme kernel instability risks with zero tangible benefit, risking physical figure corruption.