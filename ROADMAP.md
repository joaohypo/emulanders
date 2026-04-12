# Project Roadmap 🚀

This roadmap outlines the development objectives for Emulanders. Tasks are listed as independent goals and may be addressed in any order.

### 🔨 Transparent Proxy (No-Reboot Toggle)
Implementation of an IPC relay mechanism where the sysmodule maintains a constant interception of the `nfc:mf:u` service. When emulation is disabled, the module will function as a transparent bridge, forwarding game commands to the physical USB Portal. This enables switching between physical and virtual figures without requiring a game or console restart.

### 🔨 Persistent Progress (NFC Writing)
Expansion of the current Mifare `write` stub to support full data persistence. This feature will enable the game to save character progression (experience, currency, items) directly back to the `.dump` files on the SD card or, optionally, to physical Mifare tags.

### 🔨 In-Game Figure Cloning
Integration of a cloning utility within the Tesla Overlay. This feature will allow the duplication of active `.dump` files directly on the console, facilitating multiple progression branches for a single character identity.

### 🔨 Figure Discovery & Asset Management
Development of an automated identification tool that parses raw dump bytes to extract internal character IDs. This will enable automatic filename resolution and the dynamic retrieval of corresponding `.png` visual assets for the user's collection.

### 🔨 Auto-Dismount Timer
Implementation of a background worker thread within the sysmodule to automatically unmount virtual figures after a period of inactivity (e.g., 60 seconds). This objective aims to reduce background IPC polling overhead and maintain cleaner debug logs.

### 🔨 Panic Handling & Resilience
Implementation of a custom panic hook to flush the RAM debug buffer to the SD card (`last_crash_log.txt`) upon fatal execution errors. This is intended to facilitate community-driven bug reporting and diagnostics.

### 🔨 Community Localization
Coordination of native translations for all supported languages within the Tesla Overlay to ensure linguistic accuracy and professional presentation.

### 🔨 Homebrew Distribution
Preparation of project metadata and structure for official submission to the Homebrew Details and HBMenu application stores.

---

### 💡 Experimental Concepts
- **Network-Based Manager:** A local web dashboard for remote management of the figures directory.
- **Status HUD:** Real-time display of character statistics (Level/Gold) within the overlay.
