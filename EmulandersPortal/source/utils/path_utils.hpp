#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <switch.h>

#include "constants.hpp"

struct FileInfo {
    std::string name;
    std::string path;
    bool is_dir = false;
    uintmax_t size = 0;
    GLuint tex = 0;
    bool tex_checked = false;
};

// Path helpers
std::string GetFilename(const std::string& path);
std::string GetParentPath(const std::string& path);
std::string SanitizeFileName(std::string name);
std::string GetUniqueFileName(const std::string& dir, const std::string& original);
std::string DetectSwitchAlbumDir();
std::string GetDefaultPhotoBrowserDir();

// File listing & management
void RefreshFileList(std::string path, std::vector<FileInfo>& out_files, std::string& current_path);
void RecursiveListFiles(const std::string& dir, const std::vector<std::string>& exts, std::vector<FileInfo>& out);
void RobustDelete(const std::string& path);

// Skylander identification
std::string LookupSkylanderName(u32 char_id, u16 variant_id);

// Keyboard helper
std::string ShowKeyboard(const std::string& initial_text, const std::string& guide_text);

// Configuration persistence
void LoadAppConfig();
void SaveAppConfig();
