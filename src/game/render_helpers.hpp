/**
 * @file render_helpers.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "platform/raylib_compat.hpp"

namespace ridge_dash {

Color fadeColor(Color color, float alpha);
bool textureLoaded(const Texture2D& texture);
Texture2D loadSpriteTexture(const char* fileName);
void unloadSpriteTexture(Texture2D& texture);
void drawSpriteCentered(const Texture2D& texture,
                        Vector2 center,
                        float width,
                        float height,
                        float rotation,
                        Color tint = WHITE);

} // namespace ridge_dash
