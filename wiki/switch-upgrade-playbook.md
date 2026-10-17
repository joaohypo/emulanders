# Switch Upgrade Playbook (22.1 / AMS 1.11.1 -> 23.0.x / AMS 1.12.1)

See also: [`index.md`](index.md) | [`release-and-packaging.md`](release-and-packaging.md) | [`nfc-ipc-protocol.md`](nfc-ipc-protocol.md)

Working notes for upgrading the developer console and re-validating Emulanders on the new stack. Written before the upgrade; the "Results" section at the bottom is filled in afterwards.

## 1. Target stack

| Component | Current | Target | Source |
| :--- | :--- | :--- | :--- |
| Pack | CNX 22.1.0-0 | CNX 23.0.1-1 | https://github.com/CostelaCNX/CNX/releases/tag/23.0.1-1 |
| Firmware | 22.1 | 23.0.x (what the CNX pack targets) | via Daybreak |
| Atmosphere | 1.11.1 | 1.12.1 (CNX ships "Atmosphere-CNX 1.12.1") | https://github.com/Atmosphere-NX/Atmosphere/releases |

CNX 23.0.1-1 changelog (from the release page): Atmosphere-CNX 1.12.1, CNX Updater 4.2.3, DBI 912 PTBR.

## 2. What the Atmosphere changelogs mean for Emulanders

Taken from the 1.11.x / 1.12.x release notes, not from testing yet.

| Change | Version | Risk for Emulanders |
| :--- | :--- | :--- |
| Memory for custom system modules shrinks again on 23.0.0; AMS patches browser applets to get back to ~16 MB | 1.12.0 | Low. The sysmodule heap is `0x10000` and the NPDM asks for `system_resource_size = 0`. Verify it still boots (`boot2.flag`) and survives opening the eShop. |
| `fs.mitm` extended for the new `IStorageForBatchRead` | 1.12.1 | None expected; we only use plain `fsp-srv` file access. |
| `dns.mitm` for the new c-ares resolver | 1.12.1 | None; Emulanders only uses libcurl from the Portal for the OTA downloaders. Re-test both downloaders. |
| Homebrew exit behaviour (`am` patches) | 1.11.0 | Already in effect on 22.1. Confirm Portal still returns cleanly to hbmenu. |
| New `ConfigItem`, kernel 42-bit address space, ShadowStack | 1.12.0 | Not touched by a Rust/libnx sysmodule. |

Open question that only hardware can answer: did Nintendo change `nfc:mf:u` IPC in 23.0.0? The sysmodule is a MitM of that service and implements all 14 commands (see [`nfc-ipc-protocol.md`](nfc-ipc-protocol.md)). Test with Skylanders Imaginators, not just boot.

## 3. Backups (do all of these before touching anything)

Order matters: the irreplaceable items first.

### 3.1 Console identity (cannot be regenerated)
1. Boot Hekate, then Tools -> Backup eMMC: `BOOT0`, `BOOT1`, and **`PRODINFO` / `PRODINFOF`** (`Backup eMMC -> eMMC BOOT0 & BOOT1` and `eMMC RAW GPP`). Copy `backup/<serial>/` to the PC.
2. Run Lockpick_RCM and copy `prod.keys` and `title.keys` to the PC.
3. If emuMMC is in use: Tools -> Backup eMMC works on the emuMMC partition/files separately. Copy `emuMMC/` raw files only if the SD can spare the space; at minimum record whether it is partition-based or file-based.

### 3.2 Saves
Back up with JKSV or Checkpoint, then copy `JKSV/` (or `switch/Checkpoint/saves`) to the PC. Skylanders saves are the ones that matter here.

### 3.3 Emulanders data (small, back up first and keep a copy off the SD)
```text
sdmc:/emulanders/                      # figures, assets, config.json, favorites, dumps_urls.txt
sdmc:/atmosphere/contents/420000000000E311/
sdmc:/switch/EmulandersPortal/
sdmc:/switch/.overlays/emulanders.ovl
```
`sdmc:/emulanders/figures/` is the only part with user data that is hard to recreate (physical dumps).

### 3.4 Whole SD card
Copy the entire SD to a dated folder on the PC (`robocopy` or Explorer). This is the real rollback: with it, a failed pack update costs nothing.

### 3.5 Notes to record before upgrading
- exFAT or FAT32 on the SD (Daybreak asks, and picking wrong breaks boot).
- emuMMC type (partition vs file) and whether sysMMC is used online.
- Current Hekate version and which payload launches CFW.

## 4. Upgrade order

1. Finish section 3. Verify the PC copy opens.
2. Read the CNX 23.0.1-1 release page fully, including the "known issues" paragraph.
3. Update the pack with CNX Updater, or extract the release zip over the SD. Do **not** delete `sdmc:/emulanders/` or `atmosphere/contents/420000000000E311/`.
4. Atmosphere releases require updating `fusee` (`fusee-primary` no longer exists). Launch via the updated Hekate/`fusee.bin`; do not keep an old payload.
5. Boot into CFW on the **old** firmware first. If it boots, the pack update is good; stop here if anything is wrong.
6. Install the firmware with Daybreak, targeting emuMMC if that is what is in use. Choose the filesystem option that matches section 3.5. Do not enable "preserve settings" shortcuts you do not understand; use the defaults.
7. Reboot through `fusee`, confirm the firmware version in System Settings.
8. Run the validation checklist below.

If step 5 or 7 fails: restore the SD copy from 3.4; the firmware install is the only step that is not trivially reversible, which is why emuMMC is the safer target.

## 5. Emulanders validation checklist after the upgrade

- [x] Console boots to Home, Tesla opens (`L + D-Pad Down + R3`), `emulanders` entry present.
- [x] Overlay shows no version-mismatch warning (sysmodule and overlay are both 1.0.1).
- [x] Portal launches from hbmenu in Full Application Mode (Title Override) and exits back cleanly.
- [x] Portal: all 10 languages render; Settings buttons wrap without clipping.
- [x] Portal: Joy-Con dumper reads a physical figure.
- [x] Portal: image editor (with crop B-button fix), file manager, both downloaders.
- [x] Skylanders Imaginators: enable emulation before launch, mount a figure in-game, swap, unmount.
- [x] eShop / browser applet opens without crashing (memory pressure check from the 1.12.0 notes).
- [x] Debug log extraction (`debug_log_dump.txt`) confirmed clean.

## 6. Version policy

- Cosmetic/UI/translation/hardening changes: `1.0.x`.
- Changes required by the new firmware that keep working on older ones: `1.1.0`.
- Dropping support for older firmware or changing the NFC IPC surface: `1.2.0` (or higher), with the minimum firmware stated in `README.md`.

## 7. Results (Executed 2026-10-16 / 2026-10-17)

**Hardware Tested:** Nintendo Switch (V1/V2 EmuMMC SD-File `sdmc:/emuMMC/SD00/`)  
**Firmware Version:** Horizon OS `23.0.1` (via Daybreak)  
**Atmosphère Version:** `1.12.1-master-1937ccc1` (CNX `23.0.1-1`)

### Key Findings & Fixes
1. **`nfc:mf:u` Interception Verified:** Nintendo did **not** alter the command serialization or interface structure of `nfc:mf:u` in Horizon 23.0.0 / 23.0.1. All 14 commands implemented by `emulanders` sysmodule continue to operate out-of-the-box. Tag discovery events (`AttachActivateEvent`) and Mifare Classic read/write transactions execute flawlessly.
2. **Overlay Overwrite by CFW Updater:** Upgrading via CNX Updater or full pack extraction overwrites `sdmc:/switch/.overlays/` with the stock bundle. Restoring `emulanders.ovl` (hash `62B19D8C...`) immediately restores the overlay in Tesla Menu.
3. **Applet Mode Memory Crash (`0xD401`):** In Horizon 23.x, Applet Mode (Photo Album) memory pool is strictly throttled (~7–16 MB). Opening `EmulandersPortal` in Applet Mode immediately hits `0xD401 (2001-0106: Kernel Out of Memory)`. Launching via **Title Override** (holding `R` while launching an installed game) allocates the full 3.5 GB of RAM, running the Portal smoothly.
4. **Crop Screen UI Fit:** In Step 3 (Interactive Cropper), the live preview thumbnail allowed up to 165px height, pushing the `(B) Back to Photos` button into the bottom status bar at 720p. Fixed in `1.0.1` by capping `prev_h` at 125px and tuning button height to 38px, giving 190px of vertical breathing room.
5. **Sysmodule Footprint & Sigpatches:** Pruned unused/dormant background sysmodules. Confirmed `sys-patch` (`420000000000000B`) is required for fakesigned content verification on FW 23.0.1.

