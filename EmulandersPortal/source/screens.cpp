#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

#include <sys/stat.h>
#include <unistd.h>
#include <switch.h>

#include <curl/curl.h>
#include <minizip/unzip.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_internal.h"

#include "globals.hpp"
#include "PortalAlgos.hpp"
#include "emulanders_ipc.h"
#include "utils/path_utils.hpp"
#include "utils/ui_helpers.hpp"

namespace fs = std::filesystem;

static const char* OVERLAY_FAVORITES_FILE = "sdmc:/emulanders/overlay/favorites.txt";
static const std::string SCANS_DIR = std::string(FIGURES_DIR) + SCANS_SUBDIR;

static void LoadOverlayFavoritesList(std::vector<FileInfo>& target_list) {
    for (auto& item : target_list) if (item.tex) { glDeleteTextures(1, &item.tex); item.tex = 0; }
    target_list.clear();

    std::ifstream f(OVERLAY_FAVORITES_FILE);
    if (f.is_open()) {
        std::string line;
        std::error_code ec;
        while (std::getline(f, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' '))
                line.pop_back();
            if (!line.empty() && fs::exists(line, ec) && !fs::is_directory(line, ec)) {
                FileInfo fi;
                fi.name = "★ " + GetFilename(line);
                fi.path = line;
                fi.is_dir = false;
                fi.size = fs::file_size(line, ec);
                fi.tex = 0;
                fi.tex_checked = false;
                target_list.push_back(fi);
            }
        }
        f.close();
    }
}

// ============================================================
// UI Helpers
// ============================================================

GLuint LoadTextureFromFile(const char* filename, int* out_w, int* out_h) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) return 0;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    int mode = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (out_w) *out_w = surface->w;
    if (out_h) *out_h = surface->h;
    SDL_FreeSurface(surface);
    return texture;
}

GLuint LoadTextureFromMemory(const void* data, size_t size, int* out_w, int* out_h) {
    SDL_RWops* rw = SDL_RWFromConstMem(data, (int)size);
    if (!rw) return 0;
    SDL_Surface* surface = IMG_Load_RW(rw, 1);
    if (!surface) return 0;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    int mode = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (out_w) *out_w = surface->w;
    if (out_h) *out_h = surface->h;
    SDL_FreeSurface(surface);
    return texture;
}

void DrawBatteryIcon(ImDrawList* dl, ImVec2 pos, ImVec2 size, float pct, const ImVec4& accent) {
    ImU32 outline = ImColor(accent.x, accent.y, accent.z, 1.0f);
    ImU32 fill = (pct < 0.2f) ? (ImU32)IM_COL32(215, 50, 50, 255) : (ImU32)ImColor(accent.x, accent.y, accent.z, 1.0f);
    float body_w = size.x * 0.92f;

    dl->AddRect(pos, ImVec2(pos.x + body_w, pos.y + size.y), outline, 2.0f, 0, 1.5f);
    dl->AddRectFilled(ImVec2(pos.x + body_w, pos.y + size.y * 0.25f),
                      ImVec2(pos.x + size.x, pos.y + size.y * 0.75f), outline, 1.0f);
    float fill_w = (body_w - 4.0f) * pct;
    if (fill_w > 1.0f)
        dl->AddRectFilled(ImVec2(pos.x + 2.0f, pos.y + 2.0f),
                          ImVec2(pos.x + 2.0f + fill_w, pos.y + size.y - 2.0f), fill, 1.0f);
}

void SetupImGuiStyle(float scale, const ImVec4& accent, ImVec4& accent_hover, ImVec4& accent_active) {
    accent_hover  = ImVec4(accent.x + 0.1f, accent.y + 0.1f, accent.z + 0.1f, 1.0f);
    accent_active = ImVec4(accent.x - 0.1f, accent.y - 0.1f, accent.z - 0.1f, 1.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 6.0f * scale;
    style.PopupRounding     = 8.0f * scale;
    style.ScrollbarRounding = 4.0f * scale;
    style.ScrollbarSize     = 0.0f;
    style.GrabRounding      = 6.0f * scale;
    style.WindowPadding     = ImVec2(0.0f, 0.0f);
    style.FramePadding      = ImVec2(16.0f * scale, 10.0f * scale);
    style.ItemSpacing       = ImVec2(12.0f * scale, 14.0f * scale);
    style.ItemInnerSpacing  = ImVec2(8.0f * scale, 8.0f * scale);
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f * scale;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]           = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    c[ImGuiCol_TextDisabled]   = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    c[ImGuiCol_WindowBg]       = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    c[ImGuiCol_ChildBg]        = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    c[ImGuiCol_PopupBg]        = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);
    c[ImGuiCol_Border]         = accent;
    c[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    c[ImGuiCol_CheckMark]      = accent;
    c[ImGuiCol_SliderGrab]     = accent;
    c[ImGuiCol_Button]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    c[ImGuiCol_ButtonHovered]  = accent_hover;
    c[ImGuiCol_ButtonActive]   = accent_active;
    c[ImGuiCol_Header]         = accent;
    c[ImGuiCol_HeaderHovered]  = accent_hover;
    c[ImGuiCol_HeaderActive]   = accent_active;
    c[ImGuiCol_Separator]      = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    c[ImGuiCol_TableRowBg]     = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    c[ImGuiCol_TableRowBgAlt]  = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
    c[ImGuiCol_TitleBg]        = ImVec4(0.0f, 0.50f, 0.65f, 1.0f);
    c[ImGuiCol_TitleBgActive]  = accent;
    c[ImGuiCol_NavHighlight]   = accent;
}

// ============================================================
// Screen: Skylander Dumper
// ============================================================

static GLuint scan_texture = 0;

void RenderScreenDumper(float screen_w, float content_h, float ui_scale, bool& just_entered_content, ImFont* f_large, ImFont* f_small) {
    (void)f_small;
    if (G.scanned_image_ready.exchange(false)) {
        if (scan_texture) { glDeleteTextures(1, &scan_texture); scan_texture = 0; }
        scan_texture = LoadTextureFromFile(G.scanned_image_path.c_str());
    }

    if (G.scan_state == ScanState::IDLE) {
        float child_w = 900 * ui_scale;
        float child_h = 300 * ui_scale;
        float avail_h = ImGui::GetContentRegionAvail().y;

        ImGui::SetCursorPosX((screen_w - child_w) / 2.0f - ImGui::GetCursorPosX() + ImGui::GetStyle().WindowPadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail_h - child_h) / 2.0f);

        ImGui::BeginChild("DumperIdle", ImVec2(child_w, child_h), false, ImGuiWindowFlags_NoScrollbar);
        ImGui::Spacing();

        auto CenterText = [&](const char* text, ImVec4 color) {
            ImGui::PushTextWrapPos(child_w);
            ImVec2 sz = ImGui::CalcTextSize(text, NULL, false, child_w);
            ImGui::SetCursorPosX((child_w - sz.x) / 2.0f);
            ImGui::TextColored(color, "%s", text);
            ImGui::PopTextWrapPos();
        };

        CenterText(TR("Place a physical Skylander on the Right Joy-Con NFC reader.", "Coloque um Skylander físico no leitor NFC do Joy-Con direito."), ImVec4(0.8f, 0.8f, 0.8f, 1));
        ImGui::Spacing();
        CenterText(TR("Keep it still and centered on the analog stick.", "Mantenha-o parado e centralizado no analógico."), ImVec4(1, 0.8f, 0, 1));
        ImGui::Spacing();
        CenterText(TR("Note: Some third-party controllers only support Amiibo, not Skylander.", "Nota: Alguns controles paralelos suportam apenas Amiibo, não Skylander."), ImVec4(0.6f, 0.6f, 0.6f, 1));

        ImGui::Spacing(); ImGui::Spacing();
        float btn_w = 350 * ui_scale;
        ImGui::SetCursorPosX((child_w - btn_w) / 2.0f);
        if (just_entered_content) ImGui::SetKeyboardFocusHere();
        if (ImGui::Button(TR("Start Scanning", "Iniciar Leitura"), ImVec2(btn_w, 60 * ui_scale))) {
            if (emulandersIsInitialized()) emulandersSetEmulationStatus(EmulationStatus::Off);
            G.scan_state = ScanState::PREPARING;
            G.scan_progress_blocks = 0;
            G.scan_error_msg.clear();

            auto& bg = G.bg_thread;
            bg.launch([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));

                if (R_FAILED(nfcMfInitialize())) {
                    G.scan_error_msg = "Failed to initialize NFC.";
                    G.scan_state = ScanState::ERROR;
                    return;
                }

                NfcDeviceHandle h = {};
                s32 total = 0;
                nfcMfListDevices(&total, &h, 1);
                nfcMfStartDetection(&h);
                G.scan_state = ScanState::WAITING_NFC;

                bool found = false;
                for (int i = 0; i < 300; i++) {
                    NfcMifareDeviceState ds;
                    nfcMfGetDeviceState(&h, &ds);
                    if (ds == NfcMifareDeviceState_TagFound) { found = true; break; }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (!found) {
                    G.scan_error_msg = "Timeout: No tag detected.";
                    G.scan_state = ScanState::ERROR;
                    nfcMfStopDetection(&h);
                    nfcMfExit();
                    return;
                }

                G.scan_state = ScanState::READING_BLOCKS;
                NfcTagInfo tag = {};
                nfcMfGetTagInfo(&h, &tag);

                uint8_t dump[DUMP_FILE_SIZE] = {0};
                uint8_t uid[4] = {tag.uid.uid[0], tag.uid.uid[1], tag.uid.uid[2], tag.uid.uid[3]};

                for (size_t s = 0; s < DUMP_SECTORS; s++) {
                    uint8_t skey[6];
                    Runes::CalcKeyA(uid, (uint8_t)s, skey);
                    NfcMifareReadBlockParameter rp[4] = {};
                    NfcMifareReadBlockData rd[4] = {};
                    for (int b = 0; b < 4; b++) {
                        rp[b].sector_number = (int)s * 4 + b;
                        memcpy(rp[b].sector_key.sector_key, skey, 6);
                        rp[b].sector_key.mifare_command = NfcMifareCommand_AuthA;
                        rp[b].sector_key.unknown = 1;
                    }
                    if (R_SUCCEEDED(nfcMfReadMifare(&h, rd, rp, 4))) {
                        for (int b = 0; b < 4; b++)
                            memcpy(&dump[(s * 4 + b) * DUMP_BLOCK_SIZE], rd[b].data, DUMP_BLOCK_SIZE);
                        G.scan_progress_blocks = (int)(s + 1) * 4;
                        if (s == 0) {
                            u32 cid = dump[16] | (dump[17] << 8) | (dump[18] << 16);
                            u16 vid = dump[28] | (dump[29] << 8);
                            G.scan_suggested_name = LookupSkylanderName(cid, vid);
                            char ipath[MAX_PATH];
                            snprintf(ipath, MAX_PATH, "sdmc:/emulanders/assets/%06X_%04X.png", cid, vid);
                            if (!fs::exists(ipath))
                                snprintf(ipath, MAX_PATH, "sdmc:/emulanders/assets/%06X.png", cid);
                            G.scanned_image_path = ipath;
                            G.scanned_image_ready = true;
                        }
                    } else {
                        char err[128];
                        snprintf(err, sizeof(err), "Read fail at sector %zu", s);
                        G.scan_error_msg = err;
                        G.scan_state = ScanState::ERROR;
                        break;
                    }
                }

                nfcMfStopDetection(&h);
                nfcMfExit();

                if (G.scan_state != ScanState::ERROR) {
                    memcpy(G.last_scan_data, dump, DUMP_FILE_SIZE);
                    G.scan_state = ScanState::WAITING_CONFIRM;
                }
            });
        }
        ImGui::EndChild();

    } else if (G.scan_state == ScanState::PREPARING) {
        ImGui::Text("%s", TR("Initializing NFC hardware...", "Inicializando hardware NFC..."));
        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(400 * ui_scale, 20 * ui_scale));

    } else if (G.scan_state == ScanState::WAITING_NFC) {
        ImGui::TextColored(G.accent, "%s", TR("Waiting for Skylander...", "Aguardando Skylander..."));
        ImGui::Text("%s", TR("Place the figure on the NFC reader now.", "Coloque o boneco no leitor NFC agora."));

    } else if (G.scan_state == ScanState::READING_BLOCKS) {
        float cur_y = ImGui::GetCursorPosY();
        ImGui::BeginGroup();
        ImGui::Text(TR("Reading data... %d/64 blocks", "Lendo dados... %d/64 blocos"), G.scan_progress_blocks);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, G.accent);
        ImGui::ProgressBar(G.scan_progress_blocks / 64.0f, ImVec2(400 * ui_scale, 20 * ui_scale));
        ImGui::PopStyleColor();
        ImGui::EndGroup();
        if (scan_texture) {
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 325 * ui_scale);
            ImGui::SetCursorPosY(cur_y - 20 * ui_scale);
            ImGui::BeginGroup();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "%s", TR("Skylander detected!", "Skylander detectado!"));
            ImGui::Image((void*)(intptr_t)scan_texture, ImVec2(175 * ui_scale, 275 * ui_scale));
            ImGui::EndGroup();
        }

    } else if (G.scan_state == ScanState::WAITING_CONFIRM) {
        float cur_y = ImGui::GetCursorPosY();
        ImGui::BeginGroup();
        ImGui::Text("%s: %s", TR("Identified", "Identificado"), G.scan_suggested_name.c_str());
        ImGui::Spacing();

        auto SaveDump = [](const std::string& name) {
            std::error_code ec;
            fs::create_directories(std::string(FIGURES_DIR) + SCANS_SUBDIR, ec);
            if (ec) return;
            std::string fname = GetUniqueFileName(std::string(FIGURES_DIR) + SCANS_SUBDIR,
                                                   SanitizeFileName(name) + ".dump");
            std::string p = std::string(FIGURES_DIR) + SCANS_SUBDIR + fname;
            std::ofstream out(p, std::ios::binary);
            out.write((char*)G.last_scan_data, DUMP_FILE_SIZE);
            out.close();
            chmod(p.c_str(), 0777);
            G.scan_state = ScanState::DONE;
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
        };

        if (ImGui::Button(TR("Save with Official Name", "Salvar com Nome Oficial"), ImVec2(300 * ui_scale, 50 * ui_scale))) {
            SaveDump(G.scan_suggested_name);
        }
        if (ImGui::Button(TR("Save with Custom Name", "Salvar com Nome Personalizado"), ImVec2(300 * ui_scale, 50 * ui_scale))) {
            std::string res = ShowKeyboard("", TR("Enter filename", "Digite o nome do arquivo"));
            if (!res.empty()) {
                SaveDump(res);
            } else {
                G.show_blank_name_warning = true;
            }
        }
        if (ImGui::Button(TR("Cancel", "Cancelar"), ImVec2(300 * ui_scale, 50 * ui_scale)))
            G.scan_state = ScanState::IDLE;
        ImGui::EndGroup();

        if (scan_texture) {
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 325 * ui_scale);
            ImGui::SetCursorPosY(cur_y - 20 * ui_scale);
            ImGui::BeginGroup();
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "%s", TR("Skylander detected!", "Skylander detectado!"));
            ImGui::Image((void*)(intptr_t)scan_texture, ImVec2(175 * ui_scale, 275 * ui_scale));
            ImGui::EndGroup();
        }

    } else if (G.scan_state == ScanState::DONE) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", TR("Dump saved successfully!", "Dump salvo com sucesso!"));
        ImGui::Spacing();
        if (ImGui::Button(TR("Scan Another", "Ler Outro"), ImVec2(250 * ui_scale, 50 * ui_scale)))
            G.scan_state = ScanState::IDLE;

    } else if (G.scan_state == ScanState::ERROR) {
        ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "%s: %s", TR("Scan Error", "Erro de Leitura"), G.scan_error_msg.c_str());
        ImGui::Spacing();
        if (ImGui::Button(TR("Try Again", "Tentar Novamente"), ImVec2(250 * ui_scale, 50 * ui_scale)))
            G.scan_state = ScanState::IDLE;
    }

    if (G.show_blank_name_warning) {
        ImGui::OpenPopup("NameWarning");
        G.show_blank_name_warning = false;
    }
    if (ImGui::BeginPopupModal("NameWarning", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::Text("%s", TR("File name cannot be empty!", "O nome do arquivo não pode ser vazio!"));
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120 * ui_scale, 40 * ui_scale)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// ============================================================
// Screen: File Manager
// ============================================================

void RenderScreenFileManager(float screen_w, float content_h, float ui_scale, bool& just_entered_content, ImFont* f_large, ImFont* f_small) {
    (void)f_large; (void)f_small;
    static int selected_sd_idx = 0;
    static bool request_nav_focus = true;

    if (just_entered_content || G.file_manager_needs_focus) {
        selected_sd_idx = 0;
        request_nav_focus = true;
        G.file_manager_needs_focus = false;
    }

    if (G.file_manager_needs_refresh) {
        G.file_manager_needs_refresh = false;
        if (G.current_path == "<favorites>") {
            LoadOverlayFavoritesList(G.sd_files);
        } else {
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
        }
        selected_sd_idx = 0;
        request_nav_focus = true;
    }

    if (G.sd_files.empty()) {
        selected_sd_idx = -1;
    } else if (selected_sd_idx < 0) {
        selected_sd_idx = 0;
    } else if (selected_sd_idx >= (int)G.sd_files.size()) {
        selected_sd_idx = (int)G.sd_files.size() - 1;
    }

    if (G.current_path == "<favorites>") {
        ImGui::TextColored(G.accent, "Source: ★ Tesla Overlay Favorites (sdmc:/emulanders/overlay/favorites.txt)");
    } else {
        ImGui::TextColored(G.accent, "Path:");
        ImGui::SameLine(0, 8.0f * ui_scale);
        ImGui::Text("%s", G.current_path.c_str());
    }

    float avail_x = ImGui::GetContentRegionAvail().x;
    float btn_w_total = (180 + 210 + 210 + 24) * ui_scale;
    ImGui::SetCursorPosX(std::max(0.0f, (avail_x - btn_w_total) / 2.0f));

    if (ImGui::Button(TR("(L) Refresh", "(L) Atualizar"), ImVec2(180 * ui_scale, 45 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
        if (G.current_path == "<favorites>") {
            LoadOverlayFavoritesList(G.sd_files);
        } else {
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
        }
        selected_sd_idx = 0;
        request_nav_focus = true;
    }
    ImGui::SameLine(0, 12.0f * ui_scale);
    if (ImGui::Button(TR("(R) New Folder", "(R) Nova Pasta"), ImVec2(210 * ui_scale, 45 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1)) {
        if (G.current_path != "<favorites>") {
            std::string n = ShowKeyboard("NewFolder", TR("Folder name", "Nome da pasta"));
            if (!n.empty()) {
                std::string p = G.current_path + SanitizeFileName(n);
                mkdir(p.c_str(), 0777);
                chmod(p.c_str(), 0777);
                RefreshFileList(G.current_path, G.sd_files, G.current_path);
                selected_sd_idx = 0;
                request_nav_focus = true;
            }
        }
    }
    ImGui::SameLine(0, 12.0f * ui_scale);
    bool is_fav_view = (G.current_path == "<favorites>");
    if (is_fav_view) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
    if (G.sd_files.empty() && is_fav_view) ImGui::SetItemDefaultFocus();
    if (ImGui::Button(TR("[Y] ★ Favorites", "[Y] ★ Favoritos"), ImVec2(210 * ui_scale, 45 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft)) {
        G.current_path = "<favorites>";
        LoadOverlayFavoritesList(G.sd_files);
        selected_sd_idx = 0;
        request_nav_focus = true;
    }
    if (is_fav_view) ImGui::PopStyleColor();

    if (G.clipboard_action != ClipboardAction::NONE) {
        const char* act = (G.clipboard_action == ClipboardAction::COPY) ? "COPY" : "CUT";
        ImGui::TextColored(G.accent, "Clipboard [%s]: %s  |  (X) Paste  |  (Y) Clear", act, GetFilename(G.clipboard_path).c_str());
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp)) {
            std::string dest_name = GetUniqueFileName(G.current_path, G.clipboard_path);
            std::string dest = G.current_path + dest_name;
            std::error_code ec;
            if (G.clipboard_action == ClipboardAction::COPY)
                fs::copy(G.clipboard_path, dest, fs::copy_options::recursive, ec);
            else {
                fs::rename(G.clipboard_path, dest, ec);
                G.clipboard_action = ClipboardAction::NONE;
            }
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
            selected_sd_idx = 0;
            request_nav_focus = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft))
            G.clipboard_action = ClipboardAction::NONE;
    }

    float extra = (G.clipboard_action != ClipboardAction::NONE) ? 45.0f : 0.0f;
    float table_h = content_h - (content_h / 6.0f) - 130.0f * ui_scale - extra * ui_scale;
    if (just_entered_content || request_nav_focus) {
        ImGui::SetNextWindowFocus();
    }
    if (ImGui::BeginTable("Files", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH, ImVec2(0, table_h))) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 120 * ui_scale);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80 * ui_scale);
        ImGui::TableHeadersRow();

        int open_actions_idx = -1;
        int tex_loads_this_frame = 0;
        for (size_t i = 0; i < G.sd_files.size(); i++) {
            auto& file_item = G.sd_files[i];

            // Lazy thumbnail loader (max 4 per frame to ensure 60fps responsiveness)
            if (!file_item.is_dir && !file_item.tex_checked && tex_loads_this_frame < 4) {
                file_item.tex_checked = true;
                tex_loads_this_frame++;

                size_t dot = file_item.path.find_last_of('.');
                std::string adj_png = (dot != std::string::npos) ? file_item.path.substr(0, dot) + ".png" : "";
                std::error_code ec;
                if (!adj_png.empty() && fs::exists(adj_png, ec)) {
                    file_item.tex = LoadTextureFromFile(adj_png.c_str());
                } else if (file_item.size >= 0x20) {
                    std::ifstream in(file_item.path, std::ios::binary);
                    if (in.is_open()) {
                        uint8_t hdr[0x20] = {0};
                        in.read(reinterpret_cast<char*>(hdr), 0x20);
                        if (in.gcount() >= 0x20) {
                            u32 cid = hdr[16] | (hdr[17] << 8) | (hdr[18] << 16);
                            u16 vid = hdr[28] | (hdr[29] << 8);
                            char apath[256];
                            snprintf(apath, sizeof(apath), "sdmc:/emulanders/assets/%06X_%04X.png", cid, vid);
                            if (fs::exists(apath, ec)) {
                                file_item.tex = LoadTextureFromFile(apath);
                            } else {
                                snprintf(apath, sizeof(apath), "sdmc:/emulanders/assets/%06X.png", cid);
                                if (fs::exists(apath, ec)) {
                                    file_item.tex = LoadTextureFromFile(apath);
                                }
                            }
                        }
                    }
                }
            }

            ImGui::PushID((int)i);
            ImGui::TableNextRow(0, 36.0f * ui_scale);
            ImGui::TableSetColumnIndex(0);

            if (request_nav_focus && (int)i == selected_sd_idx) {
                ImGui::SetKeyboardFocusHere();
                ImGui::SetItemDefaultFocus();
                ImGuiContext& g = *GImGui;
                g.NavCursorVisible = true;
                request_nav_focus = false;
            }

            ImVec2 row_p = ImGui::GetCursorScreenPos();
            if (file_item.tex) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 30.0f * ui_scale);
            }

            bool clicked = ImGui::Selectable(file_item.name.c_str(), selected_sd_idx == (int)i,
                                              ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
            if (ImGui::IsItemFocused()) {
                selected_sd_idx = (int)i;
            }

            if (file_item.tex) {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                float th_w = 22.0f * ui_scale;
                float th_h = 32.0f * ui_scale;
                float off_y = (36.0f * ui_scale - th_h) * 0.5f;
                dl->AddImage((void*)(intptr_t)file_item.tex,
                             ImVec2(row_p.x + 2.0f * ui_scale, row_p.y + off_y),
                             ImVec2(row_p.x + 2.0f * ui_scale + th_w, row_p.y + off_y + th_h));
            }

            if (clicked) {
                selected_sd_idx = (int)i;
                if (file_item.name == "<- Go back 1 directory" || file_item.is_dir) {
                    RefreshFileList(file_item.path, G.sd_files, G.current_path);
                    selected_sd_idx = 0;
                    request_nav_focus = true;
                } else {
                    open_actions_idx = (int)i;
                }
            }

            ImGui::TableSetColumnIndex(1);
            if (!file_item.is_dir) {
                if (file_item.size > 1024*1024)
                    ImGui::Text("%.1f MB", file_item.size / (1024.0*1024.0));
                else if (file_item.size > 1024)
                    ImGui::Text("%.1f KB", file_item.size / 1024.0);
                else
                    ImGui::Text("%lu B", (unsigned long)file_item.size);
            }
            ImGui::TableSetColumnIndex(2);
            bool is_dump_entry = (!file_item.is_dir && (file_item.size == 1024 ||
                                  (file_item.name.size() >= 5 && strcasecmp(file_item.name.c_str() + file_item.name.size() - 5, ".dump") == 0)));
            ImGui::Text("%s", file_item.is_dir ? "DIR" : (is_dump_entry ? "DUMP" : "FILE"));
            ImGui::PopID();
        }

        if (open_actions_idx != -1) {
            ImGui::OpenPopup("FileActions");
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25 * ui_scale, 20 * ui_scale));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15 * ui_scale, 15 * ui_scale));
        if (ImGui::BeginPopup("FileActions") && selected_sd_idx >= 0 && selected_sd_idx < (int)G.sd_files.size()) {
            auto& sel = G.sd_files[selected_sd_idx];

            ImGui::TextColored(G.accent, "%s: %s", TR("FILE", "ARQUIVO"), sel.name.c_str());
            ImGui::Separator();
            ImGui::Spacing();

            if (!sel.is_dir) {
                if (ImGui::MenuItem(TR("Clone (1:1 Exact)", "Clonar (1:1 Exato)"))) {
                    size_t dot = sel.name.find_last_of('.');
                    std::string stem = (dot != std::string::npos) ? sel.name.substr(0, dot) : sel.name;
                    std::string ext = (dot != std::string::npos) ? sel.name.substr(dot) : ".dump";
                    std::string default_clone = stem + "_clone" + ext;
                    std::string new_name = ShowKeyboard(default_clone, TR("Clone file name", "Nome do clone"));
                    if (!new_name.empty()) {
                        if (new_name.find('.') == std::string::npos) new_name += ext;
                        std::string clean = SanitizeFileName(new_name);
                        std::string dest = G.current_path + clean;
                        if (Runes::CloneDump1to1(sel.path, dest)) {
                            RefreshFileList(G.current_path, G.sd_files, G.current_path);
                            selected_sd_idx = 0;
                            request_nav_focus = true;
                        }
                    }
                }

                bool is_dump = (sel.size == 1024 ||
                                (sel.name.size() >= 5 && strcasecmp(sel.name.c_str() + sel.name.size() - 5, ".dump") == 0) ||
                                (sel.name.size() >= 4 && strcasecmp(sel.name.c_str() + sel.name.size() - 4, ".bin") == 0));

                if (is_dump && ImGui::MenuItem(TR("Clone (New Identity)", "Clonar (Nova Identidade)"))) {
                    G.clone_source_path = sel.path;
                    if (Runes::CheckIsImaginatorOrCrystal(sel.path)) {
                        G.show_clone_warning = true;
                    } else {
                        size_t dot = sel.name.find_last_of('.');
                        std::string stem = (dot != std::string::npos) ? sel.name.substr(0, dot) : sel.name;
                        std::string ext = (dot != std::string::npos) ? sel.name.substr(dot) : ".dump";
                        std::string default_clone = stem + "_new_id" + ext;
                        std::string new_name = ShowKeyboard(default_clone, TR("New Identity clone name", "Nome do clone"));
                        if (!new_name.empty()) {
                            if (new_name.find('.') == std::string::npos) new_name += ext;
                            std::string clean = SanitizeFileName(new_name);
                            std::string dest = G.current_path + clean;
                            if (Runes::CloneDumpNewIdentity(sel.path, dest)) {
                                RefreshFileList(G.current_path, G.sd_files, G.current_path);
                                selected_sd_idx = 0;
                                request_nav_focus = true;
                            }
                        }
                    }
                }

                if (is_dump && ImGui::MenuItem(TR("Edit Artwork (Image Editor)", "Editar Foto (Editor de Imagens)"))) {
                    G.editor_dump_path = sel.path;
                    G.editor_state = EditorState::SELECT_PHOTO;
                    G.selected_menu = 2;
                    G.in_menu = false;
                    just_entered_content = true;
                }
            }

            if (ImGui::MenuItem(TR("Copy", "Copiar"))) {
                G.clipboard_path = sel.path;
                G.clipboard_action = ClipboardAction::COPY;
            }
            if (ImGui::MenuItem(TR("Cut", "Recortar"))) {
                G.clipboard_path = sel.path;
                G.clipboard_action = ClipboardAction::CUT;
            }
            if (ImGui::MenuItem(TR("Rename", "Renomear"))) {
                std::string new_name = ShowKeyboard(sel.name, TR("New name", "Novo nome"));
                if (!new_name.empty() && new_name != sel.name) {
                    std::string dest = G.current_path + SanitizeFileName(new_name);
                    std::error_code ec;
                    fs::rename(sel.path, dest, ec);
                    RefreshFileList(G.current_path, G.sd_files, G.current_path);
                    selected_sd_idx = 0;
                    request_nav_focus = true;
                }
            }
            if (ImGui::MenuItem(TR("Delete", "Excluir"))) {
                G.file_to_delete = sel.path;
                G.show_delete_confirm = true;
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::EndTable();
    }

    if (G.show_delete_confirm) {
        ImGui::OpenPopup("DeleteConfirm");
        G.show_delete_confirm = false;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40 * ui_scale, 30 * ui_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    if (ImGui::BeginPopupModal("DeleteConfirm", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos();
        float head_h = 52 * ui_scale;
        dl->AddRectFilled(p0, ImVec2(p0.x + ImGui::GetWindowWidth(), p0.y + head_h), ImColor(G.accent.x, G.accent.y, G.accent.z, 1.0f), 12.0f, ImDrawFlags_RoundCornersTop);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * ui_scale);
        ImGui::TextColored(ImVec4(1,1,1,1), "%s", TR("CONFIRM DELETION", "CONFIRMAR EXCLUSÃO"));
        ImGui::Separator();
        ImGui::Spacing(); ImGui::Spacing();

        ImGui::Text("%s", TR("Are you sure you want to delete this?", "Tem certeza de que deseja excluir este item?"));
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", GetFilename(G.file_to_delete).c_str());
        ImGui::Spacing(); ImGui::Spacing();

        float b_w = 180 * ui_scale;
        if (ImGui::Button(TR("Yes, Delete", "Sim, Excluir"), ImVec2(b_w, 50 * ui_scale))) {
            G.delete_target_name = GetFilename(G.file_to_delete);
            size_t dot = G.file_to_delete.find_last_of('.');
            if (dot != std::string::npos) {
                std::string art = G.file_to_delete.substr(0, dot) + ".png";
                std::error_code ec;
                fs::remove(art, ec);
            }
            RobustDelete(G.file_to_delete);
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
            selected_sd_idx = 0;
            request_nav_focus = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(TR("Cancel", "Cancelar"), ImVec2(b_w, 50 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown)) {
            ImGui::CloseCurrentPopup();
            request_nav_focus = true;
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);

    if (G.is_deleting && !ImGui::IsPopupOpen("DeletingInProgress")) {
        ImGui::OpenPopup("DeletingInProgress");
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40 * ui_scale, 30 * ui_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    if (ImGui::BeginPopupModal("DeletingInProgress", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos();
        float head_h = 52 * ui_scale;
        dl->AddRectFilled(p0, ImVec2(p0.x + ImGui::GetWindowWidth(), p0.y + head_h), ImColor(G.accent.x, G.accent.y, G.accent.z, 1.0f), 12.0f, ImDrawFlags_RoundCornersTop);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * ui_scale);
        ImGui::TextColored(ImVec4(1,1,1,1), "%s", TR("SYSTEM OPERATION", "OPERAÇÃO DO SISTEMA"));
        ImGui::Separator();
        ImGui::Spacing(); ImGui::Spacing();

        ImGui::Text(TR("Deleting: %s", "Excluindo: %s"), G.delete_target_name.c_str());
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, G.accent);
        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(400 * ui_scale, 25 * ui_scale), TR("Processing storage...", "Processando armazenamento..."));
        ImGui::PopStyleColor();

        if (!G.is_deleting) {
            ImGui::CloseCurrentPopup();
            RefreshFileList(G.current_path, G.sd_files, G.current_path);
            selected_sd_idx = 0;
            request_nav_focus = true;
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);

    if (G.show_clone_warning) {
        ImGui::OpenPopup("CloneWarning");
        G.show_clone_warning = false;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40 * ui_scale, 30 * ui_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    if (ImGui::BeginPopupModal("CloneWarning", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos();
        float head_h = 52 * ui_scale;
        dl->AddRectFilled(p0, ImVec2(p0.x + ImGui::GetWindowWidth(), p0.y + head_h), ImColor(G.accent.x, G.accent.y, G.accent.z, 1.0f), 12.0f, ImDrawFlags_RoundCornersTop);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5 * ui_scale);
        ImGui::TextColored(ImVec4(1,1,1,1), "%s", TR("CREATION CRYSTAL / IMAGINATOR WARNING", "AVISO: CRISTAL DE CRIAÇÃO / IMAGINATOR"));
        ImGui::Separator();
        ImGui::Spacing(); ImGui::Spacing();

        ImGui::TextWrapped("%s", TR("Creation Crystals and Imaginators permanently lock their Battle Class upon creation in Skylanders Imaginators.",
                                    "Cristais de Criação e Imaginators travam permanentemente sua Classe de Batalha ao serem criados no Skylanders Imaginators."));
        ImGui::Spacing();
        ImGui::TextWrapped("%s", TR("Cloning with a New Identity generates a distinct UID/Serial, but the figure's locked class and character data will carry over unless reset using specialized tools.",
                                    "Clonar com uma Nova Identidade gera um UID/Serial diferente, mas a classe travada e os dados do personagem serão transferidos, a menos que sejam redefinidos por ferramentas especializadas."));
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.85f, 0.2f, 1), "%s: %s", TR("Target", "Alvo"), GetFilename(G.clone_source_path).c_str());
        ImGui::Spacing(); ImGui::Spacing();

        float b_w = 200 * ui_scale;
        if (ImGui::Button(TR("Proceed with Clone", "Prosseguir com Clone"), ImVec2(b_w, 50 * ui_scale))) {
            std::string src_name = GetFilename(G.clone_source_path);
            std::string stem = (src_name.size() >= 5) ? src_name.substr(0, src_name.size() - 5) : src_name;
            std::string default_clone = stem + "_new_id";
            std::string new_name = ShowKeyboard(default_clone, TR("New Identity clone name (.dump)", "Nome do clone com nova identidade (.dump)"));
            if (!new_name.empty()) {
                if (new_name.size() < 5 || strcasecmp(new_name.c_str() + new_name.size() - 5, ".dump") != 0) {
                    new_name += ".dump";
                }
                std::string clean = SanitizeFileName(new_name);
                std::string dest = G.current_path + clean;
                if (Runes::CloneDumpNewIdentity(G.clone_source_path, dest)) {
                    RefreshFileList(G.current_path, G.sd_files, G.current_path);
                    selected_sd_idx = 0;
                    request_nav_focus = true;
                }
            }
            ImGui::CloseCurrentPopup();
            request_nav_focus = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(TR("Cancel", "Cancelar"), ImVec2(b_w, 50 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown)) {
            ImGui::CloseCurrentPopup();
            request_nav_focus = true;
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
}

// ============================================================
// Screen: Figure Image Editor
// ============================================================

void RenderScreenImageEditor(float screen_w, float content_h, float ui_scale, bool& just_entered_content, ImFont* f_large, ImFont* f_small) {
    (void)f_small;
    (void)screen_w;

    // Static State for Step 1 (.dump selection)
    static std::string ed_dump_browser_path = FIGURES_DIR;
    static std::vector<FileInfo> ed_dump_files;
    static int ed_dump_selected_idx = 0;
    static bool ed_dump_request_focus = true;

    // Static State for Step 2 (Photo browser starting in detected Switch album or latest captures)
    static std::string ed_photo_browser_path = GetDefaultPhotoBrowserDir();
    static std::vector<FileInfo> ed_photo_files;
    static int ed_photo_selected_idx = 0;
    static bool ed_photo_request_focus = true;

    // Static State for Step 3 (Crop Tool)
    static int s_photo_orig_w = 0;
    static int s_photo_orig_h = 0;
    static bool s_crop_initialized = false;

    if (just_entered_content) {
        if (G.editor_dump_path.empty() || G.editor_state == EditorState::SELECT_DUMP) {
            G.editor_state = EditorState::SELECT_DUMP;
            ed_dump_files.clear();
            ed_dump_selected_idx = 0;
            ed_dump_request_focus = true;
        } else if (G.editor_state == EditorState::SELECT_PHOTO) {
            ed_photo_browser_path = GetDefaultPhotoBrowserDir();
            for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
            ed_photo_files.clear();
            ed_photo_request_focus = true;
            ed_photo_selected_idx = 0;
        }
    }

    // --- STEP 1: SELECT FIGURE DUMP ---
    if (G.editor_state == EditorState::SELECT_DUMP) {
        if (ed_dump_files.empty() || just_entered_content) {
            for (auto& item : ed_dump_files) if (item.tex) { glDeleteTextures(1, &item.tex); item.tex = 0; }
            ed_dump_files.clear();

            if (ed_dump_browser_path == "<favorites>") {
                LoadOverlayFavoritesList(ed_dump_files);
            } else {
                std::error_code ec;
                if (!fs::exists(ed_dump_browser_path, ec)) {
                    ed_dump_browser_path = FIGURES_DIR;
                    fs::create_directories(ed_dump_browser_path, ec);
                }

                // "Go back" entry if inside a subfolder
                if (ed_dump_browser_path != FIGURES_DIR && ed_dump_browser_path != "sdmc:/") {
                    FileInfo back_item;
                    back_item.name = "<- Go back 1 directory";
                    back_item.path = GetParentPath(ed_dump_browser_path);
                    back_item.is_dir = true;
                    back_item.size = 0;
                    ed_dump_files.push_back(back_item);
                }

                std::vector<FileInfo> dir_items;
                std::vector<FileInfo> dump_items;
                for (auto const& entry : fs::directory_iterator(ed_dump_browser_path, ec)) {
                    FileInfo fi;
                    fi.name = entry.path().filename().string();
                    fi.path = entry.path().string();
                    fi.is_dir = entry.is_directory(ec);
                    fi.size = fi.is_dir ? 0 : fs::file_size(entry.path(), ec);
                    fi.tex = 0;
                    fi.tex_checked = false;

                    if (fi.is_dir) {
                        fi.name = "[DIR] " + fi.name;
                        dir_items.push_back(fi);
                    } else {
                        std::string ext = entry.path().extension().string();
                        for (auto& c : ext) c = tolower(c);
                        if (ext == ".dump" || ext == ".bin" || fi.size == 1024) {
                            dump_items.push_back(fi);
                        }
                    }
                }
                std::sort(dir_items.begin(), dir_items.end(), [](const FileInfo& a, const FileInfo& b){ return a.name < b.name; });
                std::sort(dump_items.begin(), dump_items.end(), [](const FileInfo& a, const FileInfo& b){ return a.name < b.name; });
                ed_dump_files.insert(ed_dump_files.end(), dir_items.begin(), dir_items.end());
                ed_dump_files.insert(ed_dump_files.end(), dump_items.begin(), dump_items.end());
            }

            ed_dump_selected_idx = 0;
            ed_dump_request_focus = true;
        }

        if (f_large) ImGui::PushFont(f_large);
        ImGui::TextColored(G.accent, "%s", TR("Step 1: Select Figure Dump", "Passo 1: Selecionar Boneco"));
        if (f_large) ImGui::PopFont();
        ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "%s",
            TR("Choose a figure (.dump) to assign or update custom artwork.", "Escolha um boneco (.dump) para adicionar ou alterar a foto."));

        if (ed_dump_browser_path == "<favorites>") {
            ImGui::TextColored(G.accent, "%s", TR("Source: ★ Tesla Overlay Favorites (sdmc:/emulanders/overlay/favorites.txt)", "Origem: ★ Favoritos do Overlay (sdmc:/emulanders/overlay/favorites.txt)"));
        } else {
            ImGui::TextColored(G.accent, TR("Folder: %s", "Pasta: %s"), ed_dump_browser_path.c_str());
        }
        ImGui::Spacing();

        // Responsive Step 1 Toolbar with Gamepad Shortcuts
        float btn_h = 44.0f * ui_scale;

        bool is_figures = (ed_dump_browser_path == FIGURES_DIR);
        if (is_figures) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button(TR("[L] Figures", "[L] Bonecos"), ImVec2(165.0f * ui_scale, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
            ed_dump_browser_path = FIGURES_DIR;
            for (auto& item : ed_dump_files) if (item.tex) { glDeleteTextures(1, &item.tex); item.tex = 0; }
            ed_dump_files.clear();
            ed_dump_selected_idx = 0;
            ed_dump_request_focus = true;
        }
        if (is_figures) ImGui::PopStyleColor();

        ImGui::SameLine(0, 10.0f * ui_scale);

        bool is_scans = (ed_dump_browser_path == SCANS_DIR);
        if (is_scans) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button(TR("[R] Scans", "[R] Scans"), ImVec2(140.0f * ui_scale, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1)) {
            ed_dump_browser_path = SCANS_DIR;
            for (auto& item : ed_dump_files) if (item.tex) { glDeleteTextures(1, &item.tex); item.tex = 0; }
            ed_dump_files.clear();
            ed_dump_selected_idx = 0;
            ed_dump_request_focus = true;
        }
        if (is_scans) ImGui::PopStyleColor();

        ImGui::SameLine(0, 10.0f * ui_scale);

        bool is_favs = (ed_dump_browser_path == "<favorites>");
        if (is_favs) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ed_dump_files.empty() && is_favs) ImGui::SetItemDefaultFocus();
        if (ImGui::Button(TR("[Y] ★ Favorites", "[Y] ★ Favoritos"), ImVec2(210.0f * ui_scale, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft)) {
            ed_dump_browser_path = "<favorites>";
            for (auto& item : ed_dump_files) if (item.tex) { glDeleteTextures(1, &item.tex); item.tex = 0; }
            ed_dump_files.clear();
            LoadOverlayFavoritesList(ed_dump_files);
            ed_dump_selected_idx = 0;
            ed_dump_request_focus = true;
        }
        if (is_favs) ImGui::PopStyleColor();

        // Right-aligned Back to Home Menu button
        float back_btn_w = 140.0f * ui_scale;
        float avail_w = ImGui::GetContentRegionAvail().x;
        if (avail_w > back_btn_w + 10.0f * ui_scale) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail_w - back_btn_w);
        } else {
            ImGui::SameLine(0, 10.0f * ui_scale);
        }
        if (ImGui::Button(TR("(B) Menu", "(B) Menu"), ImVec2(back_btn_w, btn_h))) {
            G.in_menu = true;
            G.menu_needs_nav_focus = true;
            just_entered_content = true;
        }
        ImGui::Spacing();

        if (ed_dump_browser_path == "<favorites>" && ed_dump_files.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), TR("No favorites found in %s", "Nenhum favorito encontrado em %s"), OVERLAY_FAVORITES_FILE);
            ImGui::Spacing();
            ImGui::TextWrapped("%s", TR("Open the Tesla Overlay (L + D-Pad Down) while playing or in-game, and star figures to add them to your Favorites!",
                                        "Abra o Overlay do Tesla (L + D-Pad Baixo) no jogo e favorite bonecos com a estrela para adicioná-los aqui!"));
            ImGui::Spacing();
        }

        if (ed_dump_request_focus) ImGui::SetNextWindowFocus();
        float tbl_h = content_h - 195.0f * ui_scale;
        if (ImGui::BeginTable("EditorDumps", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH, ImVec2(0, tbl_h))) {
            ImGui::TableSetupColumn(TR("Figure / File", "Boneco / Arquivo"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(TR("Type", "Tipo"), ImGuiTableColumnFlags_WidthFixed, 110 * ui_scale);
            ImGui::TableSetupColumn(TR("Size", "Tamanho"), ImGuiTableColumnFlags_WidthFixed, 100 * ui_scale);
            ImGui::TableHeadersRow();

            int dump_tex_loads = 0;
            for (size_t i = 0; i < ed_dump_files.size(); i++) {
                auto& item = ed_dump_files[i];

                // Load existing artwork thumbnail if available
                if (!item.is_dir && !item.tex_checked && dump_tex_loads < 3) {
                    item.tex_checked = true;
                    dump_tex_loads++;
                    size_t dot = item.path.find_last_of('.');
                    std::string adj_png = (dot != std::string::npos) ? item.path.substr(0, dot) + ".png" : "";
                    std::error_code ec;
                    if (!adj_png.empty() && fs::exists(adj_png, ec)) {
                        item.tex = LoadTextureFromFile(adj_png.c_str());
                    } else if (item.size >= 0x20) {
                        std::ifstream in(item.path, std::ios::binary);
                        if (in.is_open()) {
                            uint8_t hdr[0x20] = {0};
                            in.read(reinterpret_cast<char*>(hdr), 0x20);
                            if (in.gcount() >= 0x20) {
                                u32 cid = hdr[16] | (hdr[17] << 8) | (hdr[18] << 16);
                                u16 vid = hdr[28] | (hdr[29] << 8);
                                char apath[256];
                                snprintf(apath, sizeof(apath), "sdmc:/emulanders/assets/%06X_%04X.png", cid, vid);
                                if (fs::exists(apath, ec)) {
                                    item.tex = LoadTextureFromFile(apath);
                                } else {
                                    snprintf(apath, sizeof(apath), "sdmc:/emulanders/assets/%06X.png", cid);
                                    if (fs::exists(apath, ec)) {
                                        item.tex = LoadTextureFromFile(apath);
                                    }
                                }
                            }
                        }
                    }
                }

                ImGui::PushID((int)i);
                float row_h = item.is_dir ? (40.0f * ui_scale) : (52.0f * ui_scale);
                ImGui::TableNextRow(0, row_h);
                ImGui::TableSetColumnIndex(0);

                if (ed_dump_request_focus && (int)i == ed_dump_selected_idx) {
                    ImGui::SetKeyboardFocusHere();
                    ImGui::SetItemDefaultFocus();
                    ImGuiContext& g = *GImGui;
                    g.NavCursorVisible = true;
                    ed_dump_request_focus = false;
                }

                ImVec2 row_p = ImGui::GetCursorScreenPos();
                float text_offset_x = (item.tex != 0) ? (38.0f * ui_scale) : 0.0f;
                if (text_offset_x > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_offset_x);

                bool clicked = ImGui::Selectable(item.name.c_str(), ed_dump_selected_idx == (int)i,
                                                 ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                if (ImGui::IsItemFocused()) {
                    ed_dump_selected_idx = (int)i;
                }

                if (item.tex) {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    float th_w = 26.0f * ui_scale;
                    float th_h = 40.0f * ui_scale;
                    float off_y = (row_h - th_h) * 0.5f;
                    dl->AddImage((void*)(intptr_t)item.tex,
                                 ImVec2(row_p.x + 4.0f * ui_scale, row_p.y + off_y),
                                 ImVec2(row_p.x + 4.0f * ui_scale + th_w, row_p.y + off_y + th_h));
                }

                if (clicked) {
                    ed_dump_selected_idx = (int)i;
                    if (item.name == "<- Go back 1 directory" || item.is_dir) {
                        ed_dump_browser_path = item.path;
                        if (ed_dump_browser_path.back() != '/') ed_dump_browser_path += "/";
                        for (auto& d : ed_dump_files) if (d.tex) { glDeleteTextures(1, &d.tex); d.tex = 0; }
                        ed_dump_files.clear();
                        ed_dump_request_focus = true;
                        ed_dump_selected_idx = 0;
                    } else {
                        G.editor_dump_path = item.path;
                        G.editor_state = EditorState::SELECT_PHOTO;
                        // Start photo browser directly in detected Switch album / latest captures!
                        ed_photo_browser_path = GetDefaultPhotoBrowserDir();
                        for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
                        ed_photo_files.clear();
                        ed_photo_request_focus = true;
                        ed_photo_selected_idx = 0;
                    }
                }

                ImGui::TableSetColumnIndex(1);
                if (item.is_dir) ImGui::Text("DIR");
                else if (ed_dump_browser_path == "<favorites>") ImGui::Text("%s", TR("★ FAVORITE", "★ FAVORITO"));
                else ImGui::Text("DUMP");

                ImGui::TableSetColumnIndex(2);
                if (!item.is_dir) ImGui::Text("%lu B", (unsigned long)item.size);

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        // Back button (B) navigation in Step 1
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            if (ed_dump_browser_path == "<favorites>") {
                ed_dump_browser_path = FIGURES_DIR;
                for (auto& d : ed_dump_files) if (d.tex) { glDeleteTextures(1, &d.tex); d.tex = 0; }
                ed_dump_files.clear();
                ed_dump_request_focus = true;
                ed_dump_selected_idx = 0;
            } else if (ed_dump_browser_path != FIGURES_DIR && ed_dump_browser_path != "sdmc:/") {
                ed_dump_browser_path = GetParentPath(ed_dump_browser_path);
                for (auto& d : ed_dump_files) if (d.tex) { glDeleteTextures(1, &d.tex); d.tex = 0; }
                ed_dump_files.clear();
                ed_dump_request_focus = true;
                ed_dump_selected_idx = 0;
            } else {
                G.in_menu = true;
                G.menu_needs_nav_focus = true;
                just_entered_content = true;
            }
        }

    // --- STEP 2: SELECT PHOTO / SCREENSHOT ---
    } else if (G.editor_state == EditorState::SELECT_PHOTO) {
        if (ed_photo_files.empty() || ed_photo_request_focus) {
            for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
            ed_photo_files.clear();
            std::error_code ec;

            if (!fs::exists(ed_photo_browser_path, ec)) {
                std::string detected = DetectSwitchAlbumDir();
                if (fs::exists(detected, ec)) ed_photo_browser_path = detected;
                else ed_photo_browser_path = "sdmc:/";
            }

            // "Go back" entry if inside a subfolder
            if (ed_photo_browser_path != "sdmc:/") {
                FileInfo back_item;
                back_item.name = "<- Go back 1 directory";
                back_item.path = GetParentPath(ed_photo_browser_path);
                back_item.is_dir = true;
                back_item.size = 0;
                ed_photo_files.push_back(back_item);
            }

            std::vector<FileInfo> dir_items;
            std::vector<FileInfo> photo_items;

            for (auto const& entry : fs::directory_iterator(ed_photo_browser_path, ec)) {
                FileInfo fi;
                fi.name = entry.path().filename().string();
                fi.path = entry.path().string();
                fi.is_dir = entry.is_directory(ec);
                fi.size = fi.is_dir ? 0 : fs::file_size(entry.path(), ec);
                fi.tex = 0;
                fi.tex_checked = false;

                if (fi.is_dir) {
                    fi.name = "[DIR] " + fi.name;
                    dir_items.push_back(fi);
                } else {
                    std::string ext = entry.path().extension().string();
                    for (auto& c : ext) c = tolower(c);
                    // Strictly filter valid image extensions!
                    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                        photo_items.push_back(fi);
                    }
                }
            }

            std::sort(dir_items.begin(), dir_items.end(), [](const FileInfo& a, const FileInfo& b){ return a.name < b.name; });
            // Newest screenshots first
            std::sort(photo_items.begin(), photo_items.end(), [](const FileInfo& a, const FileInfo& b){ return a.name > b.name; });

            ed_photo_files.insert(ed_photo_files.end(), dir_items.begin(), dir_items.end());
            ed_photo_files.insert(ed_photo_files.end(), photo_items.begin(), photo_items.end());

            ed_photo_selected_idx = 0;
            ed_photo_request_focus = true;
        }

        if (f_large) ImGui::PushFont(f_large);
        ImGui::TextColored(G.accent, "%s", TR("Step 2: Choose Source Photo", "Passo 2: Escolher Foto de Origem"));
        if (f_large) ImGui::PopFont();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            TR("Assigning artwork for: %s", "Definindo arte para: %s"), GetFilename(G.editor_dump_path).c_str());

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), TR("Folder: %s", "Pasta: %s"), ed_photo_browser_path.c_str());
        ImGui::Spacing();

        // Responsive Step 2 Toolbar with 44px height and Gamepad Shortcuts
        float btn_h = 44.0f * ui_scale;

        std::string def_album = GetDefaultPhotoBrowserDir();
        bool is_def_album = (ed_photo_browser_path == def_album);
        if (is_def_album) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button(TR("[Y] Latest Photos", "[Y] Recentes"), ImVec2(170.0f * ui_scale, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft)) {
            ed_photo_browser_path = def_album;
            for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
            ed_photo_files.clear();
            ed_photo_request_focus = true;
            ed_photo_selected_idx = 0;
        }
        if (is_def_album) ImGui::PopStyleColor();

        ImGui::SameLine(0, 10.0f * ui_scale);

        std::string album_root = DetectSwitchAlbumDir();
        bool is_album_root = (ed_photo_browser_path == album_root);
        if (is_album_root) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button(TR("[X] Album Root", "[X] Álbum"), ImVec2(140.0f * ui_scale, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp)) {
            ed_photo_browser_path = album_root;
            for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
            ed_photo_files.clear();
            ed_photo_request_focus = true;
            ed_photo_selected_idx = 0;
        }
        if (is_album_root) ImGui::PopStyleColor();

        ImGui::SameLine(0, 10.0f * ui_scale);

        bool is_sd_root = (ed_photo_browser_path == "sdmc:/");
        if (is_sd_root) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button(TR("[L] SD Root (/)", "[L] Raiz SD (/)"), ImVec2(140.0f * ui_scale, btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
            ed_photo_browser_path = "sdmc:/";
            for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
            ed_photo_files.clear();
            ed_photo_request_focus = true;
            ed_photo_selected_idx = 0;
        }
        if (is_sd_root) ImGui::PopStyleColor();

        // Right-aligned Back to Figures button
        float back_w = 140.0f * ui_scale;
        float avail_w = ImGui::GetContentRegionAvail().x;
        if (avail_w > back_w + 10.0f * ui_scale) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail_w - back_w);
        } else {
            ImGui::SameLine(0, 10.0f * ui_scale);
        }
        if (ImGui::Button(TR("(B) Back", "(B) Voltar"), ImVec2(back_w, btn_h))) {
            for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
            ed_photo_files.clear();
            G.editor_state = EditorState::SELECT_DUMP;
            ed_dump_request_focus = true;
            ed_dump_selected_idx = 0;
        }
        ImGui::Spacing();

        if (ed_photo_files.empty() || (ed_photo_files.size() == 1 && ed_photo_files[0].name == "<- Go back 1 directory")) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), TR("No photos found in this folder (%s)", "Nenhuma foto encontrada nesta pasta (%s)"), ed_photo_browser_path.c_str());
            ImGui::Spacing();
            ImGui::TextWrapped("%s", TR("Switch screenshots are located in Album subfolders: YYYY/MM/DD/*.jpg.",
                                        "As capturas de tela do Switch ficam em subpastas do Álbum: AAAA/MM/DD/*.jpg."));
            ImGui::TextWrapped("%s", TR("Navigate into the year/month/day folders above to select your screenshot, or use photos (.jpg / .png) placed on your SD card.",
                                        "Navegue pelas pastas de ano/mês/dia acima para escolher sua captura, ou use fotos (.jpg / .png) colocadas no cartão SD."));
            ImGui::Spacing();
        }

        if (ed_photo_request_focus) ImGui::SetNextWindowFocus();
        float tbl_h = content_h - 195.0f * ui_scale;
        if (ImGui::BeginTable("EditorPhotos", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH, ImVec2(0, tbl_h))) {
            ImGui::TableSetupColumn(TR("Preview / Name", "Prévia / Nome"), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(TR("Type", "Tipo"), ImGuiTableColumnFlags_WidthFixed, 80 * ui_scale);
            ImGui::TableSetupColumn(TR("Size", "Tamanho"), ImGuiTableColumnFlags_WidthFixed, 100 * ui_scale);
            ImGui::TableHeadersRow();

            int thumb_loads_this_frame = 0;

            for (size_t i = 0; i < ed_photo_files.size(); i++) {
                auto& p = ed_photo_files[i];

                if (!p.is_dir && !p.tex_checked && thumb_loads_this_frame < 3) {
                    p.tex_checked = true;
                    thumb_loads_this_frame++;
                    p.tex = LoadTextureFromFile(p.path.c_str());
                }

                ImGui::PushID((int)i);
                float row_h = p.is_dir ? (40.0f * ui_scale) : (74.0f * ui_scale);
                ImGui::TableNextRow(0, row_h);
                ImGui::TableSetColumnIndex(0);

                if (ed_photo_request_focus && (int)i == ed_photo_selected_idx) {
                    ImGui::SetKeyboardFocusHere();
                    ed_photo_request_focus = false;
                }

                ImVec2 row_p = ImGui::GetCursorScreenPos();
                float text_offset_x = (p.tex != 0) ? (122.0f * ui_scale) : 0.0f;
                if (text_offset_x > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_offset_x);

                bool clicked = ImGui::Selectable(p.name.c_str(), ed_photo_selected_idx == (int)i,
                                                 ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                if (ImGui::IsItemFocused()) {
                    ed_photo_selected_idx = (int)i;
                }

                if (p.tex) {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    float th_h = 62.0f * ui_scale;
                    float th_w = th_h * 1.777f; // 16:9 thumbnail
                    float off_y = (row_h - th_h) * 0.5f;
                    dl->AddImage((void*)(intptr_t)p.tex,
                                 ImVec2(row_p.x + 4.0f * ui_scale, row_p.y + off_y),
                                 ImVec2(row_p.x + 4.0f * ui_scale + th_w, row_p.y + off_y + th_h));
                }

                if (clicked) {
                    ed_photo_selected_idx = (int)i;
                    if (p.name == "<- Go back 1 directory" || p.is_dir) {
                        ed_photo_browser_path = p.path;
                        if (ed_photo_browser_path.back() != '/') ed_photo_browser_path += "/";
                        for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
                        ed_photo_files.clear();
                        ed_photo_request_focus = true;
                        ed_photo_selected_idx = 0;
                    } else {
                        G.editor_photo_path = p.path;
                        if (G.editor_photo_tex) {
                            glDeleteTextures(1, &G.editor_photo_tex);
                            G.editor_photo_tex = 0;
                        }
                        s_photo_orig_w = 0;
                        s_photo_orig_h = 0;
                        G.editor_photo_tex = LoadTextureFromFile(p.path.c_str(), &s_photo_orig_w, &s_photo_orig_h);
                        if (s_photo_orig_w <= 0) s_photo_orig_w = 1280;
                        if (s_photo_orig_h <= 0) s_photo_orig_h = 720;
                        s_crop_initialized = false;
                        G.editor_state = EditorState::CROP_EDIT;
                    }
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", p.is_dir ? "DIR" : "PHOTO");

                ImGui::TableSetColumnIndex(2);
                if (!p.is_dir) {
                    if (p.size > 1024 * 1024) ImGui::Text("%.1f MB", p.size / (1024.0 * 1024.0));
                    else ImGui::Text("%.1f KB", p.size / 1024.0);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        // Back button (B) navigation in Step 2
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            std::string album_root = DetectSwitchAlbumDir();
            if (ed_photo_browser_path != album_root && ed_photo_browser_path != "sdmc:/" && ed_photo_browser_path != "sdmc:/Nintendo/Album/") {
                ed_photo_browser_path = GetParentPath(ed_photo_browser_path);
                for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
                ed_photo_files.clear();
                ed_photo_request_focus = true;
                ed_photo_selected_idx = 0;
            } else {
                for (auto& pf : ed_photo_files) if (pf.tex) { glDeleteTextures(1, &pf.tex); pf.tex = 0; }
                ed_photo_files.clear();
                G.editor_state = EditorState::SELECT_DUMP;
                ed_dump_request_focus = true;
                ed_dump_selected_idx = 0;
            }
        }

    // --- STEP 3: INTERACTIVE CROP TOOL (175 x 275) ---
    } else if (G.editor_state == EditorState::CROP_EDIT) {
        if (f_large) ImGui::PushFont(f_large);
        ImGui::TextColored(G.accent, "%s", TR("Step 3: Crop & Save Artwork", "Passo 3: Recortar e Salvar Arte"));
        if (f_large) ImGui::PopFont();

        size_t dot = G.editor_dump_path.find_last_of('.');
        std::string target_png = (dot != std::string::npos ? G.editor_dump_path.substr(0, dot) : G.editor_dump_path) + ".png";
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f),
            TR("Target: %s  ->  %s", "Alvo: %s  ->  %s"), GetFilename(G.editor_dump_path).c_str(), GetFilename(target_png).c_str());
        ImGui::Spacing();

        if (G.editor_photo_tex) {
            float avail_w = ImGui::GetContentRegionAvail().x;
            float avail_h = content_h - 130.0f * ui_scale;

            float right_panel_w = 250.0f * ui_scale;
            float max_disp_w = avail_w - right_panel_w - 25.0f * ui_scale;

            // Dimensions for photo display (maintaining photo aspect)
            float aspect = (float)s_photo_orig_w / (float)s_photo_orig_h;
            if (aspect <= 0.1f) aspect = 1.777f;

            float disp_h = avail_h;
            float disp_w = disp_h * aspect;
            if (disp_w > max_disp_w) {
                disp_w = max_disp_w;
                disp_h = disp_w / aspect;
            }

            // Aspect ratio constant: 175 / 275 (0.6363636f)
            const float target_ratio = (float)LOGO_WIDTH / (float)LOGO_HEIGHT;

            // Initialize crop box on first frame
            if (!s_crop_initialized) {
                float init_h = disp_h * 0.85f;
                float init_w = init_h * target_ratio;
                G.crop_size = ImVec2(init_w, init_h);
                G.crop_pos = ImVec2((disp_w - init_w) * 0.5f, (disp_h - init_h) * 0.5f);
                s_crop_initialized = true;
            }

            // Controls input handling:
            // 1. Movement: D-Pad and Left Analog Stick
            float move_spd = 3.5f * ui_scale;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadLeft))  G.crop_pos.x -= move_spd;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight)) G.crop_pos.x += move_spd;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadUp))    G.crop_pos.y -= move_spd;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown))  G.crop_pos.y += move_spd;

            if (G.controller) {
                float lx = SDL_GameControllerGetAxis(G.controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
                float ly = SDL_GameControllerGetAxis(G.controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;
                if (std::abs(lx) > 0.18f) G.crop_pos.x += lx * move_spd;
                if (std::abs(ly) > 0.18f) G.crop_pos.y += ly * move_spd;
            }

            // 2. Zoom / Scale: Right Stick or Shoulder Buttons (L / R / ZL / ZR)
            float zoom_delta = 0.0f;
            if (G.controller) {
                float ry = SDL_GameControllerGetAxis(G.controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;
                if (std::abs(ry) > 0.18f) zoom_delta -= ry * 4.0f * ui_scale;
            }
            if (ImGui::IsKeyDown(ImGuiKey_GamepadR1) || ImGui::IsKeyDown(ImGuiKey_GamepadR2)) zoom_delta += 3.0f * ui_scale;
            if (ImGui::IsKeyDown(ImGuiKey_GamepadL1) || ImGui::IsKeyDown(ImGuiKey_GamepadL2)) zoom_delta -= 3.0f * ui_scale;

            if (zoom_delta != 0.0f) {
                float new_h = G.crop_size.y + zoom_delta;
                float new_w = new_h * target_ratio;
                float min_h = 70.0f * ui_scale;
                float max_h = disp_h;
                float max_w = disp_w;
                if (new_h >= min_h && new_h <= max_h && new_w <= max_w) {
                    float diff_x = new_w - G.crop_size.x;
                    float diff_y = new_h - G.crop_size.y;
                    G.crop_pos.x -= diff_x * 0.5f;
                    G.crop_pos.y -= diff_y * 0.5f;
                    G.crop_size = ImVec2(new_w, new_h);
                }
            }

            // Clamp crop box strictly within display boundaries
            if (G.crop_pos.x < 0.0f) G.crop_pos.x = 0.0f;
            if (G.crop_pos.y < 0.0f) G.crop_pos.y = 0.0f;
            if (G.crop_pos.x + G.crop_size.x > disp_w) G.crop_pos.x = disp_w - G.crop_size.x;
            if (G.crop_pos.y + G.crop_size.y > disp_h) G.crop_pos.y = disp_h - G.crop_size.y;

            // Render photo display (Left side)
            ImGui::BeginGroup();
            ImVec2 img_p0 = ImGui::GetCursorScreenPos();
            ImGui::Image((void*)(intptr_t)G.editor_photo_tex, ImVec2(disp_w, disp_h));
            if (ImGui::IsItemActive() || (ImGui::IsItemHovered() && ImGui::IsMouseDragging(0))) {
                ImVec2 drag_delta = ImGui::GetMouseDragDelta(0);
                G.crop_pos.x += drag_delta.x;
                G.crop_pos.y += drag_delta.y;
                ImGui::ResetMouseDragDelta(0);
            }

            // Overlay rendering: Darken exterior, highlight frame, draw rule-of-thirds grid
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float cx = img_p0.x + G.crop_pos.x;
            float cy = img_p0.y + G.crop_pos.y;
            float cw = G.crop_size.x;
            float ch = G.crop_size.y;

            // 4 Darkened quadrants outside crop rectangle
            ImU32 dark_col = IM_COL32(0, 0, 0, 160);
            dl->AddRectFilled(img_p0, ImVec2(img_p0.x + disp_w, cy), dark_col); // Top
            dl->AddRectFilled(ImVec2(img_p0.x, cy + ch), ImVec2(img_p0.x + disp_w, img_p0.y + disp_h), dark_col); // Bottom
            dl->AddRectFilled(ImVec2(img_p0.x, cy), ImVec2(cx, cy + ch), dark_col); // Left
            dl->AddRectFilled(ImVec2(cx + cw, cy), ImVec2(img_p0.x + disp_w, cy + ch), dark_col); // Right

            // Rule-of-thirds grid lines
            ImU32 grid_col = IM_COL32(255, 255, 255, 50);
            dl->AddLine(ImVec2(cx + cw * 0.333f, cy), ImVec2(cx + cw * 0.333f, cy + ch), grid_col, 1.0f);
            dl->AddLine(ImVec2(cx + cw * 0.666f, cy), ImVec2(cx + cw * 0.666f, cy + ch), grid_col, 1.0f);
            dl->AddLine(ImVec2(cx, cy + ch * 0.333f), ImVec2(cx + cw, cy + ch * 0.333f), grid_col, 1.0f);
            dl->AddLine(ImVec2(cx, cy + ch * 0.666f), ImVec2(cx + cw, cy + ch * 0.666f), grid_col, 1.0f);

            // Vibrant Accent Crop Border
            dl->AddRect(ImVec2(cx, cy), ImVec2(cx + cw, cy + ch), ImColor(G.accent), 0, 0, 2.5f);

            // Corner Handles
            float corner_len = 16.0f * ui_scale;
            ImU32 handle_col = IM_COL32(255, 255, 255, 255);
            // Top-Left
            dl->AddLine(ImVec2(cx, cy), ImVec2(cx + corner_len, cy), handle_col, 3.0f);
            dl->AddLine(ImVec2(cx, cy), ImVec2(cx, cy + corner_len), handle_col, 3.0f);
            // Top-Right
            dl->AddLine(ImVec2(cx + cw, cy), ImVec2(cx + cw - corner_len, cy), handle_col, 3.0f);
            dl->AddLine(ImVec2(cx + cw, cy), ImVec2(cx + cw, cy + corner_len), handle_col, 3.0f);
            // Bottom-Left
            dl->AddLine(ImVec2(cx, cy + ch), ImVec2(cx + corner_len, cy + ch), handle_col, 3.0f);
            dl->AddLine(ImVec2(cx, cy + ch), ImVec2(cx, cy + corner_len), handle_col, 3.0f);
            // Bottom-Right
            dl->AddLine(ImVec2(cx + cw, cy + ch), ImVec2(cx + cw - corner_len, cy + ch), handle_col, 3.0f);
            dl->AddLine(ImVec2(cx + cw, cy + ch), ImVec2(cx + cw, cy + corner_len), handle_col, 3.0f);

            ImGui::EndGroup();

            // Right side: Real-time Live Preview & Action Buttons
            ImGui::SameLine(0, 20.0f * ui_scale);
            ImGui::BeginGroup();

            ImGui::TextColored(G.accent, "%s", TR("Live Preview (175 x 275)", "Prévia (175 x 275)"));
            ImVec2 uv0(G.crop_pos.x / disp_w, G.crop_pos.y / disp_h);
            ImVec2 uv1((G.crop_pos.x + G.crop_size.x) / disp_w, (G.crop_pos.y + G.crop_size.y) / disp_h);

            float prev_h = 160.0f * ui_scale;
            float prev_w = prev_h * target_ratio;
            float center_off_x = (right_panel_w - prev_w) * 0.5f;
            if (center_off_x > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + center_off_x);

            ImVec2 prev_p0 = ImGui::GetCursorScreenPos();
            ImGui::Image((void*)(intptr_t)G.editor_photo_tex, ImVec2(prev_w, prev_h), uv0, uv1);
            dl->AddRect(prev_p0, ImVec2(prev_p0.x + prev_w, prev_p0.y + prev_h), ImColor(G.accent), 4.0f, 0, 2.0f);

            ImGui::Spacing();

            auto SaveCroppedArt = [&]() {
                SDL_Surface* s = IMG_Load(G.editor_photo_path.c_str());
                if (s) {
                    float scX = (float)s->w / disp_w;
                    float scY = (float)s->h / disp_h;
                    SDL_Rect srcc = {
                        (int)(G.crop_pos.x * scX),
                        (int)(G.crop_pos.y * scY),
                        (int)(G.crop_size.x * scX),
                        (int)(G.crop_size.y * scY)
                    };
                    if (srcc.x < 0) srcc.x = 0;
                    if (srcc.y < 0) srcc.y = 0;
                    if (srcc.x + srcc.w > s->w) srcc.w = s->w - srcc.x;
                    if (srcc.y + srcc.h > s->h) srcc.h = s->h - srcc.y;

                    SDL_Surface* crop = SDL_CreateRGBSurfaceWithFormat(0, LOGO_WIDTH, LOGO_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
                    if (crop) {
                        SDL_BlitScaled(s, &srcc, crop, NULL);
                        size_t dot = G.editor_dump_path.find_last_of('.');
                        std::string out = (dot != std::string::npos ? G.editor_dump_path.substr(0, dot) : G.editor_dump_path) + ".png";
                        IMG_SavePNG(crop, out.c_str());
                        chmod(out.c_str(), 0777);
                        SDL_FreeSurface(crop);
                    }
                    SDL_FreeSurface(s);
                }
                if (G.editor_photo_tex) {
                    glDeleteTextures(1, &G.editor_photo_tex);
                    G.editor_photo_tex = 0;
                }
                for (auto& f : ed_photo_files) if (f.tex) { glDeleteTextures(1, &f.tex); f.tex = 0; }
                ed_photo_files.clear();
                for (auto& d : ed_dump_files) if (d.tex) { glDeleteTextures(1, &d.tex); d.tex = 0; }
                ed_dump_files.clear();
                G.editor_state = EditorState::SELECT_DUMP;
                ed_dump_request_focus = true;
                ed_dump_selected_idx = 0;
                for (auto& f : G.sd_files) if (f.tex) { glDeleteTextures(1, &f.tex); f.tex = 0; }
                G.sd_files.clear();
                RefreshFileList(G.current_path, G.sd_files, G.current_path);
                G.file_manager_needs_refresh = true;
                G.file_manager_needs_focus = true;
            };

            float act_btn_w = right_panel_w;
            float act_btn_h = 42.0f * ui_scale;

            ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, G.accent_hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, G.accent_active);
            if (ImGui::Button(TR("Save Artwork (A)", "Salvar Arte (A)"), ImVec2(act_btn_w, act_btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
                SaveCroppedArt();
            }
            ImGui::PopStyleColor(3);

            ImGui::Spacing();

            if (ImGui::Button(TR("Reset Frame (X)", "Redefinir (X)"), ImVec2(act_btn_w, act_btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp)) {
                s_crop_initialized = false;
            }

            ImGui::Spacing();

            if (ImGui::Button(TR("Back to Photos (B)", "Voltar (B)"), ImVec2(act_btn_w, act_btn_h)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown)) {
                if (G.editor_photo_tex) {
                    glDeleteTextures(1, &G.editor_photo_tex);
                    G.editor_photo_tex = 0;
                }
                G.editor_state = EditorState::SELECT_PHOTO;
                ed_photo_request_focus = true;
                ed_photo_selected_idx = 0;
            }

            ImGui::EndGroup();
        }
    }
}

// ============================================================
// Screen: Downloaders (Dump + Asset)
// ============================================================

static size_t CurlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return fwrite(ptr, size, nmemb, (FILE*)userdata);
}

static int CurlDownloadProgressCallback(void*, double dltotal, double dlnow, double, double) {
    G.dl_now.store(dlnow);
    if (dltotal > 0)
        G.progress.store((float)(dlnow / dltotal) * 0.5f);
    else if (dlnow > 0)
        G.progress.store(-1.0f);
    return G.app_exiting.load() ? 1 : 0;
}

void RenderScreenDownloaders(float screen_w, float content_h, float ui_scale, bool& just_entered_content, ImFont* f_large, ImFont* f_small) {
    (void)f_large; (void)f_small;
    static char url_buf[URL_BUFFER_SIZE] = "https://github.com/skylandersNFC/Skylanders-Ultimate-NFC-Pack/tree/main/Dumps";
    static std::vector<std::string> detected_urls;

    if (G.selected_menu == 3) {
        ImGui::TextWrapped("%s", TR("Download .dump files from a remote ZIP URL.", "Baixar arquivos .dump a partir de uma URL remota de arquivo ZIP."));
        ImGui::Spacing(); ImGui::Spacing();

        ImGui::PushItemWidth(screen_w * 0.5f);
        ImGui::InputText(TR("Custom URL", "URL Personalizada"), url_buf, sizeof(url_buf));
        ImGui::PopItemWidth();
        ImGui::SameLine(0, 10.0f * ui_scale);
        if (ImGui::Button(TR("(L) Edit URL", "(L) Editar URL"), ImVec2(210.0f * ui_scale, 42.0f * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
            std::string res = ShowKeyboard(url_buf, TR("Enter ZIP URL", "Digite a URL do ZIP"));
            if (!res.empty()) {
                strncpy(url_buf, res.c_str(), sizeof(url_buf) - 1);
                url_buf[sizeof(url_buf) - 1] = '\0';
            }
        }
        ImGui::Spacing(); ImGui::Spacing();

        if (!G.dl_running) {
            if (ImGui::Button(TR("Download & Extract", "Baixar e Extrair"), ImVec2(300 * ui_scale, 50 * ui_scale))) {
                std::string url = url_buf;
                if (!url.empty()) {
                    G.progress.store(0.0f);
                    G.dl_running = true;
                    G.bg_thread.launch([url]() {
                        std::string target = url;
                        std::string expected_subfolder;
                        if (url.find("github.com") != std::string::npos && url.find("/tree/") != std::string::npos) {
                            size_t p = url.find("/tree/");
                            std::string branch_and_path = url.substr(p + 6);
                            size_t slash = branch_and_path.find("/");
                            if (slash != std::string::npos)
                                expected_subfolder = branch_and_path.substr(slash + 1);
                            target = url.substr(0, p) + "/archive/refs/heads/" + branch_and_path.substr(0, slash) + ".zip";
                        }
                        strncpy(G.status_msg, TR("Downloading...", "Baixando..."), sizeof(G.status_msg) - 1);
                        CURL* curl = curl_easy_init();
                        FILE* fp = fopen("sdmc:/emulanders/tmp_dump.zip", "wb");
                        curl_easy_setopt(curl, CURLOPT_URL, target.c_str());
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlDownloadProgressCallback);
                        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                        curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_USER_AGENT);
                        curl_easy_perform(curl);
                        if (fp) fclose(fp);
                        curl_easy_cleanup(curl);

                        strncpy(G.status_msg, TR("Extracting...", "Extraindo..."), sizeof(G.status_msg) - 1);
                        G.progress.store(0.5f);
                        unzFile uf = unzOpen("sdmc:/emulanders/tmp_dump.zip");
                        if (uf) {
                            unz_global_info gi;
                            unzGetGlobalInfo(uf, &gi);
                            for (uLong i = 0; i < gi.number_entry; i++) {
                                G.progress.store(0.5f + ((float)i / gi.number_entry) * 0.5f);
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));

                                char fzip[512];
                                unz_file_info fi;
                                unzGetCurrentFileInfo(uf, &fi, fzip, sizeof(fzip), NULL, 0, NULL, 0);

                                std::string sanitized;
                                for (int j = 0; fzip[j]; j++) {
                                    char c = fzip[j];
                                    if (c == '\\') c = '/';
                                    if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') c = '_';
                                    if ((unsigned char)c > 126) c = '_';
                                    sanitized += c;
                                }

                                std::string relative_path = sanitized;
                                if (!expected_subfolder.empty()) {
                                    size_t pos = sanitized.find(expected_subfolder);
                                    if (pos == std::string::npos) {
                                        if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                        continue;
                                    }
                                    relative_path = sanitized.substr(pos + expected_subfolder.length());
                                    if (!relative_path.empty() && relative_path[0] == '/') relative_path = relative_path.substr(1);
                                    if (relative_path.empty()) {
                                        if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                        continue;
                                    }
                                }

                                std::string full = std::string(FIGURES_DIR) + DOWNLOADS_SUBDIR + relative_path;
                                if (fzip[strlen(fzip) - 1] == '/') {
                                    std::error_code ec;
                                    fs::create_directories(full, ec);
                                    chmod(full.c_str(), 0777);
                                } else {
                                    std::error_code ec;
                                    fs::create_directories(fs::path(full).parent_path(), ec);
                                    if (fs::exists(full, ec)) {
                                        if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                        continue;
                                    }
                                    if (unzOpenCurrentFile(uf) == UNZ_OK) {
                                        FILE* out = fopen(full.c_str(), "wb");
                                        if (out) {
                                            char buf[8192]; int r;
                                            while ((r = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0)
                                                fwrite(buf, 1, r, out);
                                            fclose(out);
                                            chmod(full.c_str(), 0777);
                                        }
                                        unzCloseCurrentFile(uf);
                                    }
                                }
                                if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                            }
                            unzClose(uf);
                        }
                        std::error_code ec_rm; fs::remove("sdmc:/emulanders/tmp_dump.zip", ec_rm);
                        strncpy(G.status_msg, TR("Download finished! Files saved to sdmc:/emulanders/figures/Downloads/",
                                                 "Download concluído! Arquivos salvos em sdmc:/emulanders/figures/Downloads/"), sizeof(G.status_msg) - 1);
                        G.progress.store(1.0f);
                        G.dl_running = false;
                        G.dl_finished_popup = true;
                    });
                }
            }
        } else {
            float prog = G.progress.load();
            if (prog >= 0.0f) {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, G.accent);
                ImGui::ProgressBar(prog, ImVec2(screen_w * 0.6f, 30 * ui_scale));
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, G.accent);
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(screen_w * 0.6f, 30 * ui_scale), TR("Processing...", "Processando..."));
                ImGui::PopStyleColor();
                ImGui::Text(TR("%.1f MB processed", "%.1f MB processados"), G.dl_now.load() / (1024.0 * 1024.0));
            }
            ImGui::TextColored(G.accent, "%s", G.status_msg);
        }

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "%s", TR("Detected URLs in sdmc:/emulanders/dumps_urls.txt:", "URLs detectadas em sdmc:/emulanders/dumps_urls.txt:"));

        if (detected_urls.empty() || just_entered_content) {
            detected_urls.clear();
            std::ifstream urlf("sdmc:/emulanders/dumps_urls.txt");
            if (urlf.is_open()) {
                std::string line;
                while (std::getline(urlf, line)) {
                    if (line.find("http") != std::string::npos) {
                        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
                        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                        detected_urls.push_back(line);
                    }
                }
                urlf.close();
            }
            if (detected_urls.size() == 1 && url_buf[0] == '\0') {
                strncpy(url_buf, detected_urls[0].c_str(), sizeof(url_buf)-1);
            }
        }

        if (ImGui::BeginChild("URLList", ImVec2(0, 200 * ui_scale), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
            for (size_t i = 0; i < detected_urls.size(); i++) {
                ImGui::PushID((int)i);
                if (ImGui::Button(detected_urls[i].c_str(), ImVec2(ImGui::GetContentRegionAvail().x - 110 * ui_scale, 45 * ui_scale))) {
                    strncpy(url_buf, detected_urls[i].c_str(), sizeof(url_buf)-1);
                }
                ImGui::SameLine();
                if (ImGui::Button(TR("Use", "Usar"), ImVec2(100 * ui_scale, 45 * ui_scale))) {
                    strncpy(url_buf, detected_urls[i].c_str(), sizeof(url_buf)-1);
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }

    } else if (G.selected_menu == 4) {
        ImGui::TextWrapped("%s", TR("Download the community asset pack (figure images for all Skylanders).",
                                    "Baixar o pacote de fotos da comunidade (fotos de todos os Skylanders)."));
        ImGui::TextWrapped("%s", TR("This is recommended for the best experience with the scanner and file manager.",
                                    "Recomendado para a melhor experiência no leitor e no gerenciador de arquivos."));
        ImGui::Spacing();

        if (!G.dl_running) {
            if (G.IsThreadBusy()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::Button(TR("System Busy...", "Sistema Ocupado..."), ImVec2(300 * ui_scale, 50 * ui_scale));
                ImGui::PopStyleColor(3);
            } else if (ImGui::Button(TR("Download Assets", "Baixar Assets"), ImVec2(300 * ui_scale, 50 * ui_scale))) {
                G.progress.store(0.0f);
                G.dl_running = true;
                G.bg_thread.launch([]() {
                    const char* asset_url = "https://github.com/joaohypo/emulanders/releases/download/1.0.0/emulanders_assets_pack.zip";
                    strncpy(G.status_msg, "Downloading assets...", sizeof(G.status_msg) - 1);
                    CURL* curl = curl_easy_init();
                    FILE* fp = fopen("sdmc:/emulanders/tmp_assets.zip", "wb");
                    curl_easy_setopt(curl, CURLOPT_URL, asset_url);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlDownloadProgressCallback);
                    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                    curl_easy_setopt(curl, CURLOPT_USERAGENT, APP_USER_AGENT);
                    curl_easy_perform(curl);
                    fclose(fp);
                    curl_easy_cleanup(curl);

                    strncpy(G.status_msg, "Extracting assets...", sizeof(G.status_msg) - 1);
                    G.progress.store(0.5f);
                    unzFile uf = unzOpen("sdmc:/emulanders/tmp_assets.zip");
                    if (uf) {
                        unz_global_info gi;
                        unzGetGlobalInfo(uf, &gi);
                        for (uLong i = 0; i < gi.number_entry; i++) {
                            G.progress.store(0.5f + ((float)i / gi.number_entry) * 0.5f);
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));

                            char fzip[512]; unz_file_info fi;
                            unzGetCurrentFileInfo(uf, &fi, fzip, sizeof(fzip), NULL, 0, NULL, 0);
                            for (int j = 0; fzip[j]; j++) {
                                if (fzip[j] == '\\') fzip[j] = '/';
                                if ((unsigned char)fzip[j] > 126) fzip[j] = '_';
                            }
                            std::string full = "sdmc:/" + std::string(fzip);
                            if (fzip[strlen(fzip) - 1] == '/') {
                                std::error_code ec;
                                fs::create_directories(full, ec);
                                chmod(full.c_str(), 0777);
                            } else {
                                std::error_code ec;
                                fs::create_directories(fs::path(full).parent_path(), ec);
                                if (fs::exists(full, ec)) {
                                    if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                    continue;
                                }
                                if (unzOpenCurrentFile(uf) == UNZ_OK) {
                                    FILE* out = fopen(full.c_str(), "wb");
                                    if (out) {
                                        char buf[8192]; int r;
                                        while ((r = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0)
                                            fwrite(buf, 1, r, out);
                                        fclose(out);
                                        chmod(full.c_str(), 0777);
                                    }
                                    unzCloseCurrentFile(uf);
                                }
                            }
                            if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                        }
                        unzClose(uf);
                    }
                    std::error_code ec_rm; fs::remove("sdmc:/emulanders/tmp_assets.zip", ec_rm);
                    strncpy(G.status_msg, TR("Assets installed!", "Fotos/assets instalados com sucesso!"), sizeof(G.status_msg) - 1);
                    G.progress.store(1.0f);
                    G.dl_running = false;
                    G.dl_finished_popup = true;
                });
            }
        } else {
            float prog = G.progress.load();
            if (prog >= 0.0f) {
                ImGui::ProgressBar(prog, ImVec2(screen_w * 0.6f, 30 * ui_scale));
            } else {
                ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(screen_w * 0.6f, 30 * ui_scale), TR("Downloading...", "Baixando..."));
                ImGui::Text(TR("%.1f MB downloaded", "%.1f MB baixados"), G.dl_now.load() / (1024.0 * 1024.0));
            }
            ImGui::TextColored(G.accent, "%s", G.status_msg);
        }
    }
}

// ============================================================
// Screen: Settings
// ============================================================

void RenderScreenSettings(float screen_w, float content_h, float ui_scale, bool& just_entered_content, ImFont* f_large, ImFont* f_small) {
    (void)screen_w; (void)f_small;
    if (just_entered_content) {
        ImGui::SetNextWindowFocus();
    }

    float table_h = content_h - 20.0f * ui_scale;
    if (ImGui::BeginChild("SettingsScroll", ImVec2(0, table_h), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        if (f_large) ImGui::PushFont(f_large);
        ImGui::TextColored(G.accent, "%s", TR("APPLICATION LANGUAGE", "IDIOMA DO APLICATIVO"));
        if (f_large) ImGui::PopFont();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.80f, 0.80f, 0.80f, 1.0f), "%s",
            TR("Select interface language (English / Português):",
               "Selecione o idioma da interface (Inglês / Português):"));
        ImGui::Spacing();

        if (G.language == AppLanguage::EN) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button("English", ImVec2(180.0f * ui_scale, 44.0f * ui_scale))) {
            G.language = AppLanguage::EN;
            SaveAppConfig();
        }
        if (G.language == AppLanguage::EN) ImGui::PopStyleColor();

        ImGui::SameLine(0, 15.0f * ui_scale);

        if (G.language == AppLanguage::PT_BR) ImGui::PushStyleColor(ImGuiCol_Button, G.accent);
        if (ImGui::Button("Português (Brasil)", ImVec2(230.0f * ui_scale, 44.0f * ui_scale))) {
            G.language = AppLanguage::PT_BR;
            SaveAppConfig();
        }
        if (G.language == AppLanguage::PT_BR) ImGui::PopStyleColor();

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (f_large) ImGui::PushFont(f_large);
        ImGui::TextColored(G.accent, "%s", TR("THEME & APPEARANCE", "TEMA E APARÊNCIA"));
        if (f_large) ImGui::PopFont();
        ImGui::Spacing();

        ImGui::Text("%s", TR("Color Palette Presets:", "Predefinições de Cores:"));
        ImGui::Spacing();

        struct ColorPreset {
            const char* name;
            ImVec4 color;
        };

        const ColorPreset presets[] = {
            { TR("Cyan (Default)", "Ciano (Padrão)"), ImVec4(0.00f, 0.70f, 0.85f, 1.0f) },
            { TR("Portal Blue",    "Azul Portal"),    ImVec4(0.12f, 0.45f, 0.90f, 1.0f) },
            { TR("Neon Purple",    "Roxo Neon"),      ImVec4(0.68f, 0.20f, 0.90f, 1.0f) },
            { TR("Emerald",        "Esmeralda"),      ImVec4(0.15f, 0.78f, 0.35f, 1.0f) },
            { TR("Amber Gold",     "Ouro Âmbar"),     ImVec4(0.95f, 0.65f, 0.10f, 1.0f) },
            { TR("Crimson",        "Carmesim"),       ImVec4(0.90f, 0.18f, 0.22f, 1.0f) },
            { TR("Hot Pink",       "Rosa Choque"),    ImVec4(0.92f, 0.25f, 0.55f, 1.0f) }
        };

        for (size_t i = 0; i < sizeof(presets) / sizeof(presets[0]); i++) {
            ImGui::PushID((int)i);
            ImGui::PushStyleColor(ImGuiCol_Button, presets[i].color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(std::min(1.0f, presets[i].color.x + 0.1f),
                                                                 std::min(1.0f, presets[i].color.y + 0.1f),
                                                                 std::min(1.0f, presets[i].color.z + 0.1f), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(std::max(0.0f, presets[i].color.x - 0.1f),
                                                                 std::max(0.0f, presets[i].color.y - 0.1f),
                                                                 std::max(0.0f, presets[i].color.z - 0.1f), 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
            if (just_entered_content && i == 0) {
                ImGui::SetItemDefaultFocus();
            }
            if (ImGui::Button(presets[i].name, ImVec2(200.0f * ui_scale, 44.0f * ui_scale))) {
                G.accent = presets[i].color;
                SetupImGuiStyle(ui_scale, G.accent, G.accent_hover, G.accent_active);
            }
            ImGui::PopStyleColor(4);
            if ((i + 1) % 4 != 0 && (i + 1) < sizeof(presets) / sizeof(presets[0])) {
                ImGui::SameLine(0, 10.0f * ui_scale);
            }
            ImGui::PopID();
        }

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Text("%s", TR("Custom Accent Color:", "Cor de Destaque Personalizada:"));
        ImGui::Spacing();

        float rgb[3] = { G.accent.x, G.accent.y, G.accent.z };
        bool col_changed = false;

        ImGui::BeginGroup();
        ImGui::PushItemWidth(340.0f * ui_scale);
        if (ImGui::SliderFloat(TR("Red##accent_r", "Vermelho##accent_r"), &rgb[0], 0.0f, 1.0f, "%.2f"))   col_changed = true;
        if (ImGui::SliderFloat(TR("Green##accent_g", "Verde##accent_g"), &rgb[1], 0.0f, 1.0f, "%.2f")) col_changed = true;
        if (ImGui::SliderFloat(TR("Blue##accent_b", "Azul##accent_b"), &rgb[2], 0.0f, 1.0f, "%.2f"))  col_changed = true;
        ImGui::PopItemWidth();
        ImGui::EndGroup();

        ImGui::SameLine(0, 25.0f * ui_scale);

        ImGui::BeginGroup();
        float sq_size = 86.0f * ui_scale;
        if (ImGui::ColorButton("##accent_color_preview_square", G.accent, ImGuiColorEditFlags_None, ImVec2(sq_size, sq_size))) {
            ImGui::OpenPopup("ColorPickerWheelPopup");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", TR("Click or press (A) to open color palette", "Clique ou pressione (A) para abrir a paleta"));
        }

        if (ImGui::BeginPopup("ColorPickerWheelPopup")) {
            ImGui::TextColored(G.accent, "%s", TR("Color Palette", "Paleta de Cores"));
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::ColorPicker3("##picker_wheel", rgb, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview)) {
                col_changed = true;
            }
            ImGui::Spacing();
            if (ImGui::Button(TR("Close (B)", "Fechar (B)"), ImVec2(220.0f * ui_scale, 42.0f * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown)) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::EndGroup();

        if (col_changed) {
            G.accent = ImVec4(rgb[0], rgb[1], rgb[2], 1.0f);
            SetupImGuiStyle(ui_scale, G.accent, G.accent_hover, G.accent_active);
        }

        ImGui::Spacing(); ImGui::Spacing();
        static Uint32 save_notify_time = 0;
        if (ImGui::Button(TR("Save Configuration", "Salvar Configurações"), ImVec2(340.0f * ui_scale, 48.0f * ui_scale))) {
            SaveAppConfig();
            save_notify_time = SDL_GetTicks();
        }
        if (save_notify_time != 0 && SDL_GetTicks() - save_notify_time < 3500) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.3f, 1.0f), "%s", TR("Settings saved to sdmc:/emulanders/config.json",
                                                                         "Configurações salvas em sdmc:/emulanders/config.json"));
        }

        ImGui::Spacing(); ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (f_large) ImGui::PushFont(f_large);
        ImGui::TextColored(G.accent, "%s", TR("SYSTEM & BACKEND STATUS", "STATUS DO SISTEMA E BACKEND"));
        if (f_large) ImGui::PopFont();
        ImGui::Spacing();

        bool ipc_ok = emulandersIsInitialized();
        ImGui::Text("%s", TR("Sysmodule IPC: ", "IPC Sysmodule: "));
        ImGui::SameLine();
        if (ipc_ok) ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "%s", TR("Connected (emulanders)", "Conectado (emulanders)"));
        else        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s", TR("Not Running / Offline", "Não Executando / Offline"));

        if (ipc_ok) {
            EmulationStatus st = EmulationStatus::Off;
            if (R_SUCCEEDED(emulandersGetEmulationStatus(&st))) {
                ImGui::Text("%s", TR("Emulation State: ", "Estado da Emulação: "));
                ImGui::SameLine();
                if (st == EmulationStatus::On)
                    ImGui::TextColored(G.accent, "%s", TR("Active (Emulating Virtual Portal)", "Ativo (Emulando Portal Virtual)"));
                else
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", TR("Standby (Passthrough Mode)", "Em Espera (Modo Passthrough)"));
            }
            std::string active_fig;
            if (R_SUCCEEDED(emulandersGetActiveSkylander(active_fig))) {
                ImGui::Text("%s", TR("Active Figure: ", "Boneco Ativo: "));
                ImGui::SameLine();
                if (!active_fig.empty())
                    ImGui::TextColored(G.accent, "%s", GetFilename(active_fig).c_str());
                else
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", TR("None (Portal Empty)", "Nenhum (Portal Vazio)"));
            }
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), TR("Figures Directory:  %s", "Diretório de Bonecos:  %s"), FIGURES_DIR);
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "%s", TR("Assets Directory:   sdmc:/emulanders/assets/", "Diretório de Fotos:    sdmc:/emulanders/assets/"));
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "%s", TR("Config File:        sdmc:/emulanders/config.json", "Arquivo de Config:    sdmc:/emulanders/config.json"));
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "%s", TR("Language:           English", "Idioma:              Português (Brasil)"));

        ImGui::EndChild();
    }
}
