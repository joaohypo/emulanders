#pragma once

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "imgui.h"
#include "imgui_internal.h"

GLuint LoadTextureFromFile(const char* filename, int* out_w = nullptr, int* out_h = nullptr);
GLuint LoadTextureFromMemory(const void* data, size_t size, int* out_w = nullptr, int* out_h = nullptr);
void   DrawBatteryIcon(ImDrawList* dl, ImVec2 pos, ImVec2 size, float pct, const ImVec4& accent);
void   SetupImGuiStyle(float scale, const ImVec4& accent, ImVec4& accent_hover, ImVec4& accent_active);
