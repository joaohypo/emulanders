# Emulanders Project Roadmap 🚀

This document outlines the planned evolution of Emulanders, transitioning from its current beta state to a fully-featured, production-grade sysmodule.

---

## ✅ Phase 1: Foundation & Cleanup (Completed)
- Severed ties with legacy Amiibo/NTAG logic.
- Established a dedicated repository with a clean-room legal narrative.
- Standardized nomenclature around Skylanders and Mifare Classic.

## ✅ Phase 2: User Experience & Stability (Completed)
- Redesigned the Tesla Overlay for a streamlined Skylanders catalog.
- Implemented a RAM-based **Logs Manager** with a dynamic toggle to save CPU/RAM.
- Resolved directory parsing stability for large figure collections.
- Unified the "Favorites" system with a single-button toggle (Y).

## 🔨 Phase 3: The "Transparent Proxy" (Future / Research Complete)
*Goal: Enable/Disable emulation without rebooting the console or restarting the game.*

### The Concept
Currently, Emulanders decides whether to intercept the `nfc:mf:u` service at the moment the game starts. To allow real-time switching between a physical USB Portal and Emulanders:
- **Always-on Interception:** The sysmodule will always intercept the service call.
- **Smart Relay:** When emulation is turned **OFF**, the sysmodule will act as a "Transparent Proxy," opening its own connection to the real Nintendo NFC driver and forwarding all game commands to the physical hardware.
- **Seamless Handover:** When emulation is turned **ON**, it stop relaying and starts serving data from the SD card `.dump` files.

## 🔨 Phase 4: In-Game Management & Cloning
*Goal: Manage multiple progression paths for the same physical figure.*

- **File-Based Cloning:** Add a "Clone Figure" option in the Overlay. This will duplicate the `.dump` file on the SD card, allowing the user to use the in-game "Reset" feature on the copy to start a new leveling path without losing the original save.
- **Automatic Backups:** Optionally create a timestamped backup of a `.dump` file before the game writes new data to it.

## 🔨 Phase 5: Production & Community
- **Localization:** Gather community contributions to finalize translations for all supported languages.
- **Panic Handling:** Implement a crash-handler that flushes the RAM log to the SD card if the sysmodule encounters a fatal error.
- **Public Release:** Transition from `v0.9.x` to `v1.0.0` and publish to homebrew repositories.

---

## 💡 Future Ideas
- **Web Dashboard:** A local web interface (via a separate sysmodule) to manage the figures folder over the network.
- **Automatic ID Identification:** Attempt to identify the Skylander name from the raw dump bytes to show a "Real Name" in the UI even if the filename is generic.
