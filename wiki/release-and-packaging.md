# Release, CI/CD & Homebrew App Store Packaging

See also: [`wiki/index.md`](index.md) | [`wiki/architecture.md`](architecture.md)

---

## 1. Version Synchronization Checklist (8 Files)

Before tagging any release, the version string (`MAJOR.MINOR.MICRO`) MUST be synchronized across all **8 locations** in the repository:

| # | File | Variable / Field | Format Example |
| :---: | :--- | :--- | :--- |
| 1 | [`emulanders/Cargo.toml`](../emulanders/Cargo.toml#L3) | `version` | `"1.0.1"` |
| 2 | [`Makefile`](../Makefile#L46-L48) | `dist` / `dist-dev` bundle filenames | `1.0.1` |
| 3 | [`overlay/Makefile`](../overlay/Makefile#L41-L43) | `VER_MAJOR` / `VER_MINOR` / `VER_MICRO` | `1` / `0` / `1` |
| 4 | [`overlay/source/main.cpp`](../overlay/source/main.cpp#L65) | `ExpectedVersion` (built from the Makefile `VER_*` values) | `{ 1, 0, 1, {} }` |
| 5 | [`EmulandersPortal/Makefile`](../EmulandersPortal/Makefile#L18) | `APP_VERSION` | `1.0.1` |
| 6 | [`EmulandersPortal/source/utils/constants.hpp`](../EmulandersPortal/source/utils/constants.hpp#L26-L27) | `APP_VERSION_STRING` & `APP_USER_AGENT` | `"v1.0.1"` & `"EmulandersPortal/1.0.1"` |
| 7 | [`README.md`](../README.md) | Release references & download links | `v1.0.1` / `1.0.1` |
| 8 | [`RELEASE_NOTES.md`](../RELEASE_NOTES.md) | Release title & asset URLs | `1.0.1` |

> [!WARNING]
> If `ExpectedVersion` in `overlay/source/main.cpp` does not match the version compiled into `emulanders/Cargo.toml` (`CARGO_PKG_VERSION`), `EmulandersOverlay::initServices()` sets `InitializationStatus::OkVersionMismatch` and displays a version mismatch warning in the overlay.

---

## 2. Local Build Commands

### 2.1 Prerequisites
- **devkitPro:** `devkitA64`, `libnx`, `switch-sdl2`, `switch-sdl2_image`, `switch-glad`, `switch-curl`, `switch-minizip`, `switch-zlib`, `switch-freetype`.
- **Rust Switch Toolchain:** Nightly Rust with `rust-src` and `cargo-nx` targeting `aarch64-nintendo-switch-freestanding`.

### 2.2 Compiling Each Component
```powershell
# 1. Compile the Rust Background Sysmodule (outputs emulanders.nsp)
cd emulanders
cargo nx build --release

# 2. Compile the Tesla Overlay (outputs overlay/emulanders.ovl)
cd ../overlay
make -j8

# 3. Compile Emulanders Portal Manager (outputs EmulandersPortal/EmulandersPortal.nro)
cd ../EmulandersPortal
make -j8
```

---

## 3. Dual-ZIP Packaging & 4TU Homebrew App Store (`switch-hbas-repo`)

GitHub Actions ([`.github/workflows/release.yml`](../.github/workflows/release.yml)) produces two distinct ZIP structures for every release tag:

### 3.1 `emulanders-vX.Y.Z.zip` (Manual SD Card Package)
Contains `SdOut/` at its root (matching standard `emuiibo` release archives) for users who manually drag the contents of `SdOut/` onto their SD card root.

### 3.2 `EmulandersPortal-hbappstore.zip` (4TU Homebrew App Store / Spinarak Package)
The Nintendo Switch Homebrew App Store (`fortheusers/switch-hbas-repo`, automated by **Spinarak** / **DragoniteBot**) extracts the matched release asset directly to the root of the SD card (`sdmc:/`). Therefore, `EmulandersPortal-hbappstore.zip` MUST have a **flat SD-root hierarchy** with no top-level wrapper folder:

```text
EmulandersPortal-hbappstore.zip
├── atmosphere/
│   └── contents/
│       └── 420000000000E311/
│           ├── exefs.nsp
│           ├── toolbox.json
│           └── flags/
│               └── boot2.flag
├── switch/
│   ├── .overlays/
│   │   └── emulanders.ovl
│   └── EmulandersPortal/
│       └── EmulandersPortal.nro
└── emulanders/
    └── overlay/
        └── lang/
            ├── de.json
            ├── en.json
            ├── es.json
            ├── fr.json
            ├── it.json
            ├── pt-BR.json
            ├── ru.json
            ├── zh-Hans.json
            └── zh-Hant.json
```

### 3.3 How DragoniteBot Automatic Updates Work
In [`fortheusers/switch-hbas-repo/packages/Emulanders/pkgbuild.json`](https://github.com/fortheusers/switch-hbas-repo/blob/main/packages/Emulanders/pkgbuild.json), the package tracks:
- `"url": "https://github.com/joaohypo/emulanders/releases"`
- `"asset_SimpleZip": ["EmulandersPortal-hbappstore.zip"]`

Whenever a new GitHub Release is published on `joaohypo/emulanders` with an attached `EmulandersPortal-hbappstore.zip` asset, **DragoniteBot** automatically detects the new release tag during its daily scan and opens an `[auto] Update Emulanders to X.Y.Z` pull request on `fortheusers/switch-hbas-repo`.

### 3.4 Homebrew App Store Visual Asset Specifications (`switch-hbas-repo`)
The Homebrew App Store (`hb-app.store` web frontend, `hb-appstore` Chesto app, and `Sphaira` store client) uses fixed-aspect containers distinct from Horizon OS's internal `256 × 256` `.nro` icon:
- **NRO Internal Icon (`EmulandersPortal/icon.jpg`):** Must remain **`256 × 256` px** (square `1:1`) for Horizon OS Home Screen forwarders and `hbmenu`.
- **Store Card Icon (`packages/Emulanders/icon.png`):** Must be **`256 × 150` px** (`1.706:1`, rendered at `249 × 146` CSS px on `hb-app.store/search` and in the right-hand panel on console). Submitting a `256 × 256` square causes the store UI to center-crop the top and bottom.
- **Store Banner (`packages/Emulanders/screen.png`, `"type": "banner"`):** Must be **`848 × 208` px** (`4.077:1`). Submitting a wider/taller image (e.g., `1280 × 337`) causes the on-console store viewport to clip the left/right/top/bottom edges.

---

## 4. Direct Hardware Deployment via DBI MTP Responder

During local development, compiled binaries can be pushed directly to a Nintendo Switch running **DBI (`Run MTP responder`)** over USB using Windows Shell COM automation (`Shell.Application`) against the `1: SD Card` virtual folder:
- Copy `EmulandersPortal/EmulandersPortal.nro` $\rightarrow$ `SD Card\switch\EmulandersPortal\EmulandersPortal.nro`
- Copy `overlay/emulanders.ovl` $\rightarrow$ `SD Card\switch\.overlays\emulanders.ovl`
- Copy `emulanders/target/aarch64-nintendo-switch-freestanding/release/emulanders.nsp` $\rightarrow$ `SD Card\atmosphere\contents\420000000000E311\exefs.nsp`

> [!CAUTION]
> **Never call `$item.InvokeVerb("delete")` on an MTP Shell item from an automated script.** Windows Explorer intercepts MTP deletions with a modal confirmation dialog that blocks headless execution indefinitely. Instead, rename the existing file in place (`$item.Name = "EmulandersPortal_old.nro"`) and call `$destFolder.CopyHere($srcFolder.ParseName($fileName), 16)`.
