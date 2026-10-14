# Project Roadmap 🚀

This roadmap outlines the development objectives for the entire Emulanders ecosystem (Sysmodule, Tesla Overlay, and the Standalone App). Tasks are listed as independent goals and may be addressed in any order.

## 🟢 Completed Milestones
* **[COMPLETED] Figure Discovery & Asset Management:** Development of an automated identification tool that parses raw dump bytes to extract internal character IDs, enabling dynamic retrieval of corresponding 175x275 `.png` visual assets for the user's collection.
* **[COMPLETED] Persistent Progress (NFC Writing):** Safely mutated the `nfc:mf:u` interception sequence to physically save `VirtualSkylander` modifications back to the 1KB `.dump` SD Card format instantaneously without generating busy-wait threading or Data Aborts.
* **[COMPLETED] Physical NFC Scanning & Virtualization (Emulanders Portal):** Native Joy-Con NFC (`nfp:user`) reading, art detection, custom filename entry, and Mifare 1K raw dump saving directly to SD.
* **[COMPLETED] Sandboxed SD Directory Management (Emulanders Portal):** Controller and touch-friendly file browser locked to `/figures/` with Copy, Cut, Paste, Rename, and safe asynchronous recursive Deletion.
* **[COMPLETED] Screenshot to Asset Cropper (Emulanders Portal):** Album browser, 16:9 previews, and automated cropping utility generating properly formatted assets for the Tesla overlay.
* **[COMPLETED] OTA Downloaders (Emulanders Portal):** In-app asset pack and figure dump downloaders with real-time progress bars and ZIP extraction.
* **[COMPLETED] Embedded Binary Assets (Emulanders Portal):** Zero-dependency `bin2o` pipeline embedding UI icons directly into the `.nro` executable.
* **[COMPLETED] Figure Cloning Engine (Emulanders Portal Phase 4):** 1:1 exact figure cloning with adjacent art replication, and New Identity cloning with UID randomization, sector trailer Key A calculation, Header CRC16 recalculation, data decryption/re-encryption, and Creation Crystal / Imaginators Battle Class warning dialog.
* **[COMPLETED] Visual Previews & Lazy Thumbnail Loading (Emulanders Portal Phase 4):** Per-frame throttled thumbnail rendering (22x34) with local art & fallback asset database discovery, uniform 36px table row styling, and `DUMP` tag recognition.
* **[COMPLETED] Settings Persistence & System Dashboard (Emulanders Portal Phase 4):** Configuration persistence via `sdmc:/emulanders/config.json`, 7 color palette presets (Cyan, Portal Blue, Neon Purple, Emerald, Amber, Crimson, Hot Pink), RGB fine-tuning sliders, interactive color square with popup hue wheel, and live IPC sysmodule status dashboard.
* **[COMPLETED] Complete Bilingual Localization (EN & PT-BR):** Native support for English and Brazilian Portuguese across both Emulanders Portal and the Tesla Overlay (`emulanders.ovl`). Includes automatic Switch OS system language detection, full UI text translation with diacritics / Latin-1 glyphs, and runtime switcher in Settings.
* **[COMPLETED] Interactive Crop Tool & Switch Album Integration (Emulanders Portal):** Interactive crop tool with D-pad & stick live crop frame adjustment, real-time live preview, Switch Album root / latest photos shortcut navigation, 1:1 direct touch dragging, and instant thumbnail cache invalidation upon saving.
* **[COMPLETED] Docked Controller Navigation & Gamepad Shortcuts (Emulanders Portal):** Direct button bindings (`[Y]` Favorites, `[L]`, `[R]`, `[X]`), elimination of `NavLayer_Menu` entrapment on button Y, smooth right-stick scrolling in docked mode, and resilient Home Menu cursor retention.

---

## 🛠️ Sysmodule & Tesla Overlay (Core)

### 🟢 Completed Community Localization
Brazilian Portuguese and English are fully translated and verified for the Tesla Overlay (`emulanders.ovl`) and Emulanders Portal standalone manager. Remaining languages gracefully fall back to English.

---

## 🌟 Emulanders Portal (Standalone Homebrew App)
*A dedicated full-screen application providing a safe environment for heavy collection management. Runs as a standard Homebrew App (`.nro`) with Applet Mode compatibility.*

### 🔨 Save Data Editor (Runes Port - Phase 5)
*(Future Goal)* An interface to parse and modify the internal save blocks of a `.dump` file (e.g., Gold, XP, Nicknames, Upgrade Paths). Requires deep implementation of Skylanders' specific CRC checksum algorithms to prevent data corruption. Based on the architectural reverse-engineering from the `Runes` auxiliary repository.

---

## 📦 Distribution & Packaging
Preparation of project metadata (`.nro`, icons, and release structures) for official submission to Homebrew App Store. Automated deployment script `promote_portal.ps1` enables instant on-device testing via MTP.

---

## ❌ Scrapped Objectives (Design Triage)

### 🔨 Auto-Dismount Timer
*Removed:* Originally planned to aggressively dismount idle figures due to extreme IPC polling lags (Data Aborts). Since the code was successfully refactored into a passive architecture with zero-cost RAM interactions, a mounted virtual figure consumes precisely 1KB of RAM and 0% CPU. Introducing a worker thread purely to count 60 idle seconds contradicts our minimalist vision.

### 🔨 Panic Handling & Resilience
*Removed:* Dumping custom Crash Logs overcomplicates the module. Atmosphère inherently catches any kernel disruptions and generates extensive baremetal crash dumps on its own. Re-implementing a fatal error interceptor inside our own module would be redundant feature creep.

### 🔨 Transparent Proxy (No-Reboot Toggle)
*Removed:* Decided against implementing a forced raw IPC kernel relay. The game already provides a native Sync-to-Toy NPC menu for copying digital saves back to physical toys, and playing with physical toys is most safely done with Emulation fully disabled (via game reboot).