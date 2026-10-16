# UI, Display & Input Engineering (Nintendo Switch 720p / 1080p)

See also: [`wiki/index.md`](index.md) | [`wiki/architecture.md`](architecture.md)

---

## 1. Nintendo Switch Display Geometry & Scaling

Nintendo Switch operates in two primary display modes, both with a **16:9 aspect ratio**:

| Mode | Output Resolution | Physical Display | Scaling Factor (`ui_scale`) |
| :--- | :--- | :--- | :--- |
| **Handheld / Tabletop** | `1280 × 720` (720p) | 6.2" LCD (V1/V2), 5.5" LCD (Lite), or 7.0" OLED | `ui_scale = 1.0f` (`720 / 720.0f`) |
| **Docked (TV Mode)** | `1920 × 1080` (1080p) | External HDMI Display | Hardware-upscaled from `1280 × 720` by `nvdrv`/`vi`, or dynamically scaled via `ui_scale = (float)win_h / 720.0f` on `SDL_WINDOWEVENT_SIZE_CHANGED`. |

### 1.1 Vertical Screen Budget (`EmulandersPortal`)
In [`EmulandersPortal/source/main.cpp`](../EmulandersPortal/source/main.cpp#L285-L290), the `1280 × 720` (`screen_w × screen_h`) root window is partitioned into three horizontal bands:

| Region | Height Formula | Pixel Height (720p) | Pixel Height (1080p) | Purpose |
| :--- | :--- | :---: | :---: | :--- |
| **`Header`** | `screen_h * 0.12f` | `86.4 px` | `129.6 px` | App title, version (`1.0.0`), Sysmodule/Emulation status badges, Wi-Fi, Battery, Clock. |
| **`Content` + `Sidebar`** | `screen_h * 0.80f` (`content_h`) | `576.0 px` | `864.0 px` | Left navigation sidebar (`screen_w * 0.25f` = `320 px`) and active feature viewport (`screen_w * 0.75f` = `960 px`). |
| **`Footer`** | `screen_h * 0.08f` | `57.6 px` | `86.4 px` | Context-sensitive controller and touch shortcut hints. |

### 1.2 The `GetContentRegionAvail()` Vertical Sizing Law

Inside the `Content` child window (`576 px` tall at 720p), [`main.cpp`](../EmulandersPortal/source/main.cpp#L482-L497) first renders the `ScreenHeader` sub-window of height `content_h / 6.0f` (`96 px`), followed by a separator and spacing. Each screen (`RenderScreenFileManager`, `RenderScreenImageEditor`, `RenderScreenDownloaders`, `RenderScreenSettings`) then renders its own title, path breadcrumb, and action toolbar (`~120–155 px`).

> [!IMPORTANT]
> **Never calculate table or scroll container heights as `content_h - C` (where `C` is a hardcoded constant).**
> Because `ScreenHeader` (`96 px`) and screen-specific toolbars (`~150 px`) already consume `~246 px` inside `Content`, only `~310–330 px` of vertical height remains at 720p. Using a hardcoded `content_h - 195.0f` (`381 px`) overflows the `Content` window by `~55–75 px` and clips the bottom rows of tables behind the `Footer` bar.
>
> **Always compute vertical height dynamically at the exact cursor position:**
> ```cpp
> float table_h = std::max(120.0f * ui_scale, ImGui::GetContentRegionAvail().y - 6.0f * ui_scale);
> ```
> This guarantees pixel-accurate bottom alignment across all screens, clipboard states, and both 720p and 1080p resolutions.

---

## 2. Dear ImGui Gamepad & Hybrid Input Architecture

### 2.1 Nintendo A/B Button Swap
Nintendo Switch controllers place **A on the right** (`ImGuiKey_GamepadFaceRight`) and **B on the bottom** (`ImGuiKey_GamepadFaceDown`). In [`main.cpp`](../EmulandersPortal/source/main.cpp#L86):
```cpp
io.ConfigNavSwapGamepadButtons = true;
```
This configures Dear ImGui so **A** (`FaceRight`) activates widgets and **B** (`FaceDown`) cancels/exits popups.

### 2.2 The `[Y]` Button & `ImGuiNavLayer_Main` Invariant
By default in Dear ImGui, pressing `ImGuiKey_GamepadFaceLeft` (**Switch `[Y]` button**) triggers `NavWindowing` or switches focus from `ImGuiNavLayer_Main` (`0`) to `ImGuiNavLayer_Menu` (`1`). Because Emulanders Portal uses custom header/sidebar child windows rather than an ImGui `MenuBar`, switching to `ImGuiNavLayer_Menu` causes the blue gamepad focus cursor to vanish and locks out D-Pad navigation.

To prevent this while using `[Y]` as a shortcut (e.g., `[Y] ★ Favorites` in the File Manager and Artwork Studio), [`main.cpp`](../EmulandersPortal/source/main.cpp#L221-L229) enforces the navigation layer lock every frame after `ImGui::NewFrame()`:
```cpp
ImGuiContext& g_ctx = *GImGui;
if (g_ctx.NavLayer != ImGuiNavLayer_Main) {
    g_ctx.NavLayer = ImGuiNavLayer_Main;
}
g_ctx.NavWindowingToggleLayer = false;
if (g_ctx.NavWindowingTarget != NULL) {
    g_ctx.NavWindowingTarget = NULL;
}
```

### 2.3 Hybrid Scrolling: Touch-Drag (Handheld) vs. Right-Stick (Docked/Handheld)
1. **Handheld Capacitive Touch-Drag (`SDL_FINGERDOWN` / `SDL_FINGERMOTION` / `SDL_FINGERUP`):**
   - A tap within `drag_threshold = 12.0f * ui_scale` pixels acts as a normal mouse click (`io.AddMouseButtonEvent(0, true/false)`).
   - Once finger movement exceeds `12 px` (except inside the Step 3 Interactive Cropper where drag moves the crop box), `s_touch_is_dragging` becomes `true`, the synthetic mouse click is released off-screen (`-10000, -10000`) to prevent accidental row activation, and vertical finger deltas (`s_touch_drag_accum_y`) directly scroll the hovered scrollable `ImGuiWindow` (`win->Scroll.y`).
2. **Controller Right-Stick Smooth Scrolling (`SDL_CONTROLLER_AXIS_RIGHTY`):**
   - Outside the Step 3 Cropper, deflecting the Right Stick past the `7500` deadzone smoothly scrolls the focused/hovered table or child window at `14.0f * ui_scale` pixels per frame at 60 FPS.
3. **Controller Hotplugging (Docked $\leftrightarrow$ Handheld Transitions):**
   - When the user docks or undocks the Switch, Horizon OS disconnects attached Joy-Cons and connects wireless Joy-Cons or a Pro Controller. Handling `SDL_CONTROLLERDEVICEADDED` and `SDL_CONTROLLERDEVICEREMOVED` in `main.cpp` dynamically rebinds `G.controller` without requiring an app restart.

---

## 3. Portrait Artwork Pipeline (`175 × 275`)

### 3.1 Dimensions & Aspect Ratio
Both the Tesla Overlay (`448 × 720` layer) and Emulanders Portal standardize figure portraits at **`175 × 275` pixels** (`LOGO_WIDTH = 175`, `LOGO_HEIGHT = 275`, aspect ratio `0.6363636f`, 32-bit `RGBA32` `.png`).

### 3.2 Resolution Order for Figure Portraits
When displaying a figure in either the Tesla Overlay or Emulanders Portal:
1. **Custom Adjacent PNG (Highest Priority):** `<figure_dir>/<figure_basename>.png` (created by the in-app Custom Artwork Studio).
2. **Variant-Specific Community Asset:** `sdmc:/emulanders/assets/<CharID_06X>_<VarID_04X>.png` (read from Sector 0 offsets `0x10..0x12` and `0x1C..0x1D`).
3. **Base Character Community Asset:** `sdmc:/emulanders/assets/<CharID_06X>.png`.

### 3.3 Frame-Budgeted Lazy Texture Loading
Decoding `.png` and `.jpg` files from a FAT32/exFAT SD card on the Switch's ARM Cortex-A57 CPU takes `~2–8 ms` per image. To prevent frame drops when opening folders with hundreds of `.dump` files or Album screenshots:
- `RenderScreenFileManager` loads at most **4 thumbnails per frame** (`tex_loads_this_frame < 4`).
- `RenderScreenImageEditor` loads at most **3 thumbnails per frame** (`dump_tex_loads < 3` / `thumb_loads_this_frame < 3`).
- Textures are freed (`glDeleteTextures`) immediately when navigating to a different directory or exiting the application.
