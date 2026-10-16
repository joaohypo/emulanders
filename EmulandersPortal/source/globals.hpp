#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <cstdint>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "utils/constants.hpp"
#include "utils/path_utils.hpp"
#include "utils/thread_raii.hpp"
#include "utils/translations.hpp"

enum class ScanState     { IDLE, PREPARING, WAITING_NFC, READING_BLOCKS, WAITING_CONFIRM, DONE, ERROR };
enum class EditorState   { SELECT_DUMP, SELECT_PHOTO, CROP_EDIT };
enum class ClipboardAction { NONE, COPY, CUT };

struct Globals {
    u32 battery_charge = 100;
    char ip_address[32] = "Offline";
    char time_str[32] = "00:00";

    bool in_menu = true;
    int  selected_menu = 0;

    SDL_GameController* controller = nullptr;

    ImVec4 accent        = ImVec4(0.0f, 0.70f, 0.85f, 1.0f);
    ImVec4 accent_hover  = ImVec4(0.1f, 0.80f, 0.95f, 1.0f);
    ImVec4 accent_active = ImVec4(0.0f, 0.60f, 0.75f, 1.0f);

    GLuint tex_icon_dumper   = 0;
    GLuint tex_icon_manager  = 0;
    GLuint tex_icon_editor   = 0;
    GLuint tex_icon_dumpdl   = 0;
    GLuint tex_icon_asset    = 0;
    GLuint tex_icon_settings = 0;

    ScanState scan_state = ScanState::IDLE;
    std::string scan_error_msg;
    std::string scan_suggested_name;
    int scan_progress_blocks = 0;
    std::string scanned_image_path;
    std::atomic<bool> scanned_image_ready{false};
    uint8_t last_scan_data[DUMP_FILE_SIZE] = {0};

    EditorState editor_state = EditorState::SELECT_DUMP;
    std::string editor_photo_path;
    std::string editor_dump_path;
    GLuint editor_photo_tex = 0;
    ImVec2 crop_pos  = ImVec2(100, 100);
    ImVec2 crop_size = ImVec2(LOGO_WIDTH, LOGO_HEIGHT);

    std::atomic<float>  progress{0.0f};
    std::atomic<double> dl_now{0.0};
    char status_msg[512] = "";
    std::atomic<bool> dl_running{false};
    bool dl_finished_popup = false;

    ClipboardAction clipboard_action = ClipboardAction::NONE;
    std::string clipboard_path;
    std::string current_path = FIGURES_DIR;
    std::vector<FileInfo> sd_files;
    bool show_delete_confirm = false;
    bool show_blank_name_warning = false;
    std::string file_to_delete;
    std::atomic<bool> is_deleting{false};
    std::string delete_target_name;
    bool file_manager_needs_focus = false;
    bool file_manager_needs_refresh = false;
    bool menu_needs_nav_focus = true;

    // Cloning Engine (Phase 4)
    bool show_clone_warning = false;
    std::string clone_source_path;

    std::vector<FileInfo> album_photos;
    std::vector<FileInfo> target_dumps;

    ScopedThread bg_thread;

    std::atomic<bool> app_exiting{false};
    AppLanguage language = AppLanguage::EN;

    bool IsThreadBusy() const {
        return is_deleting.load() || dl_running.load() ||
               (scan_state == ScanState::PREPARING ||
                scan_state == ScanState::WAITING_NFC ||
                scan_state == ScanState::READING_BLOCKS);
    }
};

extern Globals G;

inline const char* TR(const char* en, const char* pt) {
    return TranslateString(en, pt, G.language);
}
