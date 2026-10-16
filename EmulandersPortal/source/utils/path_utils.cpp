#include "path_utils.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>

#include "globals.hpp"

namespace fs = std::filesystem;

std::string GetFilename(const std::string& path) {
    std::string p = path;
    if (!p.empty() && p.back() == '/') p.pop_back();
    size_t pos = p.find_last_of('/');
    return (pos != std::string::npos) ? p.substr(pos + 1) : p;
}

std::string GetParentPath(const std::string& path) {
    std::string p = path;
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();
    size_t pos = p.find_last_of("/\\");
    if (pos != std::string::npos && pos >= 5) return p.substr(0, pos + 1);
    return "sdmc:/";
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

std::string DetectSwitchAlbumDir() {
    std::error_code ec;
    // 1. Check emuMMC configurations (Atmosphere / Hekate)
    if (fs::exists("sdmc:/emuMMC", ec)) {
        for (const auto& entry : fs::directory_iterator("sdmc:/emuMMC", ec)) {
            if (entry.is_directory(ec)) {
                std::string cand = entry.path().string() + "/Nintendo/Album/";
                if (fs::exists(cand, ec)) {
                    return cand;
                }
            }
        }
    }

    // 2. Fall back to sysMMC Nintendo Album
    if (fs::exists("sdmc:/Nintendo/Album", ec)) {
        return "sdmc:/Nintendo/Album/";
    }

    if (fs::exists("sdmc:/Nintendo", ec)) {
        return "sdmc:/Nintendo/";
    }

    return "sdmc:/";
}

std::string GetDefaultPhotoBrowserDir() {
    std::string root = DetectSwitchAlbumDir();
    std::error_code ec;
    if (!fs::exists(root, ec)) return root;

    // Automatically drill down to the latest Year/Month/Day folder if available
    std::string current = root;
    for (int depth = 0; depth < 3; depth++) {
        std::string latest_sub;
        for (const auto& entry : fs::directory_iterator(current, ec)) {
            if (entry.is_directory(ec)) {
                std::string fn = entry.path().filename().string();
                if (fn > latest_sub) {
                    latest_sub = fn;
                }
            }
        }
        if (latest_sub.empty()) break;
        current += latest_sub + "/";
    }
    return current;
}

void RefreshFileList(std::string path, std::vector<FileInfo>& out_files, std::string& current_path) {
    for (auto& f : out_files) {
        if (f.tex) { glDeleteTextures(1, &f.tex); f.tex = 0; }
    }
    out_files.clear();
    std::error_code ec;
    if (!path.empty() && path.back() != '/') path += "/";
    current_path = path;

    if (path != FIGURES_DIR)
        out_files.push_back({"<- Go back 1 directory", GetParentPath(path), true, 0});

    if (!fs::exists(path, ec))
        fs::create_directories(path, ec);

    for (const auto& entry : fs::directory_iterator(path, ec)) {
        FileInfo info;
        info.name = entry.path().filename().string();
        info.path = entry.path().string();
        info.is_dir = entry.is_directory(ec);
        if (info.is_dir && info.path.back() != '/') info.path += "/";
        info.size = info.is_dir ? 0 : fs::file_size(entry, ec);
        out_files.push_back(info);
    }

    std::sort(out_files.begin(), out_files.end(), [](const FileInfo& a, const FileInfo& b) {
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
            std::string ext;
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

void RobustDelete(const std::string& path) {
    std::string p = path;
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();

    chmod(p.c_str(), 0777);
    DIR* d = opendir(p.c_str());
    if (!d) {
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

std::string LookupSkylanderName(u32 char_id, u16 variant_id) {
    std::ifstream f("sdmc:/emulanders/assets/skylanders_db.json");
    if (!f.is_open()) return "New_Skylander";

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    char id_key[32];
    snprintf(id_key, sizeof(id_key), "\"%06X\":", char_id);
    size_t pos = content.find(id_key);
    if (pos == std::string::npos) {
        snprintf(id_key, sizeof(id_key), "\"%06x\":", char_id);
        pos = content.find(id_key);
    }
    if (pos == std::string::npos) return "Skylander_" + std::to_string(char_id);

    size_t name_pos = content.find("\"name\":", pos);
    if (name_pos == std::string::npos) return "Skylander_" + std::to_string(char_id);
    size_t start = content.find("\"", name_pos + 7);
    size_t end   = content.find("\"", start + 1);
    std::string name = content.substr(start + 1, end - start - 1);

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

void LoadAppConfig() {
    // Detect system language from Switch OS (default fallback)
    if (R_SUCCEEDED(setInitialize())) {
        u64 lang_code = 0;
        if (R_SUCCEEDED(setGetSystemLanguage(&lang_code))) {
            std::string base_lang(reinterpret_cast<const char*>(&lang_code));
            G.language = CodeToLanguage(base_lang.c_str());
        }
        setExit();
    }

    std::ifstream f("sdmc:/emulanders/config.json");
    if (!f.is_open()) return;
    std::string line;
    float r = G.accent.x, g = G.accent.y, b = G.accent.z;
    while (std::getline(f, line)) {
        if (line.find("\"accent_r\"") != std::string::npos) sscanf(line.c_str(), "%*[^0-9.]%f", &r);
        if (line.find("\"accent_g\"") != std::string::npos) sscanf(line.c_str(), "%*[^0-9.]%f", &g);
        if (line.find("\"accent_b\"") != std::string::npos) sscanf(line.c_str(), "%*[^0-9.]%f", &b);
        if (line.find("\"language\"") != std::string::npos) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                G.language = CodeToLanguage(line.c_str() + colon + 1);
            }
        }
    }
    f.close();
    G.accent = ImVec4(r, g, b, 1.0f);
    G.accent_hover  = ImVec4(std::min(1.0f, r + 0.1f), std::min(1.0f, g + 0.1f), std::min(1.0f, b + 0.1f), 1.0f);
    G.accent_active = ImVec4(std::max(0.0f, r - 0.1f), std::max(0.0f, g - 0.1f), std::max(0.0f, b - 0.1f), 1.0f);
}

void SaveAppConfig() {
    std::error_code ec;
    fs::create_directories("sdmc:/emulanders", ec);
    std::ofstream f("sdmc:/emulanders/config.json");
    if (!f.is_open()) return;
    f << "{\n";
    f << "  \"accent_r\": " << G.accent.x << ",\n";
    f << "  \"accent_g\": " << G.accent.y << ",\n";
    f << "  \"accent_b\": " << G.accent.z << ",\n";
    f << "  \"language\": \"" << LanguageToCode(G.language) << "\"\n";
    f << "}\n";
    f.close();
    chmod("sdmc:/emulanders/config.json", 0777);
}

