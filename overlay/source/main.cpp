#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <ui/ui_TeslaExtras.hpp>
#include <emu/emu_Service.hpp>
#include <ui/ui_PngImage.hpp>
#include <tr/tr_Translation.hpp>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace {

    constexpr auto ActionKeyShowHelp = HidNpadButton_Plus;
    constexpr auto ActionKeyEnableEmulation = HidNpadButton_ZR;
    constexpr auto ActionKeyDisableEmulation = HidNpadButton_ZL;
    constexpr auto ActionKeyActivateItem = HidNpadButton_A;
    constexpr auto ActionKeyToggleFavorite = HidNpadButton_Y;
    constexpr auto ActionKeyToogleConnectVirtualSkylander = HidNpadButton_StickR;
    constexpr auto ActionKeyResetActiveVirtualSkylander = HidNpadButton_X;
    
    inline std::string GetActionKeyGlyph(const u64 action_key) {
        for(const auto &info : tsl::impl::KEYS_INFO) {
            if(info.key == action_key) {
                return info.glyph;
            }
        }
        return "?";
    }

}

namespace {

    enum class InitializationStatus {
        Ok,
        TranslationsNotLoaded,
        EmulandersNotPresent,
        OkVersionMismatch,
        EmulandersServiceError,
        OtherServiceError
    };

    enum class Icon {
        Help,
        Reset,
        Favorite
    };

    static const std::unordered_map<Icon, std::string> IconGlyphTable = {
        { Icon::Help, "\uE142" },
        { Icon::Reset, "\uE098" },
        { Icon::Favorite, "\u2605" },
    };

    inline std::string GetIconGlyph(const Icon icon) {
        return IconGlyphTable.at(icon);
    }

}

namespace {

    constexpr emu::Version ExpectedVersion = { VER_MAJOR, VER_MINOR, VER_MICRO, {} };

    constexpr auto FavoritesFile = "sdmc:/emulanders/overlay/favorites.txt";

    InitializationStatus g_InitializationStatus;
    Result g_InitializationResult;

    inline bool IsInitializationOk() {
        return (g_InitializationStatus == InitializationStatus::Ok) || (g_InitializationStatus == InitializationStatus::OkVersionMismatch);
    }

    std::string g_SkylanderDirectory = "sdmc:/emulanders/figures";
    emu::Version g_Version;
    std::string g_ActiveSkylanderPath;
    ui::PngImage g_ActiveSkylanderImage;
    std::vector<std::string> g_Favorites;

    constexpr u32 IconMargin = 5;

    inline u32 GetIconMaxWidth() {
        return (tsl::cfg::LayerWidth / 2) - 2 * IconMargin;
    }

    constexpr u32 IconMaxHeight = 275 - 2 * IconMargin;

    inline bool IsActiveSkylanderValid() {
        return !g_ActiveSkylanderPath.empty();
    }

    inline std::string GetPathWithoutExtension(const std::string &path) {
        auto idx = path.find_last_of('.');
        if(idx != std::string::npos) {
            return path.substr(0, idx);
        }
        return path;
    }

    inline std::string MakeVersionString() {
        if(!IsInitializationOk()) {
            if(g_InitializationStatus == InitializationStatus::TranslationsNotLoaded) {
                return "Unable to load translations!";
            }
            else if(g_InitializationStatus == InitializationStatus::EmulandersNotPresent) {
                return "Emulanders Sysmodule Not Present!";
            }
            else if(g_InitializationStatus == InitializationStatus::EmulandersServiceError) {
                std::stringstream strm;
                strm << "Emulanders Service Error";
                strm << " (0x" << std::hex << std::uppercase << g_InitializationResult << ")";
                return strm.str();
            }
            else if(g_InitializationStatus == InitializationStatus::OtherServiceError) {
                std::stringstream strm;
                strm << "OtherServiceError"_tr;
                strm << " (0x" << std::hex << std::uppercase << g_InitializationResult << ")";
                return strm.str();
            }
        }
        else {
            std::stringstream strm;
            strm << "emulanders " << (int)g_Version.major << "." << (int)g_Version.minor << "." << (int)g_Version.micro;
            strm << " (" << (g_Version.dev_build ? "dev" : "release") << ")";
            if(g_InitializationStatus == InitializationStatus::OkVersionMismatch) {
                strm << "(outdated, expected " << (int)ExpectedVersion.major << "." << (int)ExpectedVersion.minor << "." << (int)ExpectedVersion.micro << ")";
            }
            return strm.str();
        }
        return std::string("Unknown status (...)");
    }

    inline emu::VirtualSkylanderStatus GetActiveVirtualSkylanderStatus() {
        if(IsActiveSkylanderValid()) {
            return emu::GetActiveVirtualSkylanderStatus();
        }
        else {
            return emu::VirtualSkylanderStatus::Invalid;
        }
    }

    void ToggleEmulationStatus() {
        switch(emu::GetEmulationStatus()) {
            case emu::EmulationStatus::On: {
                emu::SetEmulationStatus(emu::EmulationStatus::Off);
                break;
            }
            case emu::EmulationStatus::Off: {
                emu::SetEmulationStatus(emu::EmulationStatus::On);
                break;
            }
        }
    }

    std::string ResolveImagePath(const std::string &path) {
        std::string legacy_path = GetPathWithoutExtension(path) + ".png";
        std::string resolved_path = legacy_path;

        tsl::hlp::doWithSDCardHandle([&]() {
            FILE* f = fopen(legacy_path.c_str(), "rb");
            if (f) {
                fclose(f);
                resolved_path = legacy_path;
                return;
            }
            
            f = fopen(path.c_str(), "rb");
            if (!f) {
                return;
            }
            
            u8 buffer[64];
            size_t read_bytes = fread(buffer, 1, 64, f);
            fclose(f);
            
            if (read_bytes < 32) {
                return;
            }
            
            u32 char_id = buffer[16] | (buffer[17] << 8) | (buffer[18] << 16);
            u16 variant_id = buffer[28] | (buffer[29] << 8);
            
            char asset_path[FS_MAX_PATH];
            if (variant_id != 0) {
                snprintf(asset_path, sizeof(asset_path), "sdmc:/emulanders/assets/%06X_%04X.png", char_id, variant_id);
                f = fopen(asset_path, "rb");
                if (f) {
                    fclose(f);
                    resolved_path = std::string(asset_path);
                    return;
                }
            }
            
            snprintf(asset_path, sizeof(asset_path), "sdmc:/emulanders/assets/%06X.png", char_id);
            f = fopen(asset_path, "rb");
            if (f) {
                fclose(f);
                resolved_path = std::string(asset_path);
                return;
            }
        });

        return resolved_path;
    }

    void LoadActiveSkylander() {
        char active_skylander_path_str[FS_MAX_PATH] = {};
        emu::GetActiveVirtualSkylander(active_skylander_path_str, sizeof(active_skylander_path_str));
        g_ActiveSkylanderPath.assign(active_skylander_path_str);
        if(!g_ActiveSkylanderPath.empty()) {
            g_ActiveSkylanderImage.Load(ResolveImagePath(g_ActiveSkylanderPath), GetIconMaxWidth(), IconMaxHeight);
        } else {
            g_ActiveSkylanderImage.Reset();
        }
    }

    inline void SetActiveVirtualSkylander(const std::string &path) {
        emu::SetActiveVirtualSkylander(path.c_str(), path.size());
        LoadActiveSkylander();
    }

    inline void ResetActiveVirtualSkylander() {
        emu::ResetActiveVirtualSkylander();
        LoadActiveSkylander();
    }

    inline bool IsFavorite(const std::string &path) {
        return std::find(g_Favorites.begin(), g_Favorites.end(), path) != g_Favorites.end();
    }

    inline void AddFavorite(const std::string &path) {
        if(!IsFavorite(path)) {
            g_Favorites.push_back(path);
        }
    }

    inline void RemoveFavorite(const std::string &path) {
        g_Favorites.erase(std::remove(g_Favorites.begin(), g_Favorites.end(), path), g_Favorites.end()); 
    }

    void LoadFavorites() {
        g_Favorites.clear();
        tsl::hlp::doWithSDCardHandle([&]() {
            std::ifstream favs_file(FavoritesFile);
            std::string fav_path_str;
            while(std::getline(favs_file, fav_path_str)) {
                AddFavorite(fav_path_str);
            }
        });
    }

    void SaveFavorites() {
        tsl::hlp::doWithSDCardHandle([&]() {
            std::ofstream file(FavoritesFile, std::ofstream::out | std::ofstream::trunc);
            for(const auto &fav_path: g_Favorites) {
                file << fav_path << std::endl;
            }
        });
    }

    inline std::string GetPathFileName(const std::string &path) {
        return path.substr(path.find_last_of("/") + 1);
    }

    std::vector<std::string> SplitPath(const std::string &path) {
        std::vector<std::string> items;
        std::stringstream ss(path);
        std::string item;
        while(std::getline(ss, item, '/')) {
            items.push_back(item);
        }
        return items;
    }

    inline std::string GetRelativePathTo(const std::string &ref_path, const std::string &in_path) {
        const auto ref_path_items = SplitPath(ref_path);
        const auto in_path_items = SplitPath(in_path);
        u32 i = 0;
        std::string rel_path;
        for(; i < ref_path_items.size(); i++) {
            const auto cur_ref_path_item = ref_path_items.at(i);
            const auto cur_in_path_item = in_path_items.at(i);
            if(cur_ref_path_item != cur_in_path_item) {
                rel_path += "../";
            }
        }
        
        for(u32 j = i; j < in_path_items.size(); j++) {
            rel_path += in_path_items.at(j) + '/';
        }
        if(!rel_path.empty()) {
            rel_path.pop_back();
        }
        return rel_path;
    }

    inline std::string GetBaseDirectory(const std::string &path) {
        return path.substr(0, path.find_last_of("/"));
    }

}

class GuiListElement: public ui::elm::SmallListItem {
    private:
        std::string path;
        std::function<void(GuiListElement&)> action_listener;

    public:
        GuiListElement(const std::string &path, const std::string &label, const std::string &value = "") : ui::elm::SmallListItem(label, value), path(path) {
            this->setClickListener([&] (u64 keys) {
                if(keys & ActionKeyActivateItem) {
                    this->action_listener(*this);
                    return true;
                }
                return false;
            });
        }

        inline void SetActionListener(const std::function<void(GuiListElement&)> &listener) {
            this->action_listener = listener;
        }

        inline const std::string &GetPath() const {
            return this->path;
        }

        inline bool IsFavorite() const {
            return ::IsFavorite(this->path);
        }

        inline void ToggleFavorite() {
            if(this->CanBeFavorite()) {
                if(this->IsFavorite()) {
                    ::RemoveFavorite(this->path);
                } else {
                    ::AddFavorite(this->path);
                }
                this->Update();
            }
        }

        virtual bool CanBeFavorite() const {
            return false;
        }

        inline bool ContainsActiveSkylanderPath() const {
            if(!IsActiveSkylanderValid()) {
                return false;
            }
            return g_ActiveSkylanderPath.find(this->path) == 0;
        }

        virtual void Update() {}
};

class VirtualListElement: public GuiListElement {
    public:
        VirtualListElement(const std::string &label, const std::string &icon_glyph = "") : GuiListElement("", label + (!icon_glyph.empty() ? " " + icon_glyph : ""), "..") {}
};

class ActionListElement: public GuiListElement {
    public:
        ActionListElement(const std::string &label, const std::string &icon_glyph = "") : GuiListElement("", label + (!icon_glyph.empty() ? " " + icon_glyph : ""), "") {}
};

class FolderListElement: public GuiListElement {
    private:
        void Update() override {
            bool is_active_inside = this->ContainsActiveSkylanderPath();
            const std::string value = is_active_inside ? ">> ACTIVE" : "..";
            this->setValue(this->IsFavorite() ? GetIconGlyph(Icon::Favorite) + " " + value : value, !is_active_inside);
        }
    
    public:
        FolderListElement(const std::string &path) : GuiListElement(path, GetPathFileName(path)) {
            this->Update();
        }
};

class SkylanderListElement: public GuiListElement {
    private:
        bool CanBeFavorite() const override {
            return true;
        }

        void Update() override {
            bool is_active = !g_ActiveSkylanderPath.empty() && g_ActiveSkylanderPath == this->GetPath();
            std::string value = is_active ? ">> ACTIVE" : "SKYL";
            
            this->setValue(this->IsFavorite() ? GetIconGlyph(Icon::Favorite) + " " + value : value, !is_active);
        }

    public:
        SkylanderListElement(const std::string &path) : GuiListElement(path, GetPathFileName(path)) {
            this->Update();
        }
};

class SkylanderIcons: public tsl::elm::Element {
    private:
        ui::PngImage cur_skylander_image;
        std::string current_path;
        bool is_folder = false;

    public:
        static constexpr float ErrorTextFontSize = 15;

        void SetCurrentSkylanderPath(const std::string &path) {
            if(this->current_path == path) {
                return;
            }
            this->current_path = path;
            this->is_folder = false;

            if(path.empty()) {
                this->cur_skylander_image.Reset();
                return;
            }
            
            if(path.ends_with(".bin") || path.ends_with(".dump")) {
                this->cur_skylander_image.Load(ResolveImagePath(path), GetIconMaxWidth(), IconMaxHeight);
            } else {
                this->cur_skylander_image.Reset();
                this->is_folder = true;
            }
        }

    private:
        void DrawIcon(tsl::gfx::Renderer* renderer, const s32 x, const s32 y, const s32 w, const s32 h, const ui::PngImage &image, bool folder, const std::string &raw_path) {
            const auto img_buf = image.GetRGBABuffer();
            if(img_buf != nullptr) {
                renderer->drawBitmap(x + IconMargin / 2 + w / 2 - image.GetWidth() / 2, y + IconMargin, image.GetWidth(), image.GetHeight(), img_buf);
            }
            else {
                std::string msg;
                auto text_color = ui::style::color::ColorWarning;

                if (raw_path.empty()) {
                    msg = "NoFigure"_tr;
                } else if (folder) {
                    msg = "FolderSelected"_tr;
                } else {
                    msg = GetPathFileName(GetPathWithoutExtension(raw_path));
                    text_color = tsl::style::color::ColorHighlight;
                }
                
                auto [tw, th] = renderer->drawString(msg.c_str(), false, 0, 0, ErrorTextFontSize, renderer->a(tsl::style::color::ColorTransparent));
                renderer->drawString(msg.c_str(), false, x + w / 2 - tw / 2, y + h / 2, ErrorTextFontSize, renderer->a(text_color));
            }
        }

        void DrawCustom(tsl::gfx::Renderer* renderer, const s32 x, const s32 y, const s32 w, const s32 h) {
            renderer->drawRect(x + w / 2 - 1, y, 1, h - IconMargin, this->a(tsl::style::color::ColorText));
            this->DrawIcon(renderer, x, y, w / 2, h, g_ActiveSkylanderImage, false, g_ActiveSkylanderPath);
            this->DrawIcon(renderer, x + w / 2, y, w / 2, h, this->cur_skylander_image, this->is_folder, this->current_path);
        }

        virtual void draw(tsl::gfx::Renderer* renderer) override {
            renderer->enableScissoring(ELEMENT_BOUNDS(this));
            this->DrawCustom(renderer, ELEMENT_BOUNDS(this));
            renderer->disableScissoring();
        }

        virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {}
};

class SkylanderLegend: public tsl::elm::Element {
    private:
        std::string current_path;

    public:
        void SetCurrentSkylanderPath(const std::string &path) {
            if(this->current_path != path) {
                this->current_path = path;
            }
        }

        virtual void draw(tsl::gfx::Renderer* renderer) override {
            const auto x = this->getX();
            const auto y = this->getY();
            const auto w = this->getWidth();

            const u32 fontSize = 15;
            
            std::string leftText = "LegendActive"_tr;
            std::string rightText = "LegendCursor"_tr;

            auto leftColor = g_ActiveSkylanderPath.empty() ? ui::style::color::ColorWarning : tsl::style::color::ColorHighlight;
            
            bool rightMenuIsValidDump = this->current_path.ends_with(".bin") || this->current_path.ends_with(".dump");
            auto rightColor = rightMenuIsValidDump ? tsl::style::color::ColorHighlight : ui::style::color::ColorWarning;

            auto [lw, lh] = renderer->drawString(leftText.c_str(), false, 0, 0, fontSize, tsl::style::color::ColorTransparent);
            auto [rw, rh] = renderer->drawString(rightText.c_str(), false, 0, 0, fontSize, tsl::style::color::ColorTransparent);

            renderer->drawString(leftText.c_str(), false, std::max((int)x, (int)(x + (w / 4) - (lw / 2))), y + 20, fontSize, renderer->a(leftColor));
            renderer->drawString(rightText.c_str(), false, std::max((int)x + (int)(w / 2), (int)(x + (3 * w / 4) - (rw / 2))), y + 20, fontSize, renderer->a(rightColor));
        }

        virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
            this->setBoundaries(this->getX(), this->getY(), this->getWidth(), 30);
        }
};

class CustomList: public tsl::elm::List {
    private:
        tsl::elm::Element* custom_initial_focus{nullptr};

    public:
        void setCustomInitialFocus(tsl::elm::Element* item) {
            custom_initial_focus = item;
        }

        Element* requestFocus(Element *oldFocus, FocusDirection direction) override {
            auto new_focus = tsl::elm::List::requestFocus(oldFocus, direction);
            if (!new_focus) {
                return nullptr;
            }
            if (direction == FocusDirection::None) {
                auto index = getIndexInList(custom_initial_focus);
                if (index >= 0) {
                    new_focus = custom_initial_focus->requestFocus(oldFocus, FocusDirection::None);
                    if (new_focus) {
                        setFocusedIndex(index);
                    }
                }
                custom_initial_focus = nullptr;
            }
            return new_focus;
        }

};

class SkylanderGuiLogView : public tsl::Gui {
    public:
        virtual tsl::elm::Element* createUI() override {
            auto root_frame = new tsl::elm::OverlayFrame("Live Debug Log", MakeVersionString());
            auto list = new tsl::elm::List();
            
            char* log_data = new char[16384]();
            emu::GetDebugLog(log_data, 16384);
            
            std::string log_str(log_data);
            delete[] log_data;

            std::stringstream ss(log_str);
            std::string line;
            while(std::getline(ss, line)) {
                if(!line.empty()) {
                    list->addItem(new tsl::elm::ListItem(line));
                }
            }
            
            root_frame->setContent(list);
            return root_frame;
        }
};

class LogsToggleElement: public ActionListElement {
    public:
        LogsToggleElement() : ActionListElement("DebugLogging"_tr, "") {
            this->SetActionListener([&] (auto&) {
                bool current = emu::GetLoggingStatus();
                emu::SetLoggingStatus(!current);
                this->Update();
            });
            this->Update();
        }

        void Update() override {
            bool is_logging = emu::GetLoggingStatus();
            this->setColoredValue(is_logging ? "On"_tr : "Off"_tr, is_logging ? tsl::style::color::ColorHighlight : ui::style::color::ColorWarning);
        }
};

class SkylanderGuiLogsMenu : public tsl::Gui {
    private:
        ui::elm::DoubleSectionOverlayFrame *root_frame;
        LogsToggleElement *logging_toggle_item;
        tsl::elm::List *top_list;
        CustomList *bottom_list;

    public:
        virtual tsl::elm::Element* createUI() override {
            this->root_frame = new ui::elm::DoubleSectionOverlayFrame("LogsManager"_tr, MakeVersionString(), ui::SectionsLayout::big_top, true);
            this->top_list = new tsl::elm::List();
            this->root_frame->setTopSection(this->top_list);
            this->bottom_list = new CustomList();
            this->root_frame->setBottomSection(this->bottom_list);

            this->top_list->addItem(new ui::elm::BigCategoryHeader("LogsWarningTitle"_tr, true));
            this->top_list->addItem(new ui::elm::SmallListItem("LogsWarningRAM"_tr));
            this->top_list->addItem(new ui::elm::SmallListItem("LogsWarningTrouble"_tr));
            this->top_list->addItem(new ui::elm::SmallListItem("LogsWarningDisable"_tr));

            this->logging_toggle_item = new LogsToggleElement();
            this->bottom_list->addItem(this->logging_toggle_item);

            auto btn_view = new ActionListElement("ViewRAMLog"_tr, "");
            btn_view->SetActionListener([&](auto&) {
                tsl::changeTo<SkylanderGuiLogView>();
            });
            this->bottom_list->addItem(btn_view);

            auto btn_extract = new ActionListElement("ExtractToSD"_tr, "");
            btn_extract->SetActionListener([&](auto&) {
                char* log_data = new char[16384]();
                emu::GetDebugLog(log_data, 16384);
                std::string log_str(log_data);
                delete[] log_data;

                tsl::hlp::doWithSDCardHandle([&]() {
                    std::ofstream log_file("sdmc:/emulanders/debug_log_dump.txt");
                    if (log_file.is_open()) {
                        log_file << log_str;
                        log_file.flush();
                        log_file.close();
                    }
                });
                tsl::goBack();
            });
            this->bottom_list->addItem(btn_extract);

            auto btn_clear = new ActionListElement("ClearRAMLog"_tr, "");
            btn_clear->SetActionListener([&](auto&) {
                emu::ClearDebugLog();
                tsl::goBack();
            });
            this->bottom_list->addItem(btn_clear);

            update();
            return root_frame;
        }

        virtual void update() override {
            this->logging_toggle_item->Update();
            tsl::Gui::update();
        }
};

class SkylanderGuiHelp : public tsl::Gui {
    public:
        virtual tsl::elm::Element* createUI() override {
            auto root_frame = new ui::elm::DoubleSectionOverlayFrame("Help"_tr, MakeVersionString(), ui::SectionsLayout::big_top, false);
            auto top_list = new tsl::elm::List();
            root_frame->setTopSection(top_list);

            top_list->addItem(new ui::elm::SmallListItem("EnableEmulation"_tr, GetActionKeyGlyph(ActionKeyEnableEmulation)));
            top_list->addItem(new ui::elm::SmallListItem("DisableEmulation"_tr, GetActionKeyGlyph(ActionKeyDisableEmulation)));
            top_list->addItem(new ui::elm::SmallListItem("SelectSkylanderFolder"_tr, GetActionKeyGlyph(ActionKeyActivateItem)));
            top_list->addItem(new ui::elm::SmallListItem("ToggleFavorite"_tr, GetActionKeyGlyph(ActionKeyToggleFavorite)));
            top_list->addItem(new ui::elm::SmallListItem("ClearActiveSkylander"_tr, GetActionKeyGlyph(ActionKeyResetActiveVirtualSkylander)));

            return root_frame;
        }
};

class SkylanderGui : public tsl::Gui {
    public:
        enum class Kind {
            Root,
            Favorites,
            Folder
        };

    private:
        Kind kind;
        std::string base_path;
        ui::elm::DoubleSectionOverlayFrame *root_frame;
        ui::elm::SmallToggleListItem *emulation_toggle_item;
        ui::elm::SmallListItem *game_header;
        tsl::elm::List *top_list;
        SkylanderIcons *skylander_icons;
        SkylanderLegend *skylander_legend;
        CustomList *bottom_list;
        std::vector<GuiListElement*> gui_elements;

    public:
        SkylanderGui(const Kind kind, const std::string &path) : kind(kind), base_path(path), gui_elements(std::vector<GuiListElement*>()) {}

        virtual tsl::elm::Element *createUI() override {
            this->root_frame = new ui::elm::DoubleSectionOverlayFrame("emulanders", MakeVersionString(), ui::SectionsLayout::big_top, true);

            this->top_list = new tsl::elm::List();
            this->root_frame->setTopSection(this->top_list);
            this->bottom_list = new CustomList();
            this->root_frame->setBottomSection(this->bottom_list);

            this->skylander_icons = new SkylanderIcons();
            this->top_list->addItem(this->skylander_icons, IconMaxHeight + 2 * IconMargin);

            this->skylander_legend = new SkylanderLegend();
            this->top_list->addItem(this->skylander_legend);

            if(!IsInitializationOk()) {
                return this->root_frame;
            }

            if(this->kind == Kind::Root) {
                this->bottom_list->addItem(createSkylandersElement());
                this->bottom_list->addItem(createFavoritesElement());
                this->bottom_list->addItem(createResetElement());
                this->bottom_list->addItem(createHelpElement());
                this->bottom_list->addItem(createLogsMenuElement());
            }
            else {
                u32 skylander_count = 0;
                std::vector<std::string> dir_paths;

                if(this->kind == Kind::Favorites) {
                    dir_paths = g_Favorites;
                }
                else if(this->kind == Kind::Folder) {
                    tsl::hlp::doWithSDCardHandle([&]() {
                        auto dir = opendir(this->base_path.c_str());
                        if(dir) {
                            while(true) {
                                auto entry = readdir(dir);
                                if(entry == nullptr) {
                                    break;
                                }
                                if(entry->d_type & DT_DIR) {
                                    if(std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
                                    const auto dir_path = this->base_path + "/" + entry->d_name;
                                    dir_paths.push_back(dir_path);
                                }
                                else if(entry->d_type & DT_REG) {
                                    std::string name = entry->d_name;
                                    if(name.ends_with(".bin") || name.ends_with(".dump")) {
                                        const auto file_path = this->base_path + "/" + entry->d_name;
                                        dir_paths.push_back(file_path);
                                    }
                                }
                            }
                            closedir(dir);
                        }
                    });
                }

                std::sort(dir_paths.begin(), dir_paths.end());
                for(const auto &dir_path: dir_paths) {
                    GuiListElement *new_item = this->createSkylanderElement(dir_path);
                    if(new_item) {
                        skylander_count++;
                    }
                    else {
                        new_item = this->createFolderElement(dir_path);
                    }
    
                    this->bottom_list->addItem(new_item);
                    this->gui_elements.push_back(new_item);
                    if(new_item->ContainsActiveSkylanderPath()) {
                        this->bottom_list->setCustomInitialFocus(new_item);
                    }
                }

                this->bottom_list->addItem(new ui::elm::CustomCategoryHeader("AvailableSkylanders"_tr + " '" + GetPathFileName(this->base_path) + "': " + std::to_string(skylander_count), true, true), 0, 0);
            }

            this->emulation_toggle_item = new ui::elm::SmallToggleListItem("EmulationStatus"_tr + " " + GetActionKeyGlyph(ActionKeyDisableEmulation) + " " + GetActionKeyGlyph(ActionKeyEnableEmulation), false, "On"_tr, "Off"_tr);
            this->emulation_toggle_item->setClickListener([&](u64 keys) {
                if(keys & ActionKeyActivateItem) {
                    ToggleEmulationStatus();
                    return true;
                }
                return false;
            });
            this->top_list->addItem(this->emulation_toggle_item);

            this->game_header = new ui::elm::SmallListItem("InterceptingGame"_tr);
            this->top_list->addItem(this->game_header);

            this->root_frame->setClickListener([&](u64 keys) {
                if(keys & ActionKeyShowHelp) {
                    tsl::changeTo<SkylanderGuiHelp>();
                    return true;
                }
                if(keys & ActionKeyEnableEmulation) {
                    emu::SetEmulationStatus(emu::EmulationStatus::On);
                    return true;
                }
                if(keys & ActionKeyDisableEmulation) {
                    emu::SetEmulationStatus(emu::EmulationStatus::Off);
                    return true;
                }
                if(keys & ActionKeyResetActiveVirtualSkylander) {
                    ResetActiveVirtualSkylander();
                    return true;
                }
                if(auto gui_item = dynamic_cast<GuiListElement*>(getFocusedElement())) {
                    if(keys & ActionKeyToggleFavorite) {
                        gui_item->ToggleFavorite();
                        return true;
                    }
                }
                return false;
            });

            update();

            return root_frame;
        }

        virtual void update() override {
            if(!IsInitializationOk()) {
                return;
            }

            for(auto item : this->gui_elements) {
                item->Update();
            }

            const auto is_intercepted = emu::IsCurrentApplicationIdIntercepted();

            this->game_header->setColoredValue(is_intercepted ? "Yes"_tr : "No"_tr, is_intercepted ? tsl::style::color::ColorHighlight : ui::style::color::ColorWarning);

            if(auto skylander_item = dynamic_cast<GuiListElement*>(getFocusedElement())) {
                this->skylander_icons->SetCurrentSkylanderPath(skylander_item->GetPath());
                this->skylander_legend->SetCurrentSkylanderPath(skylander_item->GetPath());
            } else {
                this->skylander_icons->SetCurrentSkylanderPath("");
                this->skylander_legend->SetCurrentSkylanderPath("");
            }

            this->emulation_toggle_item->setState(emu::GetEmulationStatus() == emu::EmulationStatus::On);

            tsl::Gui::update();
        }

    private:
        VirtualListElement* createFavoritesElement() {
            auto item = new VirtualListElement("ViewFavorites"_tr, GetIconGlyph(Icon::Favorite));
            item->SetActionListener([&](auto&) {
                tsl::changeTo<SkylanderGui>(Kind::Favorites, "<favorites>");
            });
            return item;
        }

        ActionListElement* createLogsMenuElement() {
            auto item = new ActionListElement("LogsManager"_tr, "");
            item->SetActionListener([&](auto&) {
                tsl::changeTo<SkylanderGuiLogsMenu>();
            });
            return item;
        }

        ActionListElement* createResetElement() {
            auto item = new ActionListElement("ClearActiveSkylander"_tr, GetIconGlyph(Icon::Reset));
            item->SetActionListener([&](auto&) {
                ResetActiveVirtualSkylander();
                update();
            });
            return item;
        }

        ActionListElement* createHelpElement() {
            auto item = new ActionListElement("Help"_tr, GetIconGlyph(Icon::Help));
            item->SetActionListener([&](auto&) {
                tsl::changeTo<SkylanderGuiHelp>();
            });
            return item;
        }

        VirtualListElement* createSkylandersElement() {
            auto item = new VirtualListElement("ViewFiguresFolder"_tr);
            item->SetActionListener([&] (auto&) {
                tsl::changeTo<SkylanderGui>(Kind::Folder, g_SkylanderDirectory);
                static bool is_first_time = true;
                if(is_first_time && IsActiveSkylanderValid()) {
                    const auto active_skylander_rel_dir = GetBaseDirectory(GetRelativePathTo(g_SkylanderDirectory, g_ActiveSkylanderPath));
                    auto incremental_path = g_SkylanderDirectory;
                    for(const auto &dir_item: SplitPath(active_skylander_rel_dir)) {
                        incremental_path += "/" + dir_item;
                        tsl::changeTo<SkylanderGui>(Kind::Folder, incremental_path);
                    }
                }
                is_first_time = false;
            });
            return item;
        }

        FolderListElement* createFolderElement(const std::string &path) {
            auto item = new FolderListElement(path);
            item->SetActionListener([&](auto& caller) {
                tsl::changeTo<SkylanderGui>(Kind::Folder, caller.GetPath());
            });
            return item;
        }

        SkylanderListElement* createSkylanderElement(const std::string &path) {
            if(!path.ends_with(".bin") && !path.ends_with(".dump")) {
                return nullptr;
            }

            auto item = new SkylanderListElement(path);
            item->SetActionListener([&](auto& caller) {
                const auto path = caller.GetPath();
                if(g_ActiveSkylanderPath == path) {
                    ResetActiveVirtualSkylander();
                } else {
                    SetActiveVirtualSkylander(path);
                }
                update();
            });
            return item;
        }
};

class EmulandersOverlay : public tsl::Overlay {
    public:
        virtual void initServices() override {
            if(!tr::Load()) {
                g_InitializationStatus = InitializationStatus::TranslationsNotLoaded;
                return;
            }
            if(!emu::IsAvailable()) {
                g_InitializationStatus = InitializationStatus::EmulandersNotPresent;
                return;
            }
            g_InitializationResult = emu::Initialize();
            if(R_FAILED(g_InitializationResult)) {
                g_InitializationStatus = InitializationStatus::EmulandersServiceError;
                return;
            }
            g_InitializationResult = pmdmntInitialize();
            if(R_FAILED(g_InitializationResult)) {
                g_InitializationStatus = InitializationStatus::OtherServiceError;
                return;
            }
            g_InitializationResult = nsInitialize();
            if(R_FAILED(g_InitializationResult)) {
                g_InitializationStatus = InitializationStatus::OtherServiceError;
                return;
            }

            g_Version = emu::GetVersion();
            if(!g_Version.EqualsExceptBuild(ExpectedVersion)) {
                g_InitializationStatus = InitializationStatus::OkVersionMismatch;
            }

            g_InitializationStatus = InitializationStatus::Ok;
        }

        virtual void exitServices() override {
            SaveFavorites();
            nsExit();
            pmdmntExit();
            emu::Exit();
        }

        virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
            LoadActiveSkylander();
            LoadFavorites();
            return initially<SkylanderGui>(SkylanderGui::Kind::Root, "<root>");
        }
};

int main(int argc, char **argv) {
    return tsl::loop<EmulandersOverlay, tsl::impl::LaunchFlags::CloseOnExit>(argc, argv);
}