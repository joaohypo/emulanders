#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <atomic>
#include <string>

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "globals.hpp"
#include "emulanders_ipc.h"
#include "utils/path_utils.hpp"
#include "utils/ui_helpers.hpp"

#include "screens/screen_dumper.hpp"
#include "screens/screen_filemanager.hpp"
#include "screens/screen_imageeditor.hpp"
#include "screens/screen_downloaders.hpp"
#include "screens/screen_settings.hpp"

#include "icon_dumper_bin.h"
#include "icon_manager_bin.h"
#include "icon_editor_bin.h"
#include "icon_dumpdl_bin.h"
#include "icon_asset_bin.h"
#include "icon_settings_bin.h"

Globals G;

static void UpdateSystemStatus() {
    u32 battery = 0;
    if (R_SUCCEEDED(psmGetBatteryChargePercentage(&battery))) G.battery_charge = battery;

    u32 ip = 0;
    if (R_SUCCEEDED(nifmGetCurrentIpAddress(&ip)))
        snprintf(G.ip_address, sizeof(G.ip_address), "%d.%d.%d.%d",
                 ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
    else
        snprintf(G.ip_address, sizeof(G.ip_address), "Offline");

    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(G.time_str, sizeof(G.time_str), "%H:%M", tm);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    srand(static_cast<unsigned>(time(NULL)));

    socketInitializeDefault();
    psmInitialize();
    nifmInitialize(NifmServiceType_User);
    timeInitialize();
    emulandersInitialize();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) return -1;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow("Emulanders Portal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);
    gladLoadGLLoader(SDL_GL_GetProcAddress);

    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);
    float ui_scale = (float)win_h / 720.0f;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigNavSwapGamepadButtons = true;
    io.ConfigNavCursorVisibleAlways = true;
    io.ConfigNavCursorVisibleAuto = true;
    LoadAppConfig();
    SetupImGuiStyle(ui_scale, G.accent, G.accent_hover, G.accent_active);

    ImFont *f_large = nullptr, *f_title = nullptr, *f_small = nullptr;
    if (R_SUCCEEDED(plInitialize(PlServiceType_User))) {
        PlFontData font;
        if (R_SUCCEEDED(plGetSharedFontByType(&font, PlSharedFontType_Standard))) {
            static const ImWchar glyph_ranges[] = {
                0x0020, 0x00FF, // Basic Latin + Latin Supplement (accents)
                0x2600, 0x26FF, // Miscellaneous Symbols (for ★ U+2605)
                0
            };
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 22.0f * ui_scale, &cfg, glyph_ranges);
            f_large = io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 36.0f * ui_scale, &cfg, glyph_ranges);
            f_title = io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 36.0f * ui_scale, &cfg, glyph_ranges);
            f_small = io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 18.0f * ui_scale, &cfg, glyph_ranges);
        }
    }
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 430 core");

    G.tex_icon_dumper   = LoadTextureFromMemory(icon_dumper_bin, icon_dumper_bin_size);
    G.tex_icon_manager  = LoadTextureFromMemory(icon_manager_bin, icon_manager_bin_size);
    G.tex_icon_editor   = LoadTextureFromMemory(icon_editor_bin, icon_editor_bin_size);
    G.tex_icon_dumpdl   = LoadTextureFromMemory(icon_dumpdl_bin, icon_dumpdl_bin_size);
    G.tex_icon_asset    = LoadTextureFromMemory(icon_asset_bin, icon_asset_bin_size);
    G.tex_icon_settings = LoadTextureFromMemory(icon_settings_bin, icon_settings_bin_size);

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) { G.controller = SDL_GameControllerOpen(i); break; }
    }

    RefreshFileList(FIGURES_DIR, G.sd_files, G.current_path);

    bool done = false;
    Uint32 last_status_update = 0;
    bool just_entered_content = true;
    bool first_boot = true;

    static GLuint t_icons[] = {0, 0, 0, 0, 0, 0}; // populated post-init

    while (!done) {
        if (!appletMainLoop()) done = true;
        if (SDL_GetTicks() - last_status_update > 2000) {
            UpdateSystemStatus();
            last_status_update = SDL_GetTicks();
        }

        if (first_boot) {
            io.AddKeyEvent(ImGuiKey_GamepadDpadDown, true);
            io.AddKeyEvent(ImGuiKey_GamepadDpadDown, false);
            first_boot = false;
        }

        static float s_touch_start_x = 0.0f;
        static float s_touch_start_y = 0.0f;
        static float s_touch_last_y = 0.0f;
        static bool s_touch_is_dragging = false;
        static float s_touch_drag_accum_y = 0.0f;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_FINGERDOWN) {
                float mx = event.tfinger.x * (float)win_w;
                float my = event.tfinger.y * (float)win_h;
                s_touch_start_x = mx;
                s_touch_start_y = my;
                s_touch_last_y = my;
                s_touch_is_dragging = false;
                s_touch_drag_accum_y = 0.0f;
                io.AddMousePosEvent(mx, my);
                io.AddMouseButtonEvent(0, true);
            }
            else if (event.type == SDL_FINGERMOTION) {
                float mx = event.tfinger.x * (float)win_w;
                float my = event.tfinger.y * (float)win_h;
                io.AddMousePosEvent(mx, my);

                float total_dx = mx - s_touch_start_x;
                float total_dy = my - s_touch_start_y;
                float dist_sq = total_dx * total_dx + total_dy * total_dy;
                float drag_threshold = 12.0f * ui_scale;

                bool is_crop_tool = (!G.in_menu && G.selected_menu == 2 && G.editor_state == EditorState::CROP_EDIT);

                if (!is_crop_tool && !s_touch_is_dragging && dist_sq > (drag_threshold * drag_threshold)) {
                    s_touch_is_dragging = true;
                    io.AddMousePosEvent(-10000.0f, -10000.0f);
                    io.AddMouseButtonEvent(0, false);
                    io.AddMousePosEvent(mx, my);
                }

                if (s_touch_is_dragging) {
                    float delta_y = my - s_touch_last_y;
                    s_touch_drag_accum_y += delta_y;
                }
                s_touch_last_y = my;
            }
            else if (event.type == SDL_FINGERUP) {
                io.AddMouseButtonEvent(0, false);
                io.AddMousePosEvent(-10000.0f, -10000.0f);
                s_touch_is_dragging = false;
                s_touch_drag_accum_y = 0.0f;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Prevent Dear ImGui from switching to empty Menu layer or windowing when Y is pressed
        ImGuiContext& g_ctx = *GImGui;
        if (g_ctx.NavLayer != ImGuiNavLayer_Main) {
            g_ctx.NavLayer = ImGuiNavLayer_Main;
        }
        g_ctx.NavWindowingToggleLayer = false;
        if (g_ctx.NavWindowingTarget != NULL) {
            g_ctx.NavWindowingTarget = NULL;
        }

        // Direct Touch Drag Scrolling (Handheld)
        if (s_touch_is_dragging && fabs(s_touch_drag_accum_y) > 0.001f) {
            ImGuiContext& g = *GImGui;
            ImGuiWindow* win = g.HoveredWindow;
            while (win && win->ScrollMax.y <= 0.0f && win->ParentWindow) {
                win = win->ParentWindow;
            }
            if (win && win->ScrollMax.y > 0.0f) {
                float new_scroll = win->Scroll.y - s_touch_drag_accum_y;
                if (new_scroll < 0.0f) new_scroll = 0.0f;
                if (new_scroll > win->ScrollMax.y) new_scroll = win->ScrollMax.y;
                win->Scroll.y = new_scroll;
            }
            s_touch_drag_accum_y = 0.0f;
        }

        bool any_modal_open = ImGui::IsPopupOpen((const char*)NULL, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)
                            || ImGui::IsPopupOpen("FileActions")
                            || ImGui::IsPopupOpen("CloneWarning");
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            if (!any_modal_open) {
                if (G.in_menu) {
                    // Already in menu: consume B and ensure nav focus stays locked on the selected menu option
                    G.menu_needs_nav_focus = true;
                    ImGuiContext& g = *GImGui;
                    g.NavCursorVisible = true;
                } else {
                    bool handled = false;
                    if (G.selected_menu == 1 && G.current_path != FIGURES_DIR) {
                        if (G.current_path == "<favorites>") {
                            G.current_path = FIGURES_DIR;
                            RefreshFileList(FIGURES_DIR, G.sd_files, G.current_path);
                        } else {
                            RefreshFileList(GetParentPath(G.current_path), G.sd_files, G.current_path);
                        }
                        G.file_manager_needs_focus = true;
                        just_entered_content = true;
                        handled = true;
                    } else if (G.selected_menu == 2) {
                        // Delegated to RenderScreenImageEditor for multi-level back navigation
                        handled = true;
                    }
                    if (!handled) {
                        G.in_menu = true;
                        G.menu_needs_nav_focus = true;
                        just_entered_content = true;
                    }
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_GamepadStart))
            done = true;

        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;
        float header_h = screen_h * 0.12f;
        float footer_h = screen_h * 0.08f;
        float content_h = screen_h - header_h - footer_h;

        // Controller Right-Stick Smooth Scrolling (Docked & Handheld)
        if (G.controller) {
            int16_t ry = SDL_GameControllerGetAxis(G.controller, SDL_CONTROLLER_AXIS_RIGHTY);
            if (abs(ry) > 7500) {
                // In Crop Edit, Right Stick is reserved for zooming the crop box
                if (!(G.selected_menu == 2 && G.editor_state == EditorState::CROP_EDIT)) {
                    ImGuiContext& g = *GImGui;
                    ImGuiWindow* win = g.HoveredWindow ? g.HoveredWindow : g.NavWindow;
                    while (win && win->ScrollMax.y <= 0.0f && win->ParentWindow) {
                        win = win->ParentWindow;
                    }
                    if (win && win->ScrollMax.y > 0.0f) {
                        float scroll_step = ((float)ry / 32767.0f) * 14.0f * ui_scale;
                        float new_scroll = win->Scroll.y + scroll_step;
                        if (new_scroll < 0.0f) new_scroll = 0.0f;
                        if (new_scroll > win->ScrollMax.y) new_scroll = win->ScrollMax.y;
                        win->Scroll.y = new_scroll;
                    }
                }
            }
        }

        t_icons[0] = G.tex_icon_dumper;
        t_icons[1] = G.tex_icon_manager;
        t_icons[2] = G.tex_icon_editor;
        t_icons[3] = G.tex_icon_dumpdl;
        t_icons[4] = G.tex_icon_asset;
        t_icons[5] = G.tex_icon_settings;

        // -- ROOT WINDOW --
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(screen_w, screen_h));
        ImGui::Begin("Root", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);

        // -- HEADER --
        ImGui::BeginChild("Header", ImVec2(screen_w, header_h), true, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);
        {
            ImDrawList* hdl = ImGui::GetWindowDrawList();
            float title_cy = (header_h / 2.0f) - (22.0f * ui_scale);

            ImGui::SetCursorPos(ImVec2(20.0f * ui_scale, title_cy));
            if (f_title) ImGui::PushFont(f_title);
            ImGui::TextColored(G.accent, "Emulanders");
            ImGui::SameLine(0, 8.0f * ui_scale);
            ImGui::Text("Portal");
            if (f_title) ImGui::PopFont();

            ImGui::SameLine(0, 10.0f * ui_scale);
            ImGui::SetCursorPosY((header_h / 2.0f) + (5.0f * ui_scale));
            if (f_small) ImGui::PushFont(f_small);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "v0.9.2");
            if (f_small) ImGui::PopFont();

            float info_cy = (header_h / 2.0f) - (11.0f * ui_scale);
            ImVec2 ip_sz = ImGui::CalcTextSize(G.ip_address);
            ImGui::SetCursorPos(ImVec2((screen_w - ip_sz.x) / 2.0f, info_cy));
            ImGui::Text("%s", G.ip_address);

            char pct_str[16];
            snprintf(pct_str, sizeof(pct_str), "%d%%", G.battery_charge);
            ImVec2 time_sz = ImGui::CalcTextSize(G.time_str);
            ImVec2 pipe_sz = ImGui::CalcTextSize("  |  ");
            float bat_w = 30.0f * ui_scale;
            float bat_h = 15.0f * ui_scale;
            float bat_spacing = 8.0f * ui_scale;
            ImVec2 pct_sz = ImGui::CalcTextSize(pct_str);
            float right_total_w = time_sz.x + pipe_sz.x + bat_w + bat_spacing + pct_sz.x + 20.0f * ui_scale;
            float current_x = screen_w - right_total_w;

            ImGui::SetCursorPos(ImVec2(current_x, info_cy));
            ImGui::Text("%s", G.time_str);
            current_x += time_sz.x;

            ImGui::SetCursorPos(ImVec2(current_x, info_cy));
            ImGui::Text("  |  ");
            current_x += pipe_sz.x;

            ImVec2 hdr_origin = ImGui::GetWindowPos();
            ImVec2 bat_pos = ImVec2(hdr_origin.x + current_x, hdr_origin.y + info_cy + 4.0f * ui_scale);
            DrawBatteryIcon(hdl, bat_pos, ImVec2(bat_w, bat_h), G.battery_charge / 100.0f, G.accent);
            current_x += bat_w + bat_spacing;

            ImGui::SetCursorPos(ImVec2(current_x, info_cy));
            ImGui::Text("%s", pct_str);
        }
        ImGui::EndChild();

        // -- CONTENT --
        if (G.in_menu) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::BeginChild("Menu", ImVec2(screen_w, content_h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGuiContext& g_menu = *GImGui;
            if (G.menu_needs_nav_focus || g_menu.NavId == 0 || g_menu.NavWindow != ImGui::GetCurrentWindow()) {
                ImGui::SetWindowFocus();
                g_menu.NavCursorVisible = true;
            }

            float item_h = content_h / 6.0f;
            float icon_sz = item_h * 0.9f;

            auto DrawMenuOption = [&](const char* label, int idx, GLuint tex) {
                bool sel = (G.selected_menu == idx);
                ImVec2 p = ImGui::GetCursorScreenPos();

                ImGui::PushID(idx);
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
                if (f_large) ImGui::PushFont(f_large);
                if (sel) {
                    ImGuiContext& g = *GImGui;
                    if (G.menu_needs_nav_focus || g.NavId == 0 || g.NavWindow != ImGui::GetCurrentWindow()) {
                        ImGui::SetKeyboardFocusHere();
                        ImGui::SetItemDefaultFocus();
                        g.NavCursorVisible = true;
                    }
                }
                if (ImGui::Selectable("##opt", sel, 0, ImVec2(screen_w, item_h))) {
                    G.selected_menu = idx;
                    G.in_menu = false;
                    just_entered_content = true;
                }
                if (ImGui::IsItemFocused()) G.selected_menu = idx;
                if (idx == G.selected_menu) ImGui::SetItemDefaultFocus();
                ImGui::PopID();

                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 label_size = ImGui::CalcTextSize(label);
                float total_w = label_size.x + (tex ? icon_sz + 30 * ui_scale : 0);
                ImVec2 start_p = ImVec2(p.x + (screen_w - total_w) / 2.0f, p.y + (item_h - label_size.y) / 2.0f);
                draw->AddText(start_p, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), label);
                if (tex) {
                    draw->AddImage((void*)(intptr_t)tex,
                                   ImVec2(start_p.x + label_size.x + 30 * ui_scale, p.y + (item_h - icon_sz) / 2.0f),
                                   ImVec2(start_p.x + label_size.x + 30 * ui_scale + icon_sz, p.y + (item_h + icon_sz) / 2.0f));
                }
                if (f_large) ImGui::PopFont();
                ImGui::PopStyleVar();
                if (sel) draw->AddRectFilled(p, ImVec2(p.x + 12.0f * ui_scale, p.y + item_h), ImColor(G.accent));
            };

            DrawMenuOption(TR("Skylander Dumper", "Dumper de Skylanders"), 0, G.tex_icon_dumper);
            DrawMenuOption(TR("File Manager & Dump Cloning", "Gerenciador de Arquivos"), 1, G.tex_icon_manager);
            DrawMenuOption(TR("Figure Image Editor", "Editor de Imagens"), 2, G.tex_icon_editor);
            DrawMenuOption(TR("Dump Downloader", "Download de Dumps"), 3, G.tex_icon_dumpdl);
            DrawMenuOption(TR("Asset Downloader", "Download de Assets"), 4, G.tex_icon_asset);
            DrawMenuOption(TR("Settings", "Configurações"), 5, G.tex_icon_settings);

            if (G.menu_needs_nav_focus) {
                G.menu_needs_nav_focus = false;
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();
        } else {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(100.0f * ui_scale, 0.0f));
            ImGui::BeginChild("Content", ImVec2(screen_w, content_h),
                              ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);

            if (just_entered_content) { ImGui::SetWindowFocus(); }

            float header_area_h = content_h / 6.0f;
            ImGui::BeginChild("ScreenHeader", ImVec2(0, header_area_h), false, ImGuiWindowFlags_NoNav);
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetWindowPos();
                dl->AddRectFilled(p, ImVec2(p.x + ImGui::GetContentRegionAvail().x + 200 * ui_scale, p.y + header_area_h),
                                  ImColor(0.08f, 0.08f, 0.08f, 1.0f));

                if (f_large) ImGui::PushFont(f_large);
                const char* cur_title = "";
                switch (G.selected_menu) {
                    case 0: cur_title = TR("Skylander Dumper", "Dumper de Skylanders"); break;
                    case 1: cur_title = TR("File Manager & Dump Cloning", "Gerenciador de Arquivos"); break;
                    case 2: cur_title = TR("Figure Image Editor", "Editor de Imagens"); break;
                    case 3: cur_title = TR("Dump Downloader", "Download de Dumps"); break;
                    case 4: cur_title = TR("Asset Downloader", "Download de Assets"); break;
                    case 5: cur_title = TR("Settings", "Configurações"); break;
                }
                ImVec2 tsize = ImGui::CalcTextSize(cur_title);
                float icon_sz = header_area_h * 0.90f;
                float total_w = tsize.x + (t_icons[G.selected_menu] ? icon_sz + 30 * ui_scale : 0);

                ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x - total_w) / 2.0f,
                                           (header_area_h - tsize.y) / 2.0f));
                ImGui::TextColored(G.accent, "%s", cur_title);

                if (t_icons[G.selected_menu]) {
                    ImGui::SameLine(0, 30 * ui_scale);
                    ImGui::SetCursorPosY((header_area_h - icon_sz) / 2.0f);
                    ImGui::Image((void*)(intptr_t)t_icons[G.selected_menu], ImVec2(icon_sz, icon_sz));
                }
                if (f_large) ImGui::PopFont();
            }
            ImGui::EndChild();
            ImGui::Separator();

            switch (G.selected_menu) {
                case 0: RenderScreenDumper(screen_w, content_h, ui_scale, just_entered_content, f_large, f_small); break;
                case 1: RenderScreenFileManager(screen_w, content_h, ui_scale, just_entered_content, f_large, f_small); break;
                case 2: RenderScreenImageEditor(screen_w, content_h, ui_scale, just_entered_content, f_large, f_small); break;
                case 3: case 4: RenderScreenDownloaders(screen_w, content_h, ui_scale, just_entered_content, f_large, f_small); break;
                case 5: RenderScreenSettings(screen_w, content_h, ui_scale, just_entered_content, f_large, f_small); break;
            }

            if (just_entered_content) just_entered_content = false;
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        // -- Download finished popup --
        if (G.dl_finished_popup) {
            ImGui::OpenPopup("DownloadDone");
            G.dl_finished_popup = false;
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
        }
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f * ui_scale);
        ImVec2 popup_size(700 * ui_scale, 300 * ui_scale);
        ImGui::SetNextWindowPos(ImVec2((screen_w - popup_size.x) * 0.5f, (screen_h - popup_size.y) * 0.5f));
        ImGui::SetNextWindowSize(popup_size);
        if (ImGui::BeginPopupModal("DownloadDone", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetWindowPos();
            float head_h = 65 * ui_scale;
            dl->AddRectFilled(p, ImVec2(p.x + popup_size.x, p.y + head_h), ImColor(G.accent));
            ImGui::SetCursorPos(ImVec2(30 * ui_scale, 18 * ui_scale));
            if (f_large) ImGui::PushFont(f_large);
            ImGui::TextColored(ImVec4(1,1,1,1), "%s", TR("Download Complete", "Download Concluído"));
            if (f_large) ImGui::PopFont();
            ImGui::SetCursorPos(ImVec2(30 * ui_scale, 80 * ui_scale));
            ImGui::PushTextWrapPos(popup_size.x - 60 * ui_scale);
            ImGui::Text("%s", G.status_msg);
            ImGui::PopTextWrapPos();
            ImGui::SetCursorPos(ImVec2((popup_size.x - 200 * ui_scale) * 0.5f, popup_size.y - 80 * ui_scale));
            if (ImGui::Button(TR("DISMISS", "FECHAR"), ImVec2(200 * ui_scale, 55 * ui_scale)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // -- FOOTER --
        ImGui::SetCursorPos(ImVec2(0, screen_h - footer_h));
        ImGui::BeginChild("Footer", ImVec2(screen_w, footer_h), true, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);
        if (f_small) ImGui::PushFont(f_small);
        ImGui::SetCursorPosY(footer_h / 2.0f - 9.0f * ui_scale);
        ImGui::SetCursorPosX(30.0f * ui_scale);
        const char* footer_text = TR(
            "[D-Pad] Navigate  |  [A] Confirm  |  [B] Back  |  [+] Exit",
            "[D-Pad] Navegar  |  [A] Confirmar  |  [B] Voltar  |  [+] Sair"
        );
        if (G.selected_menu == 2 && G.editor_state == EditorState::CROP_EDIT) {
            footer_text = TR(
                "[D-Pad / L-Stick] Move  |  [R-Stick / L / R] Zoom  |  [A] Save  |  [X] Reset  |  [B] Back",
                "[D-Pad / L-Stick] Mover  |  [R-Stick / L / R] Zoom  |  [A] Salvar  |  [X] Redefinir  |  [B] Voltar"
            );
        } else if (G.selected_menu == 1 || G.selected_menu == 2 || G.selected_menu == 3 || G.selected_menu == 5) {
            footer_text = TR(
                "[D-Pad] Navigate  |  [R-Stick / Touch] Scroll  |  [A] Confirm  |  [B] Back  |  [+] Exit",
                "[D-Pad] Navegar  |  [R-Stick / Touch] Rolar  |  [A] Confirmar  |  [B] Voltar  |  [+] Sair"
            );
        }
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", footer_text);
        if (f_small) ImGui::PopFont();
        ImGui::EndChild();

        ImGui::End();

        // -- Render --
        ImGui::Render();
        glViewport(0, 0, win_w, win_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // -- Cleanup --
    G.app_exiting.store(true);
    G.bg_thread.join();

    glDeleteTextures(1, &G.tex_icon_dumper);
    glDeleteTextures(1, &G.tex_icon_manager);
    glDeleteTextures(1, &G.tex_icon_editor);
    glDeleteTextures(1, &G.tex_icon_asset);
    glDeleteTextures(1, &G.tex_icon_dumpdl);
    glDeleteTextures(1, &G.tex_icon_settings);
    if (G.editor_photo_tex) glDeleteTextures(1, &G.editor_photo_tex);

    if (G.controller) SDL_GameControllerClose(G.controller);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    emulandersExit();
    nifmExit();
    psmExit();
    socketExit();
    plExit();
    return 0;
}
