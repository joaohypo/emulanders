#pragma once

#include <cstddef>

constexpr size_t DUMP_FILE_SIZE       = 1024;
constexpr size_t DUMP_BLOCKS          = 64;
constexpr size_t DUMP_BLOCK_SIZE      = 16;
constexpr size_t DUMP_SECTORS         = 16;
constexpr size_t DUMP_BLOCKS_PER_SECTOR = 4;

constexpr size_t MAX_PATH             = 768;
constexpr size_t URL_BUFFER_SIZE      = 512;
constexpr size_t IPC_PATH_BUFFER_SIZE = 512;

constexpr size_t LOGO_WIDTH           = 175;
constexpr size_t LOGO_HEIGHT          = 275;

constexpr const char* FIGURES_DIR     = "sdmc:/emulanders/figures/";
constexpr const char* ASSETS_DIR      = "sdmc:/emulanders/assets/";
constexpr const char* RES_DIR         = "sdmc:/emulanders/res/";
constexpr const char* ALBUM_DIR       = "sdmc:/Nintendo/Album/";
constexpr const char* DOWNLOADS_SUBDIR = "Downloads/";
constexpr const char* SCANS_SUBDIR     = "myScans/";

constexpr const char* EMULANDERS_SERVICE = "emulande";
constexpr const char* APP_USER_AGENT     = "EmulandersPortal/1.0";
