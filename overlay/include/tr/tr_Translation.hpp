
#pragma once
#include <switch.h>
#include <string>

namespace tr {

    bool Load();
    std::string Translate(const std::string &key);

}

inline std::string operator ""_tr(const char *key_lit, size_t key_lit_size) {
    return tr::Translate(std::string(key_lit, key_lit_size));
}