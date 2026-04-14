#include <ui/ui_PngImage.hpp>
#include <tr/tr_Translation.hpp>
#include <ui/ui_TeslaExtras.hpp>
#include <ui/upng.h>

namespace ui {

    bool PngImage::Load(const std::string &png_path, const u32 max_width, const u32 max_height) {
        this->Reset();

        tsl::hlp::doWithSDCardHandle([&]() {
            auto upng = upng_new_from_file(png_path.c_str());
            if(upng == nullptr) {
                this->SetError("UpngInvalidFile"_tr);
                return;
            }

            upng_decode(upng);
            switch(upng_get_error(upng)) {
                case UPNG_EOK: {
                    const auto png_fmt = upng_get_format(upng);
                    const auto is_rgb = (png_fmt == UPNG_RGB8) || (png_fmt == UPNG_RGB16);
                    /*
                    *  DELETE ONCE RGB DOWNSCALE WORKS PROPERLY
                    */
                    if(is_rgb) {
                        this->SetError("UpngUnsupportedRgbPng"_tr);
                        break;
                    }
                    /* DELETE END */

                    const auto upng_width = upng_get_width(upng);
                    const auto upng_height = upng_get_height(upng);
                    const auto bpp = upng_get_bpp(upng);
                    const auto bit_depth = upng_get_bitdepth(upng);
                    const auto img_depth = bpp / bit_depth;
                    const auto scale1 = (double)max_height / (double)upng_height;
                    const auto scale2 = (double)max_width / (double)upng_width;
                    const auto scale = std::min({scale1, scale2, 1.0});

                    this->path = png_path;
                    this->img_buffer_width = (int)((double)upng_width * scale);
                    this->img_buffer_height = (int)((double)upng_height * scale);
                    this->img_buffer.resize(img_buffer_width * img_buffer_height * img_depth);
                    std::fill(img_buffer.begin(), img_buffer.end(), 0);

                    const auto img_buf = upng_get_buffer(upng);
                    for(auto h = 0; h < this->img_buffer_height; h++) {
                        for(auto w = 0; w < this->img_buffer_width; w++) {
                            const auto pixel = h * img_buffer_width * img_depth + w * img_depth;
                            const auto nearest_match = ((int)(h / scale) * (upng_width * img_depth)) + ((int)(w / scale) * img_depth);
                            for(u32 d = 0; d < img_depth; d++) {
                                this->img_buffer[pixel + d] = img_buf[nearest_match + d];
                            }
                        }
                    }
                    break;
                }
                case UPNG_ENOMEM: {
                    this->SetError("UpngImageTooLarge"_tr);
                    break;
                }
                case UPNG_ENOTFOUND: {
                    this->SetError("UpngImageNotFound"_tr);
                    break;
                }
                case UPNG_ENOTPNG: {
                    this->SetError("UpngNotPngImage"_tr);
                    break;
                }
                case UPNG_EMALFORMED: {
                    this->SetError("UpngMalformedPng"_tr);
                    break;
                }
                case UPNG_EUNSUPPORTED: {
                    this->SetError("UpngUnsupportedPng"_tr);
                    break;
                }
                case UPNG_EUNINTERLACED: {
                    this->SetError("UpngUnsupportedInterlacing"_tr);
                    break;
                }
                case UPNG_EUNFORMAT: {
                    this->SetError("UpngUnsupportedColorFormat"_tr);
                    break;
                }
                case UPNG_EPARAM: {
                    this->SetError("UpngInvalidParameter"_tr);
                    break;
                }
            }
            upng_free(upng);
        });

        return this->is_error;
    }

}