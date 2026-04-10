
#pragma once
#include <switch.h>
#include <string>
#include <vector>

namespace ui {

    class PngImage {
        private:
            std::string path;
            bool is_error;
            std::string error_text;
            std::vector<u8> img_buffer;
            int img_buffer_width;
            int img_buffer_height;

            inline void SetError(const std::string &text) {
                this->Reset();
                this->is_error = true;
                this->error_text = text;
            }

        public:
            PngImage() {
                this->Reset();
            }

            ~PngImage() {
                this->Reset();
            }

            inline void Reset() {
                this->path.clear();
                this->error_text.clear();
                this->is_error = false;
                this->img_buffer.clear();
                this->img_buffer_height = 0;
                this->img_buffer_width = 0;
            }

            bool Load(const std::string &png_path, const u32 max_height, const u32 max_width);

            inline const std::string &GetPath() const {
                return this->path;
            }

            inline const u8 *GetRGBABuffer() const {
                if(this->img_buffer.empty()) {
                    return nullptr;
                }
                else {
                    return this->img_buffer.data();
                }
            }

            inline u32 GetWidth() const {
                return this->img_buffer_width;
            }

            inline u32 GetHeight() const {
                return this->img_buffer_height;
            }

            inline bool IsError() const {
                return this->is_error;
            }

            inline const std::string &GetErrorText() const {
                return this->error_text;
            }
    };

}