# ⚠️ Known Issues & Workarounds

This document tracks known technical limitations and bugs within the Emulanders project and provides temporary solutions.

---

## 📺 Docked Mode: Main UI Disappearance

### 🔍 Description
When running the console in **Docked Mode**, selecting "emulanders" from the Tesla menu may result in the main UI window disappearing or failing to render. The background Tesla menu remains functional (allowing you to press 'B' to return), but the Emulanders-specific interface becomes invisible.

### 🛠️ Technical Root Cause
This issue is caused by a system-level compositing conflict within the Nintendo Switch's Video Interface (`vi`) service. It occurs when:
1. The console is in **Docked Mode** (outputting at 1080p).
2. The Switch's internal **"Adjust Screen Size"** setting is set to less than **100%**.
3. The game being played (**Skylanders: Imaginators**) has protected content flags that block screen recording/screenshots.

When these conditions meet, the system's overscan scaler fails to correctly compose the homebrew overlay layer, leading the OS to hide the layer to maintain system stability.

### ✅ Workaround (SOTA)
To fix this, you must set your console's screen scaling to its native value:
1. Go to **System Settings** on your Nintendo Switch.
2. Navigate to **TV Settings** > **Adjust Screen Size**.
3. Set the slider exactly to **100%**.
4. If the edges of the image are cut off on your television, you should adjust the **"Overscan"** or **"Aspect Ratio"** settings on the **TV itself** (usually labeled as "Just Scan", "1:1 Pixel Mapping", "Original", or "Fit to Screen").

---

## 🎮 Handheld Mode: Joy-Con Controller Incompatibility

### 🔍 Description
In Handheld Mode (Joy-Cons physically attached), the game may display a message stating "Controller not compatible with reading toys."

### ✅ Workaround
Detach the Joy-Con controllers from the console. This is a game-level restriction within *Skylanders: Imaginators* and not an issue with the Emulanders sysmodule itself.
