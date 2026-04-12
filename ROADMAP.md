# Emulanders Project Roadmap 🚀

The following represents the ongoing and future development goals for Emulanders. These tasks are isolated objectives aimed at improving the functionality, user experience, and community reach of the project.

### 🔨 Transparent Proxy (No-Reboot Toggle)
Implement an IPC relay mechanism where the sysmodule always intercepts the `nfc:mf:u` service. When emulation is OFF, the sysmodule acts as a "Transparent Cable," forwarding all game commands to the real physical USB Portal. This would allow switching between physical and virtual figures without restarting the game or the console.

### 🔨 Persistent Progress (NFC Writing)
Currently, the Mifare `write` command is a stub. The goal is to implement full write support so that character progress (experience, gold, items) is saved back to the `.dump` files on the SD card in real-time. This also opens the possibility of using the sysmodule to write data to physical tags.

### 🔨 In-Game Figure Cloning
Add a "Clone Figure" function directly in the Tesla Overlay. This will allow users to duplicate a `.dump` file on the SD card with a single button press, enabling multiple independent progression paths for the same character.

### 🔨 Figure Discovery & Asset Downloader
Develop a tool (or integrate logic) to parse raw dump bytes and identify the character's internal ID. By mapping these IDs to a database, the system could automatically identify figures and download/generate the appropriate `.png` visual icons for the entire collection.

### 🔨 Auto-Dismount Timer (Optimization)
Implement a background worker thread in the Rust sysmodule to automatically unmount virtual figures after a period of inactivity (e.g., 60 seconds). This would significantly reduce CPU overhead and keep debug logs clean by stopping the game's constant polling once the character is loaded.

### 🔨 Production Polish & Panic Handling
Improve the sysmodule's resilience by implementing a "Panic Hook" that flushes the RAM debug buffer to the SD card (`last_crash_log.txt`) in the event of a fatal error. This ensures that even the most elusive bugs can be diagnosed by the community.

### 🔨 Community Localization
Gather native speaker contributions to finalize high-quality translations for all supported languages in the Tesla Overlay, ensuring a polished experience for users worldwide.

### 🔨 Homebrew App Store Distribution
Prepare the project structure and metadata for official submission to the Homebrew Details and HBMenu stores, making Emulanders easily accessible to the wider Switch homebrew community.

---

### 💡 Research & Experimental Ideas
- **Web-Based Collection Manager:** A local network dashboard to manage, rename, and backup figures via a browser.
- **Save File Editor Integration:** Basic in-game stat viewing (Level/Gold) directly within the Tesla Overlay.
