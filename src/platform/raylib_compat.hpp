/**
 * @file raylib_compat.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#if defined(RIDGEDASH_USE_PLATFORM_COMPAT)

#include "platform/raylib_compat_types.hpp"
#include <cstdarg>

constexpr Color WHITE{255, 255, 255, 255};
constexpr Color RAYWHITE{245, 245, 245, 255};
constexpr Color BLACK{0, 0, 0, 255};
constexpr float RAD2DEG = 57.29577951308232f;

constexpr int FLAG_VSYNC_HINT = 0x00000040;
constexpr int TEXTURE_FILTER_POINT = 0;
constexpr int TEXTURE_WRAP_CLAMP = 1;

enum KeyboardKey {
    KEY_NULL = 0,
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_ZERO = 48,
    KEY_ONE = 49,
    KEY_TWO = 50,
    KEY_THREE = 51,
    KEY_FOUR = 52,
    KEY_FIVE = 53,
    KEY_SIX = 54,
    KEY_SEVEN = 55,
    KEY_EIGHT = 56,
    KEY_NINE = 57,
    KEY_A = 65,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_R = 82,
    KEY_S = 83,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Z = 90,
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
};

void SetConfigFlags(unsigned int flags);
void InitWindow(int width, int height, const char* title);
bool IsWindowReady();
void SetTargetFPS(int fps);
bool WindowShouldClose();
float GetFrameTime();
void BeginDrawing();
void EndDrawing();
void CloseWindow();

bool IsKeyDown(int key);
bool IsKeyPressed(int key);

void ClearBackground(Color color);
void DrawRectangle(int posX, int posY, int width, int height, Color color);
void DrawRectangleLines(int posX, int posY, int width, int height, Color color);
void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);
void DrawText(const char* text, int posX, int posY, int fontSize, Color color);
int MeasureText(const char* text, int fontSize);
Vector2 RidgeDashMeasureTextEx(const char* text, float fontSize, float spacing);
void RidgeDashDrawTextPro(const char* text,
                          Vector2 position,
                          Vector2 origin,
                          float rotation,
                          float fontSize,
                          float spacing,
                          Color color);

Texture2D LoadTexture(const char* fileName);
void UnloadTexture(Texture2D texture);
void SetTextureFilter(Texture2D texture, int filter);
void SetTextureWrap(Texture2D texture, int wrap);
void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);

bool FileExists(const char* fileName);
const char* GetApplicationDirectory();
const char* TextFormat(const char* text, ...);
unsigned int RidgeDashPlatformTimeMs();

inline int RidgeDashDefaultRenderScale()
{
    return 1;
}

inline void RidgeDashInputInit() {}
inline void RidgeDashInputShutdown() {}
inline bool RidgeDashWindowShouldClose()
{
    return WindowShouldClose();
}

#else

#include <raylib.h>

inline int RidgeDashDefaultRenderScale()
{
#if defined(RIDGEDASH_DRM_RENDER) || defined(RIDGEDASH_DESKTOP_RENDER)
    return 2;
#else
    return 1;
#endif
}

#if defined(RIDGEDASH_USE_EVDEV_INPUT)
void RidgeDashInputInit();
void RidgeDashInputShutdown();
bool RidgeDashWindowShouldClose();
bool RidgeDashIsKeyDown(int key);
bool RidgeDashIsKeyPressed(int key);

#if !defined(RIDGEDASH_RAYLIB_COMPAT_NO_KEY_MACROS)
#define IsKeyDown RidgeDashIsKeyDown
#define IsKeyPressed RidgeDashIsKeyPressed
#endif
#else
inline void RidgeDashInputInit() {}
inline void RidgeDashInputShutdown() {}
inline bool RidgeDashWindowShouldClose()
{
    return WindowShouldClose();
}
#endif

#endif
