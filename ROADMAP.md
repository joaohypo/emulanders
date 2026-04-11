# Emulanders Project Roadmap 🚀

The following points represent the planned objectives and areas of development for Emulanders.

### 🔨 The "Transparent Proxy"
Currently, Emulanders decides whether to intercept the `nfc:mf:u` service at the moment the game starts. To allow real-time switching between a physical USB Portal and the emulator:
- **Always-on Interception:** The sysmodule will always intercept the service call.
- **Smart Relay:** When emulation is turned **OFF**, the sysmodule will act as a "Transparent Proxy," forwarding all game commands to the physical hardware.
- **Seamless Handover:** When emulation is turned **ON**, it will stop relaying and serve data from the SD card.

### 🔨 In-Game Management & Cloning
Manage multiple progression paths for the same figure without external tools:
- **File-Based Cloning:** Add a "Clone Figure" option in the Overlay to duplicate `.dump` files, allowing for separate leveling paths via the in-game "Reset" feature.
- **Automatic Backups:** Create timestamped backups of `.dump` files before the game performs write operations.

### 🔨 Production & Community
- **Community Translations:** Crowdsource native translations for all supported languages in the Tesla Overlay.
- **Panic Handling:** Implement a crash-handler to flush the RAM log to the SD card upon fatal errors.
- **Homebrew App Store:** Prepare and submit Emulanders to the Homebrew Details and HBMenu stores for easier distribution.

---

### 💡 Future Ideas
- **Auto-Dismount Timer (PoC):** Implement a dedicated background thread in the Rust sysmodule to automatically unmount the active Skylander after a set duration (e.g., 60 seconds). Since the game constantly polls the virtual portal with IPC read requests while a figure is active, an auto-dismount feature would significantly reduce CPU overhead and keep the IPC debug logs clean.
- **Web Dashboard:** A local web interface to manage the figures folder over the network.
- **Automatic ID Identification:** Identify the Skylander's identity from raw dump bytes to display their real name in the UI.
