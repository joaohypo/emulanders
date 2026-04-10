#include <tr/tr_Translation.hpp>
#include <tesla.hpp>
#include <unordered_map>
#include <fstream>
#include <tr/json.hpp>

namespace tr {

    namespace {

        constexpr auto DefaultUnknownString = "???";
        constexpr auto DefaultLanguage = "en";

        using LanguageStrings = std::unordered_map<std::string, std::string>;

        LanguageStrings g_DefaultLanguageStrings;
        LanguageStrings g_SystemLanguageStrings;

        inline bool IsDefaultLanguage(const std::string &lang) {
            return lang == DefaultLanguage;
        }

        std::string ConvertLanguage(const std::string &base_lang) {
            if((base_lang == "en-US") || (base_lang == "en-GB")) {
                return "en";
            }

            if(base_lang == "es-419") {
                return "es";
            }

            if(base_lang == "fr-CA") {
                return "fr";
            }

            return base_lang;
        }

        bool GetSystemLanguage(std::string &out_lang) {
            auto ok = false;
            if(R_SUCCEEDED(setInitialize())) {
                u64 lang_code = 0;
                if(R_SUCCEEDED(setGetSystemLanguage(&lang_code))) {
                    out_lang.assign(reinterpret_cast<const char*>(&lang_code));
                    ok = true;
                }
                setExit();
            }

            return ok;
        }

        inline std::string MakeLanguageFilePath(const std::string &lang) {
            return "sdmc:/emulanders/overlay/lang/" + lang + ".json";
        }

        bool LoadLanguageStrings(const std::string &lang, LanguageStrings &out_strs) {
            out_strs.clear();
            auto ok = false;
            tsl::hlp::doWithSDCardHandle([&]() {
                try {
                    const auto lang_path = MakeLanguageFilePath(lang);
                    std::ifstream ifs(lang_path);
                    const auto lang_json = nlohmann::json::parse(ifs);

                    if(lang_json.count("strings")) {
                        for(const auto &str: lang_json["strings"]) {
                            if(str.count("key") && str.count("value")) {
                                const auto &key = str["key"].get<std::string>();
                                const auto &value = str["value"].get<std::string>();
                                out_strs[key] = value;
                            }
                        }
                    }
                    ok = true;
                }
                catch(std::exception&) {
                    ok = false;
                }
            });
            return ok;
        }

    }

    bool Load() {
        // Load default language
        if(!LoadLanguageStrings(DefaultLanguage, g_DefaultLanguageStrings)) {
            return false;
        }

        std::string base_lang;
        if(!GetSystemLanguage(base_lang)) {
            return false;
        }
        const auto lang = ConvertLanguage(base_lang);
        if(!IsDefaultLanguage(lang)) {
            // If loading fails, default strings will be used
            LoadLanguageStrings(lang, g_SystemLanguageStrings);
        }

        return true;
    }

    std::string Translate(const std::string &key) {
        if(g_SystemLanguageStrings.count(key)) {
            return g_SystemLanguageStrings.at(key);
        }
        else if(g_DefaultLanguageStrings.count(key)) {
            return g_DefaultLanguageStrings.at(key);
        }
        else {
            return DefaultUnknownString;
        }
    }

}