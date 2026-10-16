# Emulanders Engineering Wiki — Index

Welcome to the living engineering specification for **Emulanders**, the open-source Skylanders Portal of Power emulator and figure management suite for Nintendo Switch (Atmosphère).

This wiki follows the [Karpathy LLM Wiki pattern](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f) and is governed by the rules and invariants defined in [`AGENTS.md`](../AGENTS.md).

---

## Core Specifications

- **[Architecture & System Topology](architecture.md)** — Overview of the three-component architecture (`emulanders` Rust sysmodule, `overlay` C++ Tesla overlay, and `EmulandersPortal` C++ standalone application), inter-process communication (`emu` service), memory footprints, and the SD card filesystem contract (`sdmc:/emulanders/`).
- **[Mifare Classic 1K & Cryptography Specification](mifare-crypto-spec.md)** — Byte-level specification of 1024-byte Skylander `.dump` files, Sector 0 Toy ID / Variant ID layout, CRC-48 Key A derivation, CRC-16-CCITT Type I/II/III checksums, AES-128-ECB block encryption, AArch64 32-bit word invariants, and the Zero-Block Preservation rule.
- **[Horizon OS `nfc:mf:u` IPC Protocol](nfc-ipc-protocol.md)** — Complete 14-command specification of the `nfc:mf:u` MitM service (`IUserManager` & `IUser`), kernel event signaling (`AttachActivateEvent`, `AttachDeactivateEvent`, `AttachAvailabilityChangeEvent`), device state machine, and peaceful coexistence with `emuiibo`.
- **[UI, Display & Input Engineering (Switch 720p/1080p)](ui-display-and-input.md)** — Display geometry for Handheld (`1280×720`) and Docked (`1920×1080`) modes, dynamic `GetContentRegionAvail()` layout sizing, Dear ImGui gamepad navigation invariants (`ImGuiNavLayer_Main`), hybrid touch-drag vs. right-stick scrolling, controller hotplugging, and the `175×275` artwork pipeline.
- **[Switch Upgrade Playbook](switch-upgrade-playbook.md)** — Backup checklist, upgrade order and Emulanders validation checklist for new firmware / Atmosphère / CNX releases, plus the versioning policy.
- **[Release, CI/CD & Homebrew App Store Packaging](release-and-packaging.md)** — Build instructions (`cargo-nx` + `devkitA64`), GitHub Actions release pipeline, dual-ZIP packaging (`emulanders-vX.Y.Z.zip` vs. `EmulandersPortal-hbappstore.zip`), 4TU Homebrew App Store (`switch-hbas-repo` / Spinarak) integration, 8-file version synchronization checklist, and DBI MTP deployment.

---

## Project Tracking & Logs

- **[Chronological Engineering Log (`log.md`)](log.md)** — Append-only chronological record of architectural milestones, audits, bug fixes, and releases.
- **[Root README](../README.md)** — User-facing overview, hardware compatibility notes, and installation instructions.
- **[Known Issues](../KNOWN_ISSUES.md)** — Tracked hardware/software limitations and user workarounds.
- **[Roadmap](../ROADMAP.md)** — Planned post-1.0 features and architectural goals.
