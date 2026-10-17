# Known Issues & Workarounds

This document tracks known technical limitations and workarounds in Emulanders.

---

## Docked Mode: Main UI Disappearance

### Description
When running the console in Docked Mode, selecting "emulanders" from the Tesla menu may result in the main UI window disappearing or failing to render. The background Tesla menu remains functional (allowing you to press 'B' to return), but the Emulanders-specific interface becomes invisible.

### Technical Cause
This issue is caused by a system-level compositing conflict within the Nintendo Switch's Video Interface (`vi`) service. It occurs when:
1. The console is in Docked Mode (outputting at 1080p).
2. The Switch's internal **Adjust Screen Size** setting is set below 100%.
3. The game being played (*Skylanders: Imaginators*) has protected content flags that block screen recording/screenshots.

When these conditions meet, the system's overscan scaler fails to correctly compose the homebrew overlay layer, leading the OS to hide the layer to maintain system stability.

### Workaround
Set your console's screen scaling to its native value:
1. Go to **System Settings** on your Nintendo Switch.
2. Navigate to **TV Settings** > **Adjust Screen Size**.
3. Set the slider to **100%**.
4. If the edges of the image are cut off on your television, adjust the **Overscan** or **Aspect Ratio** settings on your TV itself (usually labeled as "Just Scan", "1:1 Pixel Mapping", "Original", or "Fit to Screen").

---

## Handheld Mode: Joy-Con Attachment Notice

### Description
In Handheld Mode with Joy-Cons physically attached to the rails, the game may display a notice stating "Controller not compatible with reading toys."

### Workaround
Detach the Joy-Con controllers from the console. This is a game-level restriction within *Skylanders: Imaginators* and not an issue with the Emulanders sysmodule itself.

---

## Applet Mode (Album) Memory Limits (Error 0xD401 / 2001-0106)

### Description
Launching `EmulandersPortal` from the Homebrew Menu via the Photo Album (Applet Mode) crashes immediately on startup with error code `0xD401 (2001-0106: Kernel Out of Memory)`.

### Technical Cause
In modern Horizon OS versions (19.0.0 through 23.0.0+), Nintendo drastically reduced the system memory pool allocated to the Photo Album applet (~7 to 16 MB). Because Emulanders Portal initializes SDL2, GLAD, Mesa/nouveau OpenGL 4.3, TrueType font atlases for 10 languages, and image textures, it requires ~60 to 120 MB of RAM, triggering an instant Kernel Out-of-Memory panic.

### Workaround
Always launch the Homebrew Menu in **Full Application Mode** via **Title Override**:
1. On the Nintendo Switch Home Menu, hold the **`R`** button on your controller.
2. While holding **`R`**, click on **any installed game** (or demo/cartridge).
3. Keep holding **`R`** until the Homebrew Menu opens.
4. Launch **Emulanders Portal** with full access to **3.5 GB of RAM**.
*(Alternatively, launch via an installed Home Menu Forwarder NSP).*
