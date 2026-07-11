/**
 * @file render_helpers.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/render_helpers.hpp"

#include "game/game_config.hpp"

#include <array>
#include <string>

namespace ridge_dash {

Color fadeColor(Color color, float alpha)
{
    color.a = static_cast<unsigned char>(game_config::clampf(alpha, 0.0f, 1.0f) * 255.0f);
    return color;
}

bool textureLoaded(const Texture2D& texture)
{
    return texture.id != 0;
}

Texture2D loadSpriteTexture(const char* fileName)
{
    const std::string appDir = GetApplicationDirectory();
    const std::array<std::string, 6> candidates = {
        std::string("assets/sprites/") + fileName,
        appDir + "assets/sprites/" + fileName,
        appDir + "../assets/sprites/" + fileName,
        appDir + "../Resources/assets/sprites/" + fileName, // macOS .app bundle
        appDir + "../share/ridgedash/sprites/" + fileName,
        std::string("/usr/share/APPLaunch/share/ridgedash/sprites/") + fileName,
    };

    for (const std::string& path : candidates) {
        if (!FileExists(path.c_str())) {
            continue;
        }

        Texture2D texture = LoadTexture(path.c_str());
        if (textureLoaded(texture)) {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
            SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
            return texture;
        }
    }

    return {};
}

void unloadSpriteTexture(Texture2D& texture)
{
    if (!textureLoaded(texture)) {
        return;
    }
    UnloadTexture(texture);
    texture = {};
}

void drawSpriteCentered(const Texture2D& texture, Vector2 center, float width, float height, float rotation, Color tint)
{
    DrawTexturePro(texture,
                   Rectangle{0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                   Rectangle{center.x, center.y, width, height},
                   Vector2{width * 0.5f, height * 0.5f},
                   rotation,
                   tint);
}

} // namespace ridge_dash
