#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <switch.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iomanip>

#include <curl/curl.h>
#include <minizip/unzip.h>
#include <mbedtls/sha256.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <EGL/egl.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "PortalAlgos.hpp"
#include "emulanders_ipc.h"

namespace fs = std::filesystem;

// --------------------------------------------------------
// Globals & State
// --------------------------------------------------------
u32 g_battery_charge = 100;
char g_ip_address[32] = "Offline";
char g_time_str[32] = "00:00";
bool g_in_menu = true;
int g_selected_menu = 0;
SDL_GameController* g_controller = nullptr;

// Theme colors (will be settable from Settings screen)
ImVec4 g_accent       = ImVec4(0.0f, 0.70f, 0.85f, 1.0f);
ImVec4 g_accent_hover = ImVec4(0.1f, 0.80f, 0.95f, 1.0f);
ImVec4 g_accent_active= ImVec4(0.0f, 0.60f, 0.75f, 1.0f);

// Menu icon textures
GLuint g_tex_logo     = 0;
GLuint g_tex_icon_dumper  = 0;
GLuint g_tex_icon_manager = 0;
GLuint g_tex_icon_editor  = 0;
GLuint g_tex_icon_dumpdl  = 0;
GLuint g_tex_icon_asset   = 0;
GLuint g_tex_icon_settings= 0;

// Scanner state
enum class ScanState { IDLE, PREPARING, WAITING_NFC, READING_BLOCKS, WAITING_CONFIRM, DONE, ERROR };
ScanState g_scan_state = ScanState::IDLE;
std::string g_scan_error_msg;
std::string g_scan_suggested_name;
int g_scan_progress_blocks = 0;
std::string g_scanned_image_path;
std::atomic<bool> g_scanned_image_ready(false);
uint8_t g_last_scan_data[1024] = {0};

// Editor state
enum class EditorState { SELECT_DUMP, SELECT_PHOTO, CROP_EDIT };
EditorState g_editor_state = EditorState::SELECT_DUMP;
std::string g_editor_photo_path;
std::string g_editor_dump_path;
GLuint g_editor_photo_tex = 0;
ImVec2 g_crop_pos  = ImVec2(100, 100);
ImVec2 g_crop_size = ImVec2(175, 275);

// Download state
std::atomic<float> g_progress(0.0f);
std::atomic<double> g_dl_now(0.0);
std::string g_status_msg;
static bool g_dl_running = false;
static bool g_dl_finished_popup = false;

// File Manager state
enum class ClipboardAction { NONE, COPY, CUT };
ClipboardAction g_clipboard_action = ClipboardAction::NONE;
std::string g_clipboard_path;

static bool g_show_delete_confirm = false;
static bool g_show_blank_name_warning = false;
static std::string g_file_to_delete = "";
static std::atomic<bool> g_is_deleting(false);
static std::string g_delete_target_name;

struct FileInfo {
    std::string name;
    std::string path;
    bool is_dir;
    uintmax_t size;
    GLuint tex = 0;
};

std::vector<FileInfo> g_sd_files;
std::string g_current_path = "sdmc:/emulanders/figures/";
std::vector<FileInfo> g_album_photos;
std::vector<FileInfo> g_target_dumps;

// --------------------------------------------------------
// Path & String Helpers
// --------------------------------------------------------
std::string GetFilename(const std::string& path) {
    std::string p = path;
    if (!p.empty() && p.back() == '/') p.pop_back();
    size_t pos = p.find_last_of('/');
    return (pos != std::string::npos) ? p.substr(pos + 1) : p;
}

std::string GetParentPath(const std::string& path) {
    std::string p = path;
    if (!p.empty() && p.back() == '/') p.pop_back();
    size_t pos = p.find_last_of('/');
    if (pos != std::string::npos && pos > 5) return p.substr(0, pos) + "/";
    return "sdmc:/emulanders/figures/";
}

std::string SanitizeFileName(std::string name) {
    for (auto& c : name) {
        if (c == ' ' || c == '(' || c == ')' || c == '\'' ||
            c == ',' || c == '&' || c == '!' || c == '#') c = '_';
    }
    return name;
}

std::string GetUniqueFileName(const std::string& dir, const std::string& original) {
    std::string filename = GetFilename(original);
    std::string stem = filename, ext;
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos && dot > 0) {
        stem = filename.substr(0, dot);
        ext  = filename.substr(dot);
    }
    std::string dest = dir;
    if (!dest.empty() && dest.back() != '/') dest += "/";

    std::string candidate = filename;
    std::error_code ec;
    int idx = 1;
    while (fs::exists(dest + candidate, ec)) {
        candidate = stem + "(" + std::to_string(idx++) + ")" + ext;
    }
    return candidate;
}

std::string ShowKeyboard(const std::string& initial_text, const std::string& guide_text) {
    SwkbdConfig kbd;
    char out_str[512] = {0};
    if (R_SUCCEEDED(swkbdCreate(&kbd, 0))) {
        swkbdConfigMakePresetDefault(&kbd);
        swkbdConfigSetInitialText(&kbd, initial_text.c_str());
        swkbdConfigSetGuideText(&kbd, guide_text.c_str());
        swkbdShow(&kbd, out_str, sizeof(out_str));
        swkbdClose(&kbd);
    }
    return std::string(out_str);
}

void RefreshFileList(std::string path) {
    g_sd_files.clear();
    std::error_code ec;
    if (!path.empty() && path.back() != '/') path += "/";
    g_current_path = path;

    if (path != "sdmc:/emulanders/figures/")
        g_sd_files.push_back({"<- Go back 1 directory", GetParentPath(path), true, 0});

    if (!fs::exists(path, ec))
        fs::create_directories(path, ec);

    for (const auto& entry : fs::directory_iterator(path, ec)) {
        FileInfo info;
        info.name = entry.path().filename().string();
        info.path = entry.path().string();
        info.is_dir = entry.is_directory(ec);
        if (info.is_dir && info.path.back() != '/') info.path += "/";
        info.size = info.is_dir ? 0 : fs::file_size(entry, ec);
        g_sd_files.push_back(info);
    }

    std::sort(g_sd_files.begin(), g_sd_files.end(), [](const FileInfo& a, const FileInfo& b) {
        if (a.name == "<- Go back 1 directory") return true;
        if (b.name == "<- Go back 1 directory") return false;
        if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
        return a.name < b.name;
    });
}

void RecursiveListFiles(const std::string& dir, const std::vector<std::string>& exts, std::vector<FileInfo>& out) {
    std::string path = dir;
    if (!path.empty() && path.back() != '/') path += "/";

    DIR* d = opendir(path.c_str());
    if (!d) return;

    struct dirent* e;
    while ((e = readdir(d))) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;

        std::string full = path + e->d_name;
        struct stat st;
        if (stat(full.c_str(), &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            RecursiveListFiles(full, exts, out);
        } else {
            std::string name = e->d_name;
            std::string ext = "";
            size_t dot = name.find_last_of('.');
            if (dot != std::string::npos) {
                ext = name.substr(dot);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            }
            for (const auto& target : exts) {
                if (ext == target) {
                    out.push_back({name, full, false, (uintmax_t)st.st_size, 0});
                    break;
                }
            }
        }
    }
    closedir(d);
}

std::string LookupSkylanderName(u32 char_id, u16 variant_id) {
    std::ifstream f("sdmc:/emulanders/assets/skylanders_db.json");
    if (!f.is_open()) return "New_Skylander";

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    // Search by character ID (hex, uppercase then lowercase)
    char id_key[32];
    snprintf(id_key, sizeof(id_key), "\"%06X\":", char_id);
    size_t pos = content.find(id_key);
    if (pos == std::string::npos) {
        snprintf(id_key, sizeof(id_key), "\"%06x\":", char_id);
        pos = content.find(id_key);
    }
    if (pos == std::string::npos) return "Skylander_" + std::to_string(char_id);

    // Extract name field
    size_t name_pos = content.find("\"name\":", pos);
    if (name_pos == std::string::npos) return "Skylander_" + std::to_string(char_id);
    size_t start = content.find("\"", name_pos + 7);
    size_t end   = content.find("\"", start + 1);
    std::string name = content.substr(start + 1, end - start - 1);

    // Try to find variant-specific name
    if (variant_id != 0) {
        char var_key[32];
        snprintf(var_key, sizeof(var_key), "\"%04X\":", variant_id);
        size_t var_pos = content.find(var_key, pos);
        if (var_pos != std::string::npos) {
            size_t v_name_pos = content.find("\"name\":", var_pos);
            if (v_name_pos != std::string::npos) {
                size_t v_s = content.find("\"", v_name_pos + 7);
                size_t v_e = content.find("\"", v_s + 1);
                if (v_s != std::string::npos && v_e != std::string::npos)
                    name = content.substr(v_s + 1, v_e - v_s - 1);
            }
        }
    }
    return name;
}

// --------------------------------------------------------
// POSIX-based robust recursive delete (avoids devkitPro fs::remove_all bugs)
// --------------------------------------------------------
void RobustDelete(const std::string& path) {
    // Strip trailing slashes
    std::string p = path;
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();

    chmod(p.c_str(), 0777);
    DIR* d = opendir(p.c_str());
    if (!d) {
        // Not a directory — just delete the file
        remove(p.c_str());
        return;
    }
    struct dirent* e;
    while ((e = readdir(d))) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        RobustDelete(p + "/" + e->d_name);
    }
    closedir(d);
    chmod(p.c_str(), 0777);
    rmdir(p.c_str());
}

// --------------------------------------------------------
// UI Helpers
// --------------------------------------------------------
void UpdateSystemStatus() {
    u32 battery = 0;
    if (R_SUCCEEDED(psmGetBatteryChargePercentage(&battery))) g_battery_charge = battery;

    u32 ip = 0;
    if (R_SUCCEEDED(nifmGetCurrentIpAddress(&ip)))
        snprintf(g_ip_address, sizeof(g_ip_address), "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
    else
        snprintf(g_ip_address, sizeof(g_ip_address), "Offline");

    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(g_time_str, sizeof(g_time_str), "%H:%M", tm);
}

GLuint LoadTextureFromFile(const char* filename, int* out_w = nullptr, int* out_h = nullptr) {
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

void DrawBatteryIcon(ImDrawList* dl, ImVec2 pos, ImVec2 size, float pct) {
    ImU32 outline = IM_COL32(200, 200, 200, 255);
    ImU32 fill = (pct < 0.2f) ? IM_COL32(215, 50, 50, 255) : IM_COL32(0, 190, 215, 255);
    float body_w = size.x * 0.92f;

    dl->AddRect(pos, ImVec2(pos.x + body_w, pos.y + size.y), outline, 2.0f, 0, 1.5f);
    dl->AddRectFilled(ImVec2(pos.x + body_w, pos.y + size.y * 0.25f),
                      ImVec2(pos.x + size.x, pos.y + size.y * 0.75f), outline, 1.0f);
    float fill_w = (body_w - 4.0f) * pct;
    if (fill_w > 1.0f)
        dl->AddRectFilled(ImVec2(pos.x + 2.0f, pos.y + 2.0f),
                          ImVec2(pos.x + 2.0f + fill_w, pos.y + size.y - 2.0f), fill, 1.0f);
}

void SetupImGuiStyle(float scale) {
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
    c[ImGuiCol_Border]         = g_accent;
    c[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    c[ImGuiCol_CheckMark]      = g_accent;
    c[ImGuiCol_SliderGrab]     = g_accent;
    c[ImGuiCol_Button]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    c[ImGuiCol_ButtonHovered]  = g_accent_hover;
    c[ImGuiCol_ButtonActive]   = g_accent_active;
    c[ImGuiCol_Header]         = g_accent;
    c[ImGuiCol_HeaderHovered]  = g_accent_hover;
    c[ImGuiCol_HeaderActive]   = g_accent_active;
    c[ImGuiCol_Separator]      = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    c[ImGuiCol_TableRowBg]     = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    c[ImGuiCol_TableRowBgAlt]  = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
    c[ImGuiCol_TitleBg]        = ImVec4(0.0f, 0.50f, 0.65f, 1.0f);
    c[ImGuiCol_TitleBgActive]  = g_accent;
}

// --------------------------------------------------------
// Curl Callbacks
// --------------------------------------------------------
size_t CurlWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return fwrite(ptr, size, nmemb, (FILE*)userdata);
}

int CurlDownloadProgressCallback(void* p, double dltotal, double dlnow, double ultotal, double ulnow) {
    g_dl_now.store(dlnow);
    if (dltotal > 0)
        g_progress.store((float)(dlnow / dltotal) * 0.5f);
    else if (dlnow > 0)
        g_progress.store(-1.0f); // Indeterminate (chunked transfer)
    return 0;
}

// --------------------------------------------------------
// Main Application
// --------------------------------------------------------
int main(int argc, char* argv[]) {
    // -- System services init --
    srand(time(NULL));
    socketInitializeDefault();
    psmInitialize();
    nifmInitialize(NifmServiceType_User);
    timeInitialize();
    emulandersInitialize();

    // -- SDL + OpenGL init --
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

    // -- UI Scale (docked = 1080p → scale > 1) --
    int win_w, win_h;
    SDL_GetWindowSize(window, &win_w, &win_h);
    float ui_scale = (float)win_h / 720.0f;

    // -- ImGui init --
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigNavSwapGamepadButtons = true;
    SetupImGuiStyle(ui_scale);

    // -- Fonts (Switch system font) --
    ImFont *f_large = nullptr, *f_title = nullptr, *f_small = nullptr;
    if (R_SUCCEEDED(plInitialize(PlServiceType_User))) {
        PlFontData font;
        if (R_SUCCEEDED(plGetSharedFontByType(&font, PlSharedFontType_Standard))) {
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 22.0f * ui_scale, &cfg);  // default font
            f_large = io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 36.0f * ui_scale, &cfg);
            f_title = io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 36.0f * ui_scale, &cfg);
            f_small = io.Fonts->AddFontFromMemoryTTF(font.address, font.size, 18.0f * ui_scale, &cfg);
        }
    }
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 430 core");

    // -- Load icon textures --
    g_tex_logo         = LoadTextureFromFile("sdmc:/emulanders/res/logo.png");
    g_tex_icon_dumper  = LoadTextureFromFile("sdmc:/emulanders/res/icon_dumper.png");
    g_tex_icon_manager = LoadTextureFromFile("sdmc:/emulanders/res/icon_manager.png");
    g_tex_icon_editor  = LoadTextureFromFile("sdmc:/emulanders/res/icon_editor.png");
    g_tex_icon_dumpdl  = LoadTextureFromFile("sdmc:/emulanders/res/icon_dumpdl.png");
    g_tex_icon_asset   = LoadTextureFromFile("sdmc:/emulanders/res/icon_asset.png");
    g_tex_icon_settings= LoadTextureFromFile("sdmc:/emulanders/res/icon_settings.png");

    // -- Controller --
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) { g_controller = SDL_GameControllerOpen(i); break; }
    }

    // -- Initial state --
    RefreshFileList("sdmc:/emulanders/figures/");
    bool done = false;
    Uint32 last_status_update = 0;
    std::thread* bg_thread = nullptr;
    GLuint scan_texture = 0;
    static bool just_entered_content = true;
    static bool first_boot = true;
    static int g_selected_sd_idx = -1;

    while (!done) {
        // -- Frame begin --
        if (!appletMainLoop()) done = true;
        if (SDL_GetTicks() - last_status_update > 2000) {
            UpdateSystemStatus();
            last_status_update = SDL_GetTicks();
        }
        
        // Initial boot focus hack: Force a D-Pad Down click
        if (first_boot) {
            ImGui::GetIO().AddKeyEvent(ImGuiKey_GamepadDpadDown, true);
            ImGui::GetIO().AddKeyEvent(ImGuiKey_GamepadDpadDown, false);
            first_boot = false;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            // Translate touch to mouse for ImGui (Switch touch support)
            if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP || event.type == SDL_FINGERMOTION) {
                float mx = event.tfinger.x * (float)win_w;
                float my = event.tfinger.y * (float)win_h;
                io.AddMousePosEvent(mx, my);
                if (event.type == SDL_FINGERDOWN)  io.AddMouseButtonEvent(0, true);
                if (event.type == SDL_FINGERUP)    io.AddMouseButtonEvent(0, false);
            }
        }

        // Load scanned image texture on main thread (from bg scan thread)
        if (g_scanned_image_ready.exchange(false)) {
            if (scan_texture) { glDeleteTextures(1, &scan_texture); scan_texture = 0; }
            scan_texture = LoadTextureFromFile(g_scanned_image_path.c_str());
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(0, 0, 0, 0));

        // B button → go back to menu
        bool any_modal_open = ImGui::IsPopupOpen((const char*)NULL, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        if (!any_modal_open && !g_in_menu && (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown) || ImGui::IsKeyPressed(ImGuiKey_Escape))) {
            g_in_menu = true;
            just_entered_content = true;
        }
        // + button → exit app
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadStart))
            done = true;

        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;
        float header_h = screen_h * 0.12f;
        float footer_h = screen_h * 0.08f;
        float content_h = screen_h - header_h - footer_h;

        // ===========================================
        // ROOT WINDOW
        // ===========================================
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(screen_w, screen_h));
        ImGui::Begin("Root", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        // ═══════════════════════════════════════════
        // HEADER (permanent)
        // ═══════════════════════════════════════════
        ImGui::BeginChild("Header", ImVec2(screen_w, header_h), true);
        {
            ImDrawList* hdl = ImGui::GetWindowDrawList();
            float title_cy = (header_h / 2.0f) - (22.0f * ui_scale);

            // Title flush left
            ImGui::SetCursorPos(ImVec2(20.0f * ui_scale, title_cy));
            if (f_title) ImGui::PushFont(f_title);
            ImGui::TextColored(g_accent, "Emulanders");
            ImGui::SameLine(0, 8.0f * ui_scale);
            ImGui::Text("Portal");
            if (f_title) ImGui::PopFont();

            // Version
            ImGui::SameLine(0, 10.0f * ui_scale);
            ImGui::SetCursorPosY((header_h / 2.0f) + (5.0f * ui_scale));
            if (f_small) ImGui::PushFont(f_small);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "v0.9.1");
            if (f_small) ImGui::PopFont();

            // Right side: Time | Battery anim | Battery percentage (Centered IP)
            float info_cy = (header_h / 2.0f) - (11.0f * ui_scale);

            // Centered IP address
            ImVec2 ip_sz = ImGui::CalcTextSize(g_ip_address);
            ImGui::SetCursorPos(ImVec2((screen_w - ip_sz.x) / 2.0f, info_cy));
            ImGui::Text("%s", g_ip_address);

            // Time, pipe, battery, percentage on the right side
            char pct_str[16];
            snprintf(pct_str, sizeof(pct_str), "%d%%", g_battery_charge);
            
            ImVec2 time_sz = ImGui::CalcTextSize(g_time_str);
            ImVec2 pipe_sz = ImGui::CalcTextSize("  |  ");
            float bat_w = 30.0f * ui_scale;
            float bat_h = 15.0f * ui_scale;
            float bat_spacing = 8.0f * ui_scale;
            ImVec2 pct_sz = ImGui::CalcTextSize(pct_str);

            float right_total_w = time_sz.x + pipe_sz.x + bat_w + bat_spacing + pct_sz.x + 20.0f * ui_scale;
            float current_x = screen_w - right_total_w;

            // Time
            ImGui::SetCursorPos(ImVec2(current_x, info_cy));
            ImGui::Text("%s", g_time_str);
            current_x += time_sz.x;

            // Pipe
            ImGui::SetCursorPos(ImVec2(current_x, info_cy));
            ImGui::Text("  |  ");
            current_x += pipe_sz.x;

            // Battery Icon
            ImVec2 hdr_origin = ImGui::GetWindowPos();
            ImVec2 bat_pos = ImVec2(hdr_origin.x + current_x, hdr_origin.y + info_cy + 4.0f * ui_scale);
            DrawBatteryIcon(hdl, bat_pos, ImVec2(bat_w, bat_h), g_battery_charge / 100.0f);
            current_x += bat_w + bat_spacing;

            // Battery percentage
            ImGui::SetCursorPos(ImVec2(current_x, info_cy));
            ImGui::Text("%s", pct_str);
        }
        ImGui::EndChild();

        // ═══════════════════════════════════════════
        // CONTENT AREA
        // ═══════════════════════════════════════════



        if (g_in_menu) {
            // --- MAIN MENU (6 items, no scroll) ---
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            ImGui::BeginChild("Menu", ImVec2(screen_w, content_h), false, ImGuiWindowFlags_NoScrollbar);
            if (just_entered_content) {
                ImGui::SetWindowFocus();
                just_entered_content = false;
            }

            float item_h = content_h / 6.0f;
            float icon_sz = item_h * 0.9f; // Even larger icons in menu

            auto DrawMenuOption = [&](const char* label, int idx, GLuint tex) {
                bool sel = (g_selected_menu == idx);
                ImVec2 p = ImGui::GetCursorScreenPos();

                if (just_entered_content && idx == 0) {
                    ImGui::SetKeyboardFocusHere();
                }

                // Selectable first (background)
                ImGui::PushID(idx);
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
                if (f_large) ImGui::PushFont(f_large);
                
                if (ImGui::Selectable("##opt", sel, 0, ImVec2(screen_w, item_h))) {
                    g_selected_menu = idx;
                    g_in_menu = false;
                    just_entered_content = true;
                }
                if (ImGui::IsItemFocused()) g_selected_menu = idx;
                ImGui::PopID();
                
                // Overlay text + icon (drawn AFTER selectable so it's on top)
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

                // Accent bar on left
                if (sel) {
                    draw->AddRectFilled(p, ImVec2(p.x + 12.0f * ui_scale, p.y + item_h), ImColor(g_accent));
                }
            };

            DrawMenuOption("Skylander Dumper",             0, g_tex_icon_dumper);
            DrawMenuOption("File Manager & Dump Cloning",  1, g_tex_icon_manager);
            DrawMenuOption("Figure Image Editor",          2, g_tex_icon_editor);
            DrawMenuOption("Dump Downloader",              3, g_tex_icon_dumpdl);
            DrawMenuOption("Asset Downloader",             4, g_tex_icon_asset);
            DrawMenuOption("Settings",                     5, g_tex_icon_settings);
            ImGui::EndChild();
            ImGui::PopStyleVar();
        } else {
            // --- CONTENT SCREENS ---
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(100.0f * ui_scale, 20.0f * ui_scale)); // Reduced vertical padding
            ImGui::BeginChild("Content", ImVec2(screen_w, content_h + 15.0f * ui_scale), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);

            if (just_entered_content) {
                ImGui::SetWindowFocus();
                // Ensure focus on first element
                ImGui::SetKeyboardFocusHere();
            }

            const char* titles[] = {"Skylander Dumper", "File Manager & Dump Cloning", "Figure Image Editor",
                                    "Dump Downloader", "Asset Downloader", "Settings"};
            GLuint t_icons[] = {g_tex_icon_dumper, g_tex_icon_manager, g_tex_icon_editor,
                                g_tex_icon_dumpdl, g_tex_icon_asset, g_tex_icon_settings};

            // Sub-header with title + icon (Centered)
            float header_area_h = 60.0f * ui_scale; // Further reduced height as requested
            ImGui::BeginChild("ScreenHeader", ImVec2(0, header_area_h), false);

            {
                if (f_large) ImGui::PushFont(f_large);
                ImVec2 tsize = ImGui::CalcTextSize(titles[g_selected_menu]);
                float icon_sz = header_area_h * 0.80f;
                float total_w = tsize.x + (t_icons[g_selected_menu] ? icon_sz + 30 * ui_scale : 0);
                
                ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x - total_w) / 2.0f, (header_area_h - tsize.y) / 2.0f));
                ImGui::TextColored(g_accent, "%s", titles[g_selected_menu]);
                
                if (t_icons[g_selected_menu]) {
                    ImGui::SameLine(0, 30 * ui_scale);
                    ImGui::SetCursorPosY((header_area_h - icon_sz) / 2.0f);
                    ImGui::Image((void*)(intptr_t)t_icons[g_selected_menu], ImVec2(icon_sz, icon_sz));
                }
                if (f_large) ImGui::PopFont();
            }
            ImGui::EndChild();
            ImGui::Separator();

            if (g_selected_menu == 0) { // DUMPER
                if (g_scan_state == ScanState::IDLE) {
                    float child_w = 900 * ui_scale; 
                    float child_h = 300 * ui_scale;
                    float avail_h = ImGui::GetContentRegionAvail().y;
                    
                    // Bypass parent padding for absolute centering
                    ImGui::SetCursorPosX((screen_w - child_w) / 2.0f - ImGui::GetCursorPosX() + ImGui::GetStyle().WindowPadding.x);
                    ImGui::SetCursorPosY((avail_h - child_h) / 2.0f);
                    
                    ImGui::BeginChild("DumperIdle", ImVec2(child_w, child_h), false, ImGuiWindowFlags_NoScrollbar);
                    ImGui::Spacing();
                    
                    auto CenterText = [&](const char* text, ImVec4 color) {
                        ImGui::PushTextWrapPos(child_w);
                        ImVec2 sz = ImGui::CalcTextSize(text, NULL, false, child_w);
                        ImGui::SetCursorPosX((child_w - sz.x) / 2.0f);
                        ImGui::TextColored(color, "%s", text);
                        ImGui::PopTextWrapPos();
                    };

                    CenterText("Place a physical Skylander on the Right Joy-Con NFC reader.", ImVec4(0.8f, 0.8f, 0.8f, 1));
                    ImGui::Spacing();
                    CenterText("Keep it still and centered on the analog stick.", ImVec4(1, 0.8f, 0, 1));
                    ImGui::Spacing();
                    CenterText("Note: Some third-party controllers only support Amiibo, not Skylander.", ImVec4(0.6f, 0.6f, 0.6f, 1));
                    
                    ImGui::Spacing(); ImGui::Spacing();
                    float btn_w = 350 * ui_scale;
                    ImGui::SetCursorPosX((child_w - btn_w) / 2.0f);
                    if (ImGui::Button("Start Scanning", ImVec2(btn_w, 60 * ui_scale))) {
                        if (emulandersIsInitialized()) emulandersSetEmulationStatus(EmulationStatus::Off);
                        g_scan_state = ScanState::PREPARING;
                        g_scan_progress_blocks = 0;
                        g_scan_error_msg.clear();
                        if (bg_thread) { bg_thread->join(); delete bg_thread; }
                        bg_thread = new std::thread([]() {
                            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                            if (R_FAILED(nfcMfInitialize())) { g_scan_error_msg = "Failed to initialize NFC."; g_scan_state = ScanState::ERROR; return; }
                            NfcDeviceHandle h = {}; s32 total = 0;
                            nfcMfListDevices(&total, &h, 1);
                            nfcMfStartDetection(&h);
                            g_scan_state = ScanState::WAITING_NFC;
                            bool found = false;
                            for (int i = 0; i < 300; i++) {
                                NfcMifareDeviceState ds;
                                nfcMfGetDeviceState(&h, &ds);
                                if (ds == NfcMifareDeviceState_TagFound) { found = true; break; }
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            }
                            if (!found) { g_scan_error_msg = "Timeout: No tag detected."; g_scan_state = ScanState::ERROR; nfcMfStopDetection(&h); nfcMfExit(); return; }
                            g_scan_state = ScanState::READING_BLOCKS;
                            NfcTagInfo tag = {};
                            nfcMfGetTagInfo(&h, &tag);
                            uint8_t dump[1024] = {0};
                            uint8_t uid[4] = {tag.uid.uid[0], tag.uid.uid[1], tag.uid.uid[2], tag.uid.uid[3]};
                            for (int s = 0; s < 16; s++) {
                                uint8_t skey[6];
                                Runes::CalcKeyA(uid, s, skey);
                                NfcMifareReadBlockParameter rp[4] = {};
                                NfcMifareReadBlockData rd[4] = {};
                                for (int b = 0; b < 4; b++) {
                                    rp[b].sector_number = s * 4 + b;
                                    memcpy(rp[b].sector_key.sector_key, skey, 6);
                                    rp[b].sector_key.mifare_command = NfcMifareCommand_AuthA;
                                    rp[b].sector_key.unknown = 1;
                                }
                                if (R_SUCCEEDED(nfcMfReadMifare(&h, rd, rp, 4))) {
                                    for (int b = 0; b < 4; b++)
                                        memcpy(&dump[(s * 4 + b) * 16], rd[b].data, 16);
                                    g_scan_progress_blocks = (s + 1) * 4;
                                    if (s == 0) {
                                        u32 cid = dump[16] | (dump[17] << 8) | (dump[18] << 16);
                                        u16 vid = dump[28] | (dump[29] << 8);
                                        g_scan_suggested_name = LookupSkylanderName(cid, vid);
                                        char ipath[256];
                                        snprintf(ipath, 256, "sdmc:/emulanders/assets/%06X_%04X.png", cid, vid);
                                        if (!fs::exists(ipath))
                                            snprintf(ipath, 256, "sdmc:/emulanders/assets/%06X.png", cid);
                                        g_scanned_image_path = ipath;
                                        g_scanned_image_ready = true;
                                    }
                                } else {
                                    char err[128];
                                    snprintf(err, sizeof(err), "Read fail at sector %d", s);
                                    g_scan_error_msg = err;
                                    g_scan_state = ScanState::ERROR;
                                    break;
                                }
                            }
                            nfcMfStopDetection(&h);
                            nfcMfExit();
                            if (g_scan_state != ScanState::ERROR) {
                                memcpy(g_last_scan_data, dump, 1024);
                                g_scan_state = ScanState::WAITING_CONFIRM;
                            }
                        });
                    }
                    ImGui::EndChild();
                } else if (g_scan_state == ScanState::PREPARING) {
                    ImGui::Text("Initializing NFC hardware...");
                    ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(400 * ui_scale, 20 * ui_scale));
                } else if (g_scan_state == ScanState::WAITING_NFC) {
                    ImGui::TextColored(g_accent, "Waiting for Skylander...");
                    ImGui::Text("Place the figure on the NFC reader now.");
                } else if (g_scan_state == ScanState::READING_BLOCKS) {
                    float cur_y = ImGui::GetCursorPosY();
                    ImGui::BeginGroup();
                    ImGui::Text("Reading data... %d/64 blocks", g_scan_progress_blocks);
                    ImGui::ProgressBar(g_scan_progress_blocks / 64.0f, ImVec2(400 * ui_scale, 20 * ui_scale));
                    ImGui::EndGroup();
                    if (scan_texture) {
                        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 325 * ui_scale); // Moved 100px left from original
                        ImGui::SetCursorPosY(cur_y - 20 * ui_scale);
                        ImGui::BeginGroup();
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Skylander detected!");
                        ImGui::Image((void*)(intptr_t)scan_texture, ImVec2(175 * ui_scale, 275 * ui_scale));
                        ImGui::EndGroup();
                    }
                } else if (g_scan_state == ScanState::WAITING_CONFIRM) {
                    float cur_y = ImGui::GetCursorPosY();
                    ImGui::BeginGroup();
                    ImGui::Text("Identified: %s", g_scan_suggested_name.c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Save with Official Name", ImVec2(300 * ui_scale, 50 * ui_scale))) {
                        std::error_code ec;
                        fs::create_directories("sdmc:/emulanders/figures/myScans/", ec);
                        std::string fname = GetUniqueFileName("sdmc:/emulanders/figures/myScans/",
                                                              SanitizeFileName(g_scan_suggested_name) + ".dump");
                        std::string p = "sdmc:/emulanders/figures/myScans/" + fname;
                        std::ofstream out(p, std::ios::binary);
                        out.write((char*)g_last_scan_data, 1024);
                        out.close();
                        chmod(p.c_str(), 0777);
                        g_scan_state = ScanState::DONE;
                        RefreshFileList(g_current_path);
                    }
                    if (ImGui::Button("Save with Custom Name", ImVec2(300 * ui_scale, 50 * ui_scale))) {
                        std::string res = ShowKeyboard("", "Enter filename");
                        if (!res.empty()) {
                            std::error_code ec;
                            fs::create_directories("sdmc:/emulanders/figures/myScans/", ec);
                            std::string fname = GetUniqueFileName("sdmc:/emulanders/figures/myScans/",
                                                                  SanitizeFileName(res) + ".dump");
                            std::string p = "sdmc:/emulanders/figures/myScans/" + fname;
                            std::ofstream out(p, std::ios::binary);
                            out.write((char*)g_last_scan_data, 1024);
                            out.close();
                            chmod(p.c_str(), 0777);
                            g_scan_state = ScanState::DONE;
                            RefreshFileList(g_current_path);
                        } else {
                            g_show_blank_name_warning = true;
                        }
                    }
                    if (ImGui::Button("Cancel", ImVec2(300 * ui_scale, 50 * ui_scale)))
                        g_scan_state = ScanState::IDLE;
                    ImGui::EndGroup();
                    if (scan_texture) {
                        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 325 * ui_scale); // Moved 100px left from original
                        ImGui::SetCursorPosY(cur_y - 20 * ui_scale);
                        ImGui::BeginGroup();
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "Skylander detected!");
                        ImGui::Image((void*)(intptr_t)scan_texture, ImVec2(175 * ui_scale, 275 * ui_scale));
                        ImGui::EndGroup();
                    }
                } else if (g_scan_state == ScanState::DONE) {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Dump saved successfully!");
                    ImGui::Spacing();
                    if (ImGui::Button("Scan Another", ImVec2(250 * ui_scale, 50 * ui_scale)))
                        g_scan_state = ScanState::IDLE;
                } else if (g_scan_state == ScanState::ERROR) {
                    ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "Scan Error: %s", g_scan_error_msg.c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Try Again", ImVec2(250 * ui_scale, 50 * ui_scale)))
                        g_scan_state = ScanState::IDLE;
                }

                if (g_show_blank_name_warning) {
                    ImGui::OpenPopup("NameWarning");
                    g_show_blank_name_warning = false;
                }
                if (ImGui::BeginPopupModal("NameWarning", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
                    ImGui::Text("File name cannot be empty!");
                    ImGui::Spacing();
                    if (ImGui::Button("OK", ImVec2(120 * ui_scale, 40 * ui_scale)))
                        ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }
            } else if (g_selected_menu == 1) { // FILE MANAGER
                // Utility buttons (NoNav — only touch or L/R)
                ImGui::TextColored(g_accent, "Path:");
                ImGui::SameLine(0, 8.0f * ui_scale);
                ImGui::Text("%s", g_current_path.c_str());
                // ImGui::Spacing(); removed spacing

                float avail_x = ImGui::GetContentRegionAvail().x;
                float btn_w_total = (200 + 250 + 12) * ui_scale;
                ImGui::SetCursorPosX((avail_x - btn_w_total) / 2.0f + 70.0f * ui_scale); // Moved 70px right


                ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
                if (ImGui::Button("(L) Refresh", ImVec2(200 * ui_scale, 45 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
                    RefreshFileList(g_current_path);
                    ImGui::GetIO().AddKeyEvent(ImGuiKey_GamepadDpadUp, true);
                    ImGui::GetIO().AddKeyEvent(ImGuiKey_GamepadDpadUp, false);
                    ImGui::GetIO().AddKeyEvent(ImGuiKey_GamepadDpadDown, true);
                    ImGui::GetIO().AddKeyEvent(ImGuiKey_GamepadDpadDown, false);
                }
                ImGui::SameLine();
                if (ImGui::Button("(R) New Folder", ImVec2(250 * ui_scale, 45 * ui_scale)) || ImGui::IsKeyPressed(ImGuiKey_GamepadR1)) {
                    std::string n = ShowKeyboard("NewFolder", "Folder name");
                    if (!n.empty()) {
                        std::string p = g_current_path + SanitizeFileName(n);
                        mkdir(p.c_str(), 0777);
                        chmod(p.c_str(), 0777);
                        RefreshFileList(g_current_path);
                    }
                }
                ImGui::PopItemFlag();

                // Clipboard indicator
                if (g_clipboard_action != ClipboardAction::NONE) {
                    const char* act = (g_clipboard_action == ClipboardAction::COPY) ? "COPY" : "CUT";
                    ImGui::TextColored(g_accent, "Clipboard [%s]: %s  |  (X) Paste  |  (Y) Clear", act, GetFilename(g_clipboard_path).c_str());
                    if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceUp)) { // X = Paste
                        std::string dest_name = GetUniqueFileName(g_current_path, g_clipboard_path);
                        std::string dest = g_current_path + dest_name;
                        std::error_code ec;
                        if (g_clipboard_action == ClipboardAction::COPY)
                            fs::copy(g_clipboard_path, dest, fs::copy_options::recursive, ec);
                        else {
                            fs::rename(g_clipboard_path, dest, ec);
                            g_clipboard_action = ClipboardAction::NONE;
                        }
                        RefreshFileList(g_current_path);
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft)) // Y = Clear
                        g_clipboard_action = ClipboardAction::NONE;
                }

                // File table
                float table_h = content_h - 140 * ui_scale;
                if (just_entered_content) {
                    ImGui::SetNextWindowFocus();
                }
                if (ImGui::BeginTable("Files", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH, ImVec2(0, table_h))) {
                    if (just_entered_content) ImGui::SetKeyboardFocusHere();
                    
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 120 * ui_scale);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80 * ui_scale);
                    ImGui::TableHeadersRow();

                    for (size_t i = 0; i < g_sd_files.size(); i++) {
                        ImGui::PushID((int)i);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        bool clicked = ImGui::Selectable(g_sd_files[i].name.c_str(), g_selected_sd_idx == (int)i,
                                                         ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
                        if (clicked) {
                            g_selected_sd_idx = (int)i;
                            if (g_sd_files[i].name == "<- Go back 1 directory") {
                                RefreshFileList(g_sd_files[i].path);
                                just_entered_content = true;
                            } else if (!g_sd_files[i].is_dir) {
                                ImGui::OpenPopup("FileActions");
                            }
                        }

                        // For directories, enter with A (FaceRight)
                        if (g_sd_files[i].is_dir && g_sd_files[i].name != "<- Go back 1 directory" && ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
                            RefreshFileList(g_sd_files[i].path);
                            just_entered_content = true;
                        }


                        ImGui::TableSetColumnIndex(1);
                        if (!g_sd_files[i].is_dir) {
                            if (g_sd_files[i].size > 1024*1024)
                                ImGui::Text("%.1f MB", g_sd_files[i].size / (1024.0*1024.0));
                            else if (g_sd_files[i].size > 1024)
                                ImGui::Text("%.1f KB", g_sd_files[i].size / 1024.0);
                            else
                                ImGui::Text("%lu B", (unsigned long)g_sd_files[i].size);
                        }
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%s", g_sd_files[i].is_dir ? "DIR" : "FILE");
                        ImGui::PopID();
                    }

                    // File actions popup
                    if (ImGui::BeginPopup("FileActions") && g_selected_sd_idx >= 0 && g_selected_sd_idx < (int)g_sd_files.size()) {
                        auto& sel = g_sd_files[g_selected_sd_idx];
                        if (sel.is_dir && ImGui::MenuItem("Open")) {
                            RefreshFileList(sel.path);
                            just_entered_content = true;
                        }
                        if (ImGui::MenuItem("Copy")) {
                            g_clipboard_path = sel.path;
                            g_clipboard_action = ClipboardAction::COPY;
                        }
                        if (ImGui::MenuItem("Cut")) {
                            g_clipboard_path = sel.path;
                            g_clipboard_action = ClipboardAction::CUT;
                        }
                        if (ImGui::MenuItem("Rename")) {
                            std::string new_name = ShowKeyboard(sel.name, "New name");
                            if (!new_name.empty() && new_name != sel.name) {
                                std::string dest = g_current_path + SanitizeFileName(new_name);
                                std::error_code ec;
                                fs::rename(sel.path, dest, ec);
                                RefreshFileList(g_current_path);
                            }
                        }
                        if (ImGui::MenuItem("Delete")) {
                            g_file_to_delete = sel.path;
                            g_show_delete_confirm = true;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::EndTable();
                }

                if (g_show_delete_confirm) {
                    ImGui::OpenPopup("DeleteConfirm");
                    g_show_delete_confirm = false;
                }
                if (ImGui::BeginPopupModal("DeleteConfirm", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
                    ImGui::Text("Are you sure you want to delete this?");
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1, 0.2f, 0.2f, 1), "%s", GetFilename(g_file_to_delete).c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("Yes, Delete", ImVec2(140 * ui_scale, 40 * ui_scale))) {
                        g_delete_target_name = GetFilename(g_file_to_delete);
                        g_is_deleting = true;
                        if (bg_thread) { bg_thread->join(); delete bg_thread; }
                        bg_thread = new std::thread([]() {
                            RobustDelete(g_file_to_delete);
                            g_is_deleting = false;
                        });
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(140 * ui_scale, 40 * ui_scale)))
                        ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }

                if (g_is_deleting) {
                    ImGui::OpenPopup("DeletingInProgress");
                }
                if (ImGui::BeginPopupModal("DeletingInProgress", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {
                    ImGui::Text("Deleting: %s", g_delete_target_name.c_str());
                    ImGui::Spacing();
                    ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(300 * ui_scale, 20 * ui_scale), "Please wait...");
                    if (!g_is_deleting) {
                        ImGui::CloseCurrentPopup();
                        RefreshFileList(g_current_path);
                    }
                    ImGui::EndPopup();
                }
            } else if (g_selected_menu == 2) { // IMAGE EDITOR
                if (g_editor_state == EditorState::SELECT_DUMP) {
                    static std::string ed_current_path = "sdmc:/emulanders/figures/";
                    static std::vector<FileInfo> ed_files;
                    if (ed_files.empty() || just_entered_content) {
                        ed_files.clear();
                        std::error_code ec;
                        for (auto const& entry : fs::directory_iterator(ed_current_path, ec)) {
                            FileInfo fi; fi.name = entry.path().filename().string(); fi.path = entry.path().string();
                            fi.is_dir = entry.is_directory(ec);
                            if (fi.is_dir) fi.name = "[DIR] " + fi.name;
                            fi.size = fi.is_dir ? 0 : fs::file_size(entry.path(), ec);
                            if (fi.is_dir || fi.name.find(".dump") != std::string::npos) ed_files.push_back(fi);
                        }
                    }

                    if (f_large) ImGui::PushFont(f_large);
                    ImGui::TextColored(g_accent, "Step 1:");
                    if (f_large) ImGui::PopFont();
                    ImGui::SameLine();
                    ImGui::Text("Select a .dump file (Path: %s)", ed_current_path.c_str());

                    if (ImGui::BeginTable("EditorDumps", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH, ImVec2(0, content_h - 180 * ui_scale))) {
                        ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        if (ed_current_path != "sdmc:/") {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            if (just_entered_content) ImGui::SetKeyboardFocusHere();
                            if (ImGui::Selectable("<- Go back 1 directory", false, ImGuiSelectableFlags_SpanAllColumns)) {
                                ed_current_path = GetParentPath(ed_current_path);
                                ed_files.clear();
                                just_entered_content = true;
                            }
                        }
                        for (size_t i = 0; i < ed_files.size(); i++) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            if (just_entered_content && i == 0 && ed_current_path == "sdmc:/emulanders/figures/") ImGui::SetKeyboardFocusHere();
                            if (ImGui::Selectable(ed_files[i].name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                                if (ed_files[i].is_dir) {
                                    ed_current_path = ed_files[i].path + "/";
                                    ed_files.clear();
                                    just_entered_content = true;
                                } else {
                                    g_editor_dump_path = ed_files[i].path;
                                    g_editor_state = EditorState::SELECT_PHOTO;
                                    g_album_photos.clear();
                                    just_entered_content = true;
                                }
                            }
                        }
                        if (just_entered_content) just_entered_content = false;
                        ImGui::EndTable();
                    }
                } else if (g_editor_state == EditorState::SELECT_PHOTO) {
                    static std::vector<FileInfo> ph_files;
                    if (ph_files.empty()) {
                        for(auto& f : ph_files) if(f.tex) glDeleteTextures(1, &f.tex);
                        ph_files.clear();
                        RecursiveListFiles("sdmc:/Nintendo/Album/", {".jpg", ".jpeg", ".png"}, ph_files);
                        std::sort(ph_files.begin(), ph_files.end(), [](const FileInfo& a, const FileInfo& b){ return a.path > b.path; });
                    }
                    
                    if (f_large) ImGui::PushFont(f_large);
                    ImGui::TextColored(g_accent, "Step 2:");
                    if (f_large) ImGui::PopFont();
                    ImGui::SameLine();
                    ImGui::Text("Select a screenshot (for: %s)", GetFilename(g_editor_dump_path).c_str());

                    float table_height = ImGui::GetContentRegionAvail().y - 30.0f * ui_scale;
                    if (ImGui::BeginTable("EditorPhotos", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH, ImVec2(0, table_height))) {
                        ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableHeadersRow();

                        ImGuiListClipper clipper;
                        clipper.Begin((int)ph_files.size());
                        while (clipper.Step()) {
                            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                                auto& p = ph_files[row];
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);

                                if (p.tex == 0) p.tex = LoadTextureFromFile(p.path.c_str());

                                float item_h = 80.0f * ui_scale;
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (item_h - ImGui::GetTextLineHeight()) * 0.5f);

                                if (p.tex) {
                                    ImGui::Image((void*)(intptr_t)p.tex, ImVec2(item_h * 1.77f, item_h));
                                    ImGui::SameLine();
                                }
                                ImGui::PushID(row);
                                if (ImGui::Selectable(p.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, item_h))) {
                                    g_editor_photo_path = p.path;
                                    if (g_editor_photo_tex) glDeleteTextures(1, &g_editor_photo_tex);
                                    g_editor_photo_tex = LoadTextureFromFile(p.path.c_str());
                                    g_crop_pos = ImVec2(100, 100);
                                    g_crop_size = ImVec2(175, 275);
                                    g_editor_state = EditorState::CROP_EDIT;
                                }
                                ImGui::PopID();
                            }
                        }
                        if (just_entered_content) just_entered_content = false;
                        ImGui::EndTable();
                    }
                } else if (g_editor_state == EditorState::CROP_EDIT) {
                    ImGui::Text("D-Pad: Move  |  R-Stick: Resize  |  A: Save  |  B: Cancel");
                    if (g_editor_photo_tex) {
                        float img_w = 800 * ui_scale, img_h = 450 * ui_scale;
                        ImGui::Image((void*)(intptr_t)g_editor_photo_tex, ImVec2(img_w, img_h));
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        ImVec2 p0 = ImGui::GetItemRectMin();
                        dl->AddRect(ImVec2(p0.x + g_crop_pos.x, p0.y + g_crop_pos.y),
                                    ImVec2(p0.x + g_crop_pos.x + g_crop_size.x, p0.y + g_crop_pos.y + g_crop_size.y),
                                    IM_COL32(255, 255, 0, 255), 0, 0, 3.0f);

                        if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadLeft))  g_crop_pos.x -= 2;
                        if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight)) g_crop_pos.x += 2;
                        if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadUp))    g_crop_pos.y -= 2;
                        if (ImGui::IsKeyDown(ImGuiKey_GamepadDpadDown))  g_crop_pos.y += 2;

                        if (g_controller) {
                            float rx = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f;
                            float ry = SDL_GameControllerGetAxis(g_controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;
                            if (abs(rx) > 0.2f) g_crop_size.x += rx * 3;
                            if (abs(ry) > 0.2f) g_crop_size.y += ry * 3;
                        }
                        // A = Save crop
                        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
                            SDL_Surface* s = IMG_Load(g_editor_photo_path.c_str());
                            if (s) {
                                float scX = (float)s->w / img_w, scY = (float)s->h / img_h;
                                SDL_Rect srcc = {(int)(g_crop_pos.x*scX), (int)(g_crop_pos.y*scY),
                                                 (int)(g_crop_size.x*scX), (int)(g_crop_size.y*scY)};
                                SDL_Surface* crop = SDL_CreateRGBSurfaceWithFormat(0, 175, 275, 32, SDL_PIXELFORMAT_RGBA32);
                                SDL_BlitScaled(s, &srcc, crop, NULL);
                                std::string out = g_editor_dump_path.substr(0, g_editor_dump_path.find_last_of('.')) + ".png";
                                IMG_SavePNG(crop, out.c_str());
                                chmod(out.c_str(), 0777);
                                SDL_FreeSurface(crop);
                                SDL_FreeSurface(s);
                            }
                            g_editor_state = EditorState::SELECT_DUMP;
                            g_target_dumps.clear();
                        }
                        // B = Cancel
                        if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown))
                            g_editor_state = EditorState::SELECT_DUMP;
                    }
                }
            } else if (g_selected_menu == 3) { // DUMP DOWNLOADER
                static char url_buf[512] = "";
                static std::vector<std::string> detected_urls;
                ImGui::TextWrapped("Download .dump files from a remote ZIP URL.");
                ImGui::Spacing(); ImGui::Spacing();
                
                ImGui::PushItemWidth(screen_w * 0.5f);
                ImGui::InputText("Custom URL", url_buf, sizeof(url_buf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
                if (ImGui::Button("(L) Edit URL", ImVec2(150 * ui_scale, 0)) || ImGui::IsKeyPressed(ImGuiKey_GamepadL1)) {
                    std::string res = ShowKeyboard(url_buf, "Enter ZIP URL");
                    if (!res.empty()) {
                        strncpy(url_buf, res.c_str(), sizeof(url_buf) - 1);
                        url_buf[sizeof(url_buf) - 1] = '\0';
                    }
                }
                ImGui::PopItemFlag();
                ImGui::Spacing(); ImGui::Spacing();

                if (!g_dl_running) {
                    if (ImGui::Button("Download & Extract", ImVec2(300 * ui_scale, 50 * ui_scale))) {
                        std::string url = url_buf;
                        if (!url.empty()) {
                            g_progress.store(0.0f);
                            g_dl_running = true;
                            if (bg_thread) { bg_thread->join(); delete bg_thread; }
                            bg_thread = new std::thread([url]() {
                                // Convert GitHub tree URL to archive ZIP
                                std::string target = url;
                                std::string expected_subfolder = "";
                                if (url.find("github.com") != std::string::npos && url.find("/tree/") != std::string::npos) {
                                    size_t p = url.find("/tree/");
                                    std::string branch_and_path = url.substr(p + 6);
                                    size_t slash = branch_and_path.find("/");
                                    if (slash != std::string::npos) {
                                        expected_subfolder = branch_and_path.substr(slash + 1);
                                    }
                                    target = url.substr(0, p) + "/archive/refs/heads/" + branch_and_path.substr(0, slash) + ".zip";
                                }
                                // Download
                                g_status_msg = "Downloading...";
                                CURL* curl = curl_easy_init();
                                FILE* fp = fopen("sdmc:/emulanders/tmp_dump.zip", "wb");
                                curl_easy_setopt(curl, CURLOPT_URL, target.c_str());
                                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
                                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                                curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlDownloadProgressCallback);
                                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                                curl_easy_setopt(curl, CURLOPT_USERAGENT, "EmulandersPortal/1.0");
                                curl_easy_perform(curl);
                                fclose(fp);
                                curl_easy_cleanup(curl);

                                // Extract
                                g_status_msg = "Extracting...";
                                g_progress.store(0.5f); // Start at 50% immediately
                                unzFile uf = unzOpen("sdmc:/emulanders/tmp_dump.zip");
                                if (uf) {
                                    unz_global_info gi;
                                    unzGetGlobalInfo(uf, &gi);
                                    for (uLong i = 0; i < gi.number_entry; i++) {
                                        g_progress.store(0.5f + ((float)i / gi.number_entry) * 0.5f);
                                        std::this_thread::sleep_for(std::chrono::milliseconds(1));

                                        char fzip[512];
                                        unz_file_info fi;
                                        unzGetCurrentFileInfo(uf, &fi, fzip, sizeof(fzip), NULL, 0, NULL, 0);
                                        // Sanitize path
                                        for (int j = 0; fzip[j]; j++) {
                                            if (fzip[j] == '\\') fzip[j] = '/';
                                            if ((unsigned char)fzip[j] > 126) fzip[j] = '_';
                                        }
                                        std::string fzip_str(fzip);
                                        std::string relative_path = fzip_str;

                                        // If downloading a specific github folder, strip the root folder and everything before the target folder
                                        if (!expected_subfolder.empty()) {
                                            size_t pos = fzip_str.find(expected_subfolder);
                                            if (pos == std::string::npos) {
                                                if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                                continue;
                                            }
                                            // The path inside the expected_subfolder
                                            relative_path = fzip_str.substr(pos + expected_subfolder.length());
                                            if (!relative_path.empty() && relative_path[0] == '/') {
                                                relative_path = relative_path.substr(1);
                                            }
                                            // If the entry is the subfolder itself, just skip it
                                            if (relative_path.empty()) {
                                                if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                                continue;
                                            }
                                        } else {
                                            // Normal zip: remove the top-level folder if it's a typical github zip
                                            // e.g., Repo-main/folder/file -> folder/file
                                            size_t first_slash = fzip_str.find('/');
                                            if (first_slash != std::string::npos && first_slash < fzip_str.length() - 1) {
                                                relative_path = fzip_str.substr(first_slash + 1);
                                            } else if (fzip[strlen(fzip) - 1] == '/') {
                                                // Skip the root folder itself
                                                if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                                continue;
                                            }
                                        }

                                        std::string full = "sdmc:/emulanders/figures/Downloads/" + relative_path;

                                        if (fzip[strlen(fzip) - 1] == '/') {
                                            std::error_code ec;
                                            fs::create_directories(full, ec);
                                            chmod(full.c_str(), 0777);
                                        } else {
                                            std::error_code ec;
                                            fs::create_directories(fs::path(full).parent_path(), ec);
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
                                fs::remove("sdmc:/emulanders/tmp_dump.zip");
                                g_status_msg = "Download finished! Files saved to sdmc:/emulanders/figures/Downloads/";
                                g_progress.store(1.0f);
                                g_dl_running = false;
                                g_dl_finished_popup = true;
                            });
                        }
                    }
                } else {
                    float prog = g_progress.load();
                    if (prog >= 0.0f) {
                        ImGui::ProgressBar(prog, ImVec2(screen_w * 0.6f, 30 * ui_scale));
                    } else {
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(screen_w * 0.6f, 30 * ui_scale), "Processing...");
                        ImGui::Text("%.1f MB processed", g_dl_now.load() / (1024.0 * 1024.0));
                    }
                    ImGui::TextColored(g_accent, "%s", g_status_msg.c_str());
                }

                // --- DETECTED URLS LIST ---
                ImGui::Spacing(); ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1), "Detected URLs in sdmc:/emulanders/dumps_urls.txt:");

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
                        if (ImGui::Button("Use", ImVec2(100 * ui_scale, 45 * ui_scale))) {
                            strncpy(url_buf, detected_urls[i].c_str(), sizeof(url_buf)-1);
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndChild();
                }
            } else if (g_selected_menu == 4) { // ASSET DOWNLOADER
                ImGui::TextWrapped("Download the community asset pack (figure images for all Skylanders).");
                ImGui::TextWrapped("This is recommended for the best experience with the scanner and file manager.");
                ImGui::Spacing();
                const std::string asset_url_str = "https://github.com/joaohypo/emulanders/releases/download/0.9.1/emulanders_assets_pack.zip";

                if (!g_dl_running) {
                    if (ImGui::Button("Download Assets", ImVec2(300 * ui_scale, 50 * ui_scale))) {
                        g_progress.store(0.0f);
                        g_dl_running = true;
                        if (bg_thread) { bg_thread->join(); delete bg_thread; }
                        bg_thread = new std::thread([asset_url_str]() {
                            const char* asset_url = asset_url_str.c_str();
                            g_status_msg = "Downloading assets...";
                            CURL* curl = curl_easy_init();
                            FILE* fp = fopen("sdmc:/emulanders/tmp_assets.zip", "wb");
                            curl_easy_setopt(curl, CURLOPT_URL, asset_url);
                            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
                            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlDownloadProgressCallback);
                            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                            curl_easy_setopt(curl, CURLOPT_USERAGENT, "EmulandersPortal/1.0");
                            curl_easy_perform(curl);
                            fclose(fp);
                            curl_easy_cleanup(curl);
                            // Extract
                            g_status_msg = "Extracting assets...";
                            g_progress.store(0.5f);
                            unzFile uf = unzOpen("sdmc:/emulanders/tmp_assets.zip");
                            if (uf) {
                                unz_global_info gi;
                                unzGetGlobalInfo(uf, &gi);
                                for (uLong i = 0; i < gi.number_entry; i++) {
                                    g_progress.store(0.5f + ((float)i / gi.number_entry) * 0.5f);
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
                                        if (unzOpenCurrentFile(uf) == UNZ_OK) {
                                            FILE* out = fopen(full.c_str(), "wb");
                                            char buf[8192]; int r;
                                            while ((r = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0)
                                                fwrite(buf, 1, r, out);
                                            fclose(out);
                                            chmod(full.c_str(), 0777);
                                            unzCloseCurrentFile(uf);
                                        }
                                    }
                                    g_progress.store(0.5f + ((float)i / gi.number_entry) * 0.5f);
                                    if (i + 1 < gi.number_entry) unzGoToNextFile(uf);
                                }
                                unzClose(uf);
                            }
                            fs::remove("sdmc:/emulanders/tmp_assets.zip");
                            g_status_msg = "Assets installed!";
                            g_progress.store(1.0f);
                            g_dl_running = false;
                            g_dl_finished_popup = true;
                        });
                    }
                } else {
                    float prog = g_progress.load();
                    if (prog >= 0.0f) {
                        ImGui::ProgressBar(prog, ImVec2(screen_w * 0.6f, 30 * ui_scale));
                    } else {
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(screen_w * 0.6f, 30 * ui_scale), "Downloading...");
                        ImGui::Text("%.1f MB downloaded", g_dl_now.load() / (1024.0 * 1024.0));
                    }
                    ImGui::TextColored(g_accent, "%s", g_status_msg.c_str());
                }
            } else if (g_selected_menu == 5) { // SETTINGS
                ImGui::Text("Accent Color");
                ImGui::ColorEdit3("##accent", (float*)&g_accent, ImGuiColorEditFlags_NoInputs);
                g_accent_hover = ImVec4(g_accent.x + 0.1f, g_accent.y + 0.1f, g_accent.z + 0.1f, 1.0f);
                g_accent_active = ImVec4(g_accent.x - 0.1f, g_accent.y - 0.1f, g_accent.z - 0.1f, 1.0f);
                SetupImGuiStyle(ui_scale);
                ImGui::Spacing();
                ImGui::Text("Language: English (more coming soon)");
            }
            if (just_entered_content) just_entered_content = false;
            ImGui::EndChild();
            ImGui::PopStyleVar(); // WindowPadding
        }

        // Download finished popup (overlays any screen)
        if (g_dl_finished_popup) {
            ImGui::OpenPopup("DownloadDone");
            g_dl_finished_popup = false;
            RefreshFileList(g_current_path); // auto-refresh
        }
        // Temporary style
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f * ui_scale);
        ImVec2 popup_size(700 * ui_scale, 300 * ui_scale);
        ImGui::SetNextWindowPos(ImVec2((screen_w - popup_size.x) * 0.5f, (screen_h - popup_size.y) * 0.5f));
        ImGui::SetNextWindowSize(popup_size);
        if (ImGui::BeginPopupModal("DownloadDone", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
            // Colored header
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetWindowPos();
            dl->AddRectFilled(p, ImVec2(p.x + popup_size.x, p.y + 50 * ui_scale), ImColor(g_accent));
            
            ImGui::SetCursorPos(ImVec2(30 * ui_scale, 12 * ui_scale));
            if (f_large) ImGui::PushFont(f_large);
            ImGui::TextColored(ImVec4(1,1,1,1), "Download Complete");
            if (f_large) ImGui::PopFont();

            ImGui::SetCursorPos(ImVec2(30 * ui_scale, 80 * ui_scale));
            ImGui::PushTextWrapPos(popup_size.x - 60 * ui_scale);
            ImGui::Text("%s", g_status_msg.c_str());
            ImGui::PopTextWrapPos();

            ImGui::SetCursorPos(ImVec2((popup_size.x - 200 * ui_scale) * 0.5f, popup_size.y - 80 * ui_scale));
            if (ImGui::Button("DISMISS", ImVec2(200 * ui_scale, 55 * ui_scale)))
                ImGui::CloseCurrentPopup();
            
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        // ═══════════════════════════════════════════
        // FOOTER (permanent)
        // ═══════════════════════════════════════════
        ImGui::SetCursorPos(ImVec2(0, screen_h - footer_h));
        ImGui::BeginChild("Footer", ImVec2(screen_w, footer_h), true);
        if (f_small) ImGui::PushFont(f_small);
        ImGui::SetCursorPosY(footer_h / 2.0f - 9.0f * ui_scale);
        ImGui::SetCursorPosX(30.0f * ui_scale);
        
        const char* footer_text = "[D-Pad] Navigate  |  [A] Confirm  |  [B] Back  |  [+] Exit";
        if (g_selected_menu == 1 || g_selected_menu == 2 || g_selected_menu == 3) {
            footer_text = "[D-Pad] Navigate  |  [L-Analog] Scroll  |  [A] Confirm  |  [B] Back  |  [+] Exit";
        }
        
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", footer_text);
        if (f_small) ImGui::PopFont();
        ImGui::EndChild();

        ImGui::End();
        ImGui::PopStyleColor();

        // -- Render --
        ImGui::Render();
        glViewport(0, 0, win_w, win_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------
    if (bg_thread) { bg_thread->join(); delete bg_thread; }

    glDeleteTextures(1, &g_tex_logo);
    glDeleteTextures(1, &g_tex_icon_dumper);
    glDeleteTextures(1, &g_tex_icon_manager);
    glDeleteTextures(1, &g_tex_icon_editor);
    glDeleteTextures(1, &g_tex_icon_asset);
    glDeleteTextures(1, &g_tex_icon_dumpdl);
    glDeleteTextures(1, &g_tex_icon_settings);
    if (scan_texture) glDeleteTextures(1, &scan_texture);
    if (g_editor_photo_tex) glDeleteTextures(1, &g_editor_photo_tex);

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
