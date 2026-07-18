#if defined(RIDGEDASH_3DS)

#include "platform/raylib_compat_types.hpp"

#include <3ds.h>
#include <citro2d.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace {

constexpr float kDepth = 0.5f;
constexpr int kCanvasWidth = RIDGEDASH_SCREEN_WIDTH;
constexpr int kCanvasHeight = RIDGEDASH_SCREEN_HEIGHT;
constexpr size_t kCommandBufferSize = C3D_DEFAULT_CMDBUF_SIZE * 2;
constexpr size_t kMaxObjects = C2D_DEFAULT_MAX_OBJECTS * 2;
constexpr float kTopScreenWidth = 400.0f;
constexpr float kTopScreenHeight = 240.0f;
constexpr float kCanvasRenderScale = 2.0f;
constexpr int kCanvasRenderWidth = static_cast<int>(kCanvasWidth * kCanvasRenderScale);
constexpr int kCanvasRenderHeight = static_cast<int>(kCanvasHeight * kCanvasRenderScale);
constexpr int kCanvasTextureWidth = 1024;
constexpr int kCanvasTextureHeight = 512;
constexpr float kCanvasDisplayScale = kTopScreenWidth / kCanvasWidth;
constexpr float kCanvasDisplayHeight = kCanvasHeight * kCanvasDisplayScale;
constexpr float kCanvasDisplayY = (kTopScreenHeight - kCanvasDisplayHeight) * 0.5f;

// raylib key values used by GameInput. Keeping them local avoids name clashes
// with libctru's KEY_A/KEY_X/etc. constants in this translation unit.
constexpr int kKeySpace = 32;
constexpr int kKeyFive = 53;
constexpr int kKeySeven = 55;
constexpr int kKeyA = 65;
constexpr int kKeyC = 67;
constexpr int kKeyD = 68;
constexpr int kKeyE = 69;
constexpr int kKeyF = 70;
constexpr int kKeyG = 71;
constexpr int kKeyR = 82;
constexpr int kKeyS = 83;
constexpr int kKeyW = 87;
constexpr int kKeyX = 88;
constexpr int kKeyZ = 90;
constexpr int kKeyEscape = 256;
constexpr int kKeyEnter = 257;
constexpr int kKeyRight = 262;
constexpr int kKeyLeft = 263;
constexpr int kKeyDown = 264;
constexpr int kKeyUp = 265;

constexpr std::array<const char*, 29> kSpriteNames = {
    "cactus.png",       "car_body_day.png", "car_body_night.png",  "coin.png",          "driver.png",
    "driver_blink.png", "driver_cried.png", "driver_helmeted.png", "driver_scared.png", "driver_shocked.png",
    "flea.png",         "fuel_can.png",     "giant_flea.png",      "helmet.png",        "magnet.png",
    "magnet_small.png", "moon.png",         "moon_blink.png",      "rocket.png",        "sailboat.png",
    "snowman.png",      "squid_a.png",      "squid_b.png",         "squid_c.png",       "squid_d.png",
    "sun.png",          "sun_blink.png",    "wheel_day.png",       "wheel_night.png",
};

constexpr std::array<uint8_t, 95> kDefaultFontWidths = {
    3, 1, 4, 6, 5, 7, 6, 2, 3, 3, 5, 5, 2, 4, 1, 7, 5, 2, 5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 3, 4, 3, 6,
    7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 5, 6, 5, 7, 6, 6, 6, 6, 6, 6, 7, 6, 7, 7, 6, 6, 6, 2, 7, 2, 3, 5,
    2, 5, 5, 5, 5, 5, 4, 5, 5, 1, 2, 5, 2, 5, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 3, 1, 3, 4,
};

constexpr int kDefaultFontFirstCharacter = 32;
constexpr int kDefaultFontBaseSize = 10;
constexpr int kDefaultFontAtlasSize = 128;
constexpr int kDefaultFontDivisor = 1;

struct ThreeDsState {
    C3D_RenderTarget* top = nullptr;
    C3D_RenderTarget* bottom = nullptr;
    C3D_RenderTarget* canvas = nullptr;
    C3D_Tex canvasTexture{};
    Tex3DS_SubTexture canvasSubTexture{};
    bool canvasTextureInitialized = false;
    C2D_SpriteSheet sprites = nullptr;
    C2D_SpriteSheet font = nullptr;
    u32 keysHeld = 0;
    u32 keysDown = 0;
    u64 lastFrameMs = 0;
    float frameTime = 1.0f / 60.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    bool ready = false;
    bool aptRunning = true;
    bool gfxInitialized = false;
    bool romfsInitialized = false;
    bool c3dInitialized = false;
    bool c2dInitialized = false;
};

ThreeDsState g;

u32 color32(Color color)
{
    return C2D_Color32(color.r, color.g, color.b, color.a);
}

const char* baseName(const char* path)
{
    if (!path) {
        return "";
    }
    const char* slash = std::strrchr(path, '/');
    return slash ? slash + 1 : path;
}

int spriteIndex(const char* path)
{
    const char* name = baseName(path);
    for (size_t i = 0; i < kSpriteNames.size(); ++i) {
        if (std::strcmp(name, kSpriteNames[i]) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

struct FontGlyph {
    int x;
    int y;
    int width;
};

size_t fontGlyphIndex(char character)
{
    const unsigned char value = static_cast<unsigned char>(character);
    if (value < kDefaultFontFirstCharacter || value >= kDefaultFontFirstCharacter + kDefaultFontWidths.size()) {
        return static_cast<size_t>('?' - kDefaultFontFirstCharacter);
    }
    return static_cast<size_t>(value - kDefaultFontFirstCharacter);
}

FontGlyph fontGlyph(size_t wantedIndex)
{
    int line = 0;
    int currentX = kDefaultFontDivisor;
    int nextX = kDefaultFontDivisor;

    for (size_t i = 0; i <= wantedIndex; ++i) {
        FontGlyph glyph{
            currentX, kDefaultFontDivisor + line * (kDefaultFontBaseSize + kDefaultFontDivisor), kDefaultFontWidths[i]};
        nextX += glyph.width + kDefaultFontDivisor;
        if (nextX >= kDefaultFontAtlasSize) {
            ++line;
            currentX = 2 * kDefaultFontDivisor + glyph.width;
            nextX = currentX;
            glyph.x = kDefaultFontDivisor;
            glyph.y = kDefaultFontDivisor + line * (kDefaultFontBaseSize + kDefaultFontDivisor);
        } else {
            currentX = nextX;
        }
        if (i == wantedIndex) {
            return glyph;
        }
    }
    return {};
}

C2D_Image fontGlyphImage(const FontGlyph& glyph, Tex3DS_SubTexture& subTexture)
{
    const C2D_Image atlas = C2D_SpriteSheetGetImage(g.font, 0);
    const Tex3DS_SubTexture& source = *atlas.subtex;

    subTexture.width = static_cast<u16>(glyph.width);
    subTexture.height = kDefaultFontBaseSize;
    if (Tex3DS_SubTextureRotated(&source)) {
        const float unitX = (source.right - source.left) / source.width;
        const float unitY = (source.bottom - source.top) / source.height;
        subTexture.left = source.left + glyph.x * unitX;
        subTexture.right = subTexture.left + glyph.width * unitX;
        subTexture.top = source.top + glyph.y * unitY;
        subTexture.bottom = subTexture.top + kDefaultFontBaseSize * unitY;
    } else {
        const float unitX = (source.right - source.left) / source.width;
        const float unitY = (source.top - source.bottom) / source.height;
        subTexture.left = source.left + glyph.x * unitX;
        subTexture.right = subTexture.left + glyph.width * unitX;
        subTexture.top = source.top - glyph.y * unitY;
        subTexture.bottom = subTexture.top - kDefaultFontBaseSize * unitY;
    }
    return C2D_Image{atlas.tex, &subTexture};
}

u32 nativeKeyForGameKey(int key)
{
    switch (key) {
        case kKeyRight:
            return KEY_RIGHT | KEY_CPAD_RIGHT;
        case kKeyLeft:
            return KEY_LEFT | KEY_CPAD_LEFT;
        case kKeyDown:
        case kKeyS:
            return KEY_DOWN;
        case kKeyUp:
        case kKeyW:
            return KEY_UP;
        case kKeyEnter:
        case kKeyE:
            return KEY_A;
        case kKeySpace:
            return KEY_R;
        case kKeyA:
            return KEY_L;
        case kKeyZ:
            return KEY_B;
        case kKeyR:
            return KEY_X;
        case kKeyG:
            return KEY_Y;
        case kKeyEscape:
            return KEY_START;
        case kKeyFive:
            return KEY_B;
        case kKeySeven:
            return KEY_A;
        case kKeyC:
        case kKeyD:
        case kKeyF:
        case kKeyX:
        default:
            return 0;
    }
}

void pointOnRotatedRectangle(const Rectangle& rec,
                             const Vector2& origin,
                             float radians,
                             float localX,
                             float localY,
                             float& outX,
                             float& outY)
{
    const float x = localX - origin.x;
    const float y = localY - origin.y;
    const float cs = std::cos(radians);
    const float sn = std::sin(radians);
    outX = rec.x + g.offsetX + x * cs - y * sn;
    outY = rec.y + g.offsetY + x * sn + y * cs;
}

bool initSupersampledCanvas()
{
    if (!C3D_TexInitVRAM(&g.canvasTexture, kCanvasTextureWidth, kCanvasTextureHeight, GPU_RGBA8)) {
        return false;
    }
    g.canvasTextureInitialized = true;
    C3D_TexSetFilter(&g.canvasTexture, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&g.canvasTexture, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    g.canvas = C3D_RenderTargetCreateFromTex(&g.canvasTexture, GPU_TEXFACE_2D, 0, -1);
    if (!g.canvas) {
        return false;
    }

    g.canvasSubTexture.width = kCanvasRenderWidth;
    g.canvasSubTexture.height = kCanvasRenderHeight;
    g.canvasSubTexture.left = 0.0f;
    g.canvasSubTexture.right = static_cast<float>(kCanvasRenderWidth) / kCanvasTextureWidth;
    g.canvasSubTexture.top = 1.0f;
    g.canvasSubTexture.bottom = 1.0f - static_cast<float>(kCanvasRenderHeight) / kCanvasTextureHeight;
    return true;
}

void beginSupersampledCanvasScene()
{
    C2D_SceneBegin(g.canvas);
    C2D_ViewReset();
    C2D_ViewScale(kCanvasRenderScale, kCanvasRenderScale);
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
}

void drawSupersampledCanvas()
{
    C2D_SceneBegin(g.top);
    C2D_ViewReset();

    constexpr u32 scissorTop = 0;
    constexpr u32 scissorBottom = static_cast<u32>(kTopScreenWidth);
    constexpr u32 scissorLeft = static_cast<u32>(kCanvasDisplayY);
    constexpr u32 scissorRight = static_cast<u32>(kCanvasDisplayY + kCanvasDisplayHeight + 1.0f);
    C3D_SetScissor(GPU_SCISSOR_NORMAL, scissorLeft, scissorTop, scissorRight, scissorBottom);

    const C2D_Image image{&g.canvasTexture, &g.canvasSubTexture};
    const C2D_DrawParams params{
        {0.0f, kCanvasDisplayY, kTopScreenWidth, kCanvasDisplayHeight},
        {0.0f, 0.0f},
        kDepth,
        0.0f,
    };
    C2D_DrawImage(image, &params, nullptr);
}

} // namespace

Vector2 RidgeDashMeasureTextEx(const char* text, float fontSize, float spacing);
void RidgeDashDrawTextPro(const char* text,
                          Vector2 position,
                          Vector2 origin,
                          float rotation,
                          float fontSize,
                          float spacing,
                          Color color);

void SetConfigFlags(unsigned int) {}

void InitWindow(int width, int height, const char*)
{
    if (width != kCanvasWidth || height != kCanvasHeight) {
        return;
    }

    gfxInitDefault();
    g.gfxInitialized = true;
    g.romfsInitialized = R_SUCCEEDED(romfsInit());
    g.c3dInitialized = C3D_Init(kCommandBufferSize);
    if (!g.c3dInitialized) {
        return;
    }
    g.c2dInitialized = C2D_Init(kMaxObjects);
    if (!g.c2dInitialized) {
        return;
    }

    C2D_Prepare();
    C2D_SetTintMode(C2D_TintMult);
    g.top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    g.bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (g.romfsInitialized) {
        g.sprites = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
        g.font = C2D_SpriteSheetLoad("romfs:/gfx/font.t3x");
        if (g.font && C2D_SpriteSheetCount(g.font) > 0) {
            const C2D_Image fontAtlas = C2D_SpriteSheetGetImage(g.font, 0);
            C3D_TexSetFilter(fontAtlas.tex, GPU_NEAREST, GPU_NEAREST);
        }
    }
    const bool canvasReady = initSupersampledCanvas();
    g.lastFrameMs = osGetTime();
    g.ready = g.top && g.bottom && g.sprites && g.font && canvasReady;
}

bool IsWindowReady()
{
    return g.ready;
}

void SetTargetFPS(int) {}

bool WindowShouldClose()
{
    g.aptRunning = aptMainLoop();
    if (!g.aptRunning) {
        return true;
    }

    hidScanInput();
    g.keysDown = hidKeysDown();
    g.keysHeld = hidKeysHeld();
    return (g.keysDown & KEY_SELECT) != 0;
}

float GetFrameTime()
{
    const u64 now = osGetTime();
    if (g.lastFrameMs != 0) {
        g.frameTime = std::clamp(static_cast<float>(now - g.lastFrameMs) / 1000.0f, 1.0f / 240.0f, 0.1f);
    }
    g.lastFrameMs = now;
    return g.frameTime;
}

void BeginDrawing()
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(g.top, C2D_Color32(0, 0, 0, 255));
    C2D_TargetClear(g.bottom, C2D_Color32(0, 0, 0, 255));
    C2D_TargetClear(g.canvas, C2D_Color32(0, 0, 0, 255));
    g.offsetX = 0.0f;
    g.offsetY = 0.0f;
    beginSupersampledCanvasScene();
}

void EndDrawing()
{
    C2D_Flush();
    drawSupersampledCanvas();
    C3D_FrameEnd(0);
}

void CloseWindow()
{
    if (g.font) {
        C2D_SpriteSheetFree(g.font);
        g.font = nullptr;
    }
    if (g.sprites) {
        C2D_SpriteSheetFree(g.sprites);
        g.sprites = nullptr;
    }
    if (g.canvas) {
        C3D_RenderTargetDelete(g.canvas);
        g.canvas = nullptr;
    }
    if (g.canvasTextureInitialized) {
        C3D_TexDelete(&g.canvasTexture);
        g.canvasTextureInitialized = false;
    }
    if (g.c2dInitialized) {
        C2D_Fini();
        g.c2dInitialized = false;
    }
    if (g.c3dInitialized) {
        C3D_Fini();
        g.c3dInitialized = false;
    }
    if (g.romfsInitialized) {
        romfsExit();
        g.romfsInitialized = false;
    }
    if (g.gfxInitialized) {
        gfxExit();
        g.gfxInitialized = false;
    }
    g.ready = false;
}

bool IsKeyDown(int key)
{
    const u32 nativeKey = nativeKeyForGameKey(key);
    return nativeKey != 0 && (g.keysHeld & nativeKey) != 0;
}

bool IsKeyPressed(int key)
{
    const u32 nativeKey = nativeKeyForGameKey(key);
    return nativeKey != 0 && (g.keysDown & nativeKey) != 0;
}

void ClearBackground(Color color)
{
    C2D_DrawRectSolid(g.offsetX, g.offsetY, kDepth, kCanvasWidth, kCanvasHeight, color32(color));
}

void DrawRectangle(int posX, int posY, int width, int height, Color color)
{
    C2D_DrawRectSolid(posX + g.offsetX, posY + g.offsetY, kDepth, width, height, color32(color));
}

void DrawRectangleLines(int posX, int posY, int width, int height, Color color)
{
    const u32 nativeColor = color32(color);
    const float x = posX + g.offsetX;
    const float y = posY + g.offsetY;
    C2D_DrawLine(x, y, nativeColor, x + width, y, nativeColor, 1.0f, kDepth);
    C2D_DrawLine(x + width, y, nativeColor, x + width, y + height, nativeColor, 1.0f, kDepth);
    C2D_DrawLine(x + width, y + height, nativeColor, x, y + height, nativeColor, 1.0f, kDepth);
    C2D_DrawLine(x, y + height, nativeColor, x, y, nativeColor, 1.0f, kDepth);
}

void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color)
{
    const float radians = rotation * 0.01745329251994329577f;
    std::array<float, 8> p{};
    pointOnRotatedRectangle(rec, origin, radians, 0.0f, 0.0f, p[0], p[1]);
    pointOnRotatedRectangle(rec, origin, radians, rec.width, 0.0f, p[2], p[3]);
    pointOnRotatedRectangle(rec, origin, radians, rec.width, rec.height, p[4], p[5]);
    pointOnRotatedRectangle(rec, origin, radians, 0.0f, rec.height, p[6], p[7]);
    const u32 nativeColor = color32(color);
    C2D_DrawTriangle(p[0], p[1], nativeColor, p[2], p[3], nativeColor, p[4], p[5], nativeColor, kDepth);
    C2D_DrawTriangle(p[0], p[1], nativeColor, p[4], p[5], nativeColor, p[6], p[7], nativeColor, kDepth);
}

void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color)
{
    const u32 nativeColor = color32(color);
    C2D_DrawLine(startPos.x + g.offsetX,
                 startPos.y + g.offsetY,
                 nativeColor,
                 endPos.x + g.offsetX,
                 endPos.y + g.offsetY,
                 nativeColor,
                 std::max(1.0f, thick),
                 kDepth);
}

void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color)
{
    const u32 nativeColor = color32(color);
    C2D_DrawTriangle(v1.x + g.offsetX,
                     v1.y + g.offsetY,
                     nativeColor,
                     v2.x + g.offsetX,
                     v2.y + g.offsetY,
                     nativeColor,
                     v3.x + g.offsetX,
                     v3.y + g.offsetY,
                     nativeColor,
                     kDepth);
}

void DrawText(const char* text, int posX, int posY, int fontSize, Color color)
{
    fontSize = std::max(fontSize, kDefaultFontBaseSize);
    const int spacing = fontSize / kDefaultFontBaseSize;
    RidgeDashDrawTextPro(text,
                         Vector2{static_cast<float>(posX), static_cast<float>(posY)},
                         Vector2{0.0f, 0.0f},
                         0.0f,
                         static_cast<float>(fontSize),
                         static_cast<float>(spacing),
                         color);
}

int MeasureText(const char* text, int fontSize)
{
    fontSize = std::max(fontSize, kDefaultFontBaseSize);
    const int spacing = fontSize / kDefaultFontBaseSize;
    return static_cast<int>(RidgeDashMeasureTextEx(text, static_cast<float>(fontSize), static_cast<float>(spacing)).x);
}

Vector2 RidgeDashMeasureTextEx(const char* text, float fontSize, float spacing)
{
    if (!text || !*text) {
        return {};
    }

    fontSize = std::max(fontSize, static_cast<float>(kDefaultFontBaseSize));
    const float scale = fontSize / kDefaultFontBaseSize;
    float width = 0.0f;
    int count = 0;
    for (const char* p = text; *p; ++p) {
        width += fontGlyph(fontGlyphIndex(*p)).width * scale;
        ++count;
    }
    width += std::max(0, count - 1) * spacing;
    return Vector2{width, fontSize};
}

void RidgeDashDrawTextPro(const char* text,
                          Vector2 position,
                          Vector2 origin,
                          float rotation,
                          float fontSize,
                          float spacing,
                          Color color)
{
    if (!text || !g.font || C2D_SpriteSheetCount(g.font) == 0) {
        return;
    }

    fontSize = std::max(fontSize, static_cast<float>(kDefaultFontBaseSize));
    const float scale = fontSize / kDefaultFontBaseSize;
    const float radians = rotation * 0.01745329251994329577f;
    float cursor = 0.0f;
    C2D_ImageTint tint;
    C2D_PlainImageTint(&tint, color32(color), 1.0f);

    for (const char* p = text; *p; ++p) {
        const FontGlyph glyph = fontGlyph(fontGlyphIndex(*p));
        if (*p != ' ' && *p != '\t') {
            Tex3DS_SubTexture subTexture{};
            const C2D_Image image = fontGlyphImage(glyph, subTexture);
            const C2D_DrawParams params = {
                {position.x + g.offsetX, position.y + g.offsetY, glyph.width * scale, fontSize},
                {origin.x - cursor, origin.y},
                kDepth,
                radians,
            };
            C2D_DrawImage(image, &params, &tint);
        }
        cursor += glyph.width * scale + spacing;
    }
}

Texture2D LoadTexture(const char* fileName)
{
    const int index = spriteIndex(fileName);
    if (index < 0 || !g.sprites || static_cast<size_t>(index) >= C2D_SpriteSheetCount(g.sprites)) {
        return {};
    }
    const C2D_Image image = C2D_SpriteSheetGetImage(g.sprites, static_cast<size_t>(index));
    return Texture2D{static_cast<unsigned int>(index + 1), image.subtex->width, image.subtex->height, 1, 0};
}

void UnloadTexture(Texture2D) {}
void SetTextureFilter(Texture2D, int) {}
void SetTextureWrap(Texture2D, int) {}

void DrawTexturePro(Texture2D texture, Rectangle, Rectangle dest, Vector2 origin, float rotation, Color tint)
{
    if (!g.sprites || texture.id == 0) {
        return;
    }
    const size_t index = static_cast<size_t>(texture.id - 1);
    if (index >= C2D_SpriteSheetCount(g.sprites) || texture.width == 0 || texture.height == 0) {
        return;
    }

    C2D_Sprite sprite;
    C2D_SpriteFromSheet(&sprite, g.sprites, index);
    C2D_SpriteSetScale(&sprite, dest.width / texture.width, dest.height / texture.height);
    C2D_SpriteSetCenter(&sprite, origin.x / dest.width, origin.y / dest.height);
    C2D_SpriteSetPos(&sprite, dest.x + g.offsetX, dest.y + g.offsetY);
    C2D_SpriteSetRotationDegrees(&sprite, rotation);
    C2D_SpriteSetDepth(&sprite, kDepth);

    C2D_ImageTint imageTint;
    C2D_PlainImageTint(&imageTint, color32(tint), 1.0f);
    C2D_DrawSpriteTinted(&sprite, &imageTint);
}

bool FileExists(const char* fileName)
{
    if (spriteIndex(fileName) >= 0) {
        return g.sprites != nullptr;
    }
    FILE* file = std::fopen(fileName, "rb");
    if (!file) {
        return false;
    }
    std::fclose(file);
    return true;
}

const char* GetApplicationDirectory()
{
    return "romfs:/";
}

const char* TextFormat(const char* text, ...)
{
    static std::array<std::array<char, 256>, 4> buffers{};
    static size_t nextBuffer = 0;
    auto& buffer = buffers[nextBuffer++ % buffers.size()];
    va_list args;
    va_start(args, text);
    std::vsnprintf(buffer.data(), buffer.size(), text, args);
    va_end(args);
    return buffer.data();
}

unsigned int RidgeDashPlatformTimeMs()
{
    return static_cast<unsigned int>(osGetTime());
}

bool RidgeDashEnable3dslinkStdio()
{
    return link3dsStdio() >= 0;
}

#endif // RIDGEDASH_3DS
