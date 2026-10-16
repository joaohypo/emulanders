#pragma once

#include "imgui.h"

enum class AppLanguage {
    EN = 0,
    PT_BR,
    ES,
    FR,
    DE,
    IT,
    RU,
    KO,
    ZH_HANS,
    ZH_HANT
};

struct LanguageOption {
    AppLanguage lang;
    const char* code;
    const char* display_name;
};

const LanguageOption* GetSupportedLanguages(int* out_count);
const char* LanguageToCode(AppLanguage lang);
const char* LanguageToDisplayName(AppLanguage lang);
AppLanguage CodeToLanguage(const char* code);

const char* TranslateString(const char* en, const char* pt, AppLanguage lang);
void BuildExtraLanguageGlyphRanges(ImFontGlyphRangesBuilder& builder);
