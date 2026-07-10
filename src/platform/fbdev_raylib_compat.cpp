/**
 * @file fbdev_raylib_compat.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#if defined(RIDGEDASH_USE_FBDEV)

#include "platform/raylib_compat.hpp"

constexpr int kGameKeyNull = KEY_NULL;
constexpr int kGameKeyEscape = KEY_ESCAPE;
constexpr int kGameKeyEnter = KEY_ENTER;
constexpr int kGameKeyLeft = KEY_LEFT;
constexpr int kGameKeyRight = KEY_RIGHT;
constexpr int kGameKeyDown = KEY_DOWN;
constexpr int kGameKeyUp = KEY_UP;
constexpr int kGameKeySpace = KEY_SPACE;
constexpr int kGameKeyA = KEY_A;
constexpr int kGameKeyC = KEY_C;
constexpr int kGameKeyD = KEY_D;
constexpr int kGameKeyF = KEY_F;
constexpr int kGameKeyG = KEY_G;
constexpr int kGameKeyR = KEY_R;
constexpr int kGameKeyS = KEY_S;
constexpr int kGameKeyW = KEY_W;
constexpr int kGameKeyX = KEY_X;
constexpr int kGameKeyZ = KEY_Z;
constexpr int kGameKeyFive = KEY_FIVE;
constexpr int kGameKeySeven = KEY_SEVEN;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <linux/fb.h>
#include <linux/input.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace {

struct TextureData {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;
};

struct FbState {
    int fbFd = -1;
    fb_var_screeninfo vinfo{};
    fb_fix_screeninfo finfo{};
    uint8_t* fb = nullptr;
    size_t fbSize = 0;
    int width = 0;
    int height = 0;
    std::vector<Color> canvas;

    std::vector<int> inputFds;
    std::vector<std::pair<dev_t, ino_t>> inputNodes;
    std::array<bool, 512> keys{};
    std::array<bool, 512> previousKeys{};
    std::array<bool, 512> pressedKeys{};

    std::unordered_map<unsigned int, TextureData> textures;
    unsigned int nextTextureId = 1;
    int targetFps = 60;
    bool closeRequested = false;
    bool windowReady = false;
    std::chrono::steady_clock::time_point lastFrame = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point frameStart = std::chrono::steady_clock::now();
    float frameTime = 1.0f / 60.0f;
    std::string appDir;
    bool inputDebug = false;
};

FbState g;

int mapLinuxKey(unsigned short code)
{
    switch (code) {
        case KEY_ESC:
            return kGameKeyEscape;
        case KEY_ENTER:
        case KEY_KPENTER:
            return kGameKeyEnter;
        case KEY_LEFT:
            return kGameKeyLeft;
        case KEY_RIGHT:
            return kGameKeyRight;
        case KEY_DOWN:
            return kGameKeyDown;
        case KEY_UP:
            return kGameKeyUp;
        case KEY_SPACE:
            return kGameKeySpace;
        case KEY_A:
            return kGameKeyA;
        case KEY_C:
        case KEY_6:
        case KEY_KP6:
            return kGameKeyC;
        case KEY_D:
            return kGameKeyD;
        case KEY_F:
            return kGameKeyF;
        case KEY_G:
            return kGameKeyG;
        case KEY_R:
            return kGameKeyR;
        case KEY_S:
            return kGameKeyS;
        case KEY_W:
            return kGameKeyW;
        case KEY_X:
            return kGameKeyX;
        case KEY_Z:
        case KEY_4:
        case KEY_KP4:
            return kGameKeyZ;
        case KEY_5:
        case KEY_KP5:
            return kGameKeyFive;
        case KEY_7:
        case KEY_KP7:
            return kGameKeySeven;
        default:
            return kGameKeyNull;
    }
}

bool testBit(const unsigned long* bits, int bit)
{
    constexpr int kBitsPerWord = static_cast<int>(sizeof(unsigned long) * 8);
    return (bits[bit / kBitsPerWord] & (1UL << (bit % kBitsPerWord))) != 0;
}

bool looksLikeKeyboard(int fd)
{
    unsigned long bits[(KEY_MAX + static_cast<int>(sizeof(unsigned long) * 8)) / (sizeof(unsigned long) * 8)]{};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) < 0) {
        return false;
    }
    return testBit(bits, KEY_ESC) || testBit(bits, KEY_ENTER) || testBit(bits, KEY_LEFT) || testBit(bits, KEY_RIGHT) ||
           testBit(bits, KEY_DOWN) || testBit(bits, KEY_UP) || testBit(bits, KEY_SPACE) || testBit(bits, KEY_A) ||
           testBit(bits, KEY_D) || testBit(bits, KEY_F) || testBit(bits, KEY_G) || testBit(bits, KEY_S) ||
           testBit(bits, KEY_W) || testBit(bits, KEY_X) || testBit(bits, KEY_Z) || testBit(bits, KEY_C) ||
           testBit(bits, KEY_4) || testBit(bits, KEY_5) || testBit(bits, KEY_6) || testBit(bits, KEY_7);
}

bool envEnabled(const char* name, bool fallback)
{
    const char* value = std::getenv(name);
    if (!value || value[0] == '\0') {
        return fallback;
    }
    return std::strcmp(value, "0") != 0 && std::strcmp(value, "false") != 0 && std::strcmp(value, "False") != 0 &&
           std::strcmp(value, "off") != 0 && std::strcmp(value, "OFF") != 0;
}

bool openInputDevice(const std::string& path, bool requireKeyboard)
{
    struct stat st {};
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    for (const auto& node : g.inputNodes) {
        if (node.first == st.st_dev && node.second == st.st_ino) {
            return false;
        }
    }

    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (g.inputDebug) {
            std::fprintf(stderr, "RidgeDash input: failed to open %s: %s\n", path.c_str(), std::strerror(errno));
        }
        return false;
    }

    if (requireKeyboard && !looksLikeKeyboard(fd)) {
        close(fd);
        return false;
    }

    if (envEnabled("RIDGEDASH_INPUT_GRAB", false) && ioctl(fd, EVIOCGRAB, 1) < 0 && g.inputDebug) {
        std::fprintf(stderr, "RidgeDash input: failed to grab %s: %s\n", path.c_str(), std::strerror(errno));
    }

    g.inputFds.push_back(fd);
    g.inputNodes.push_back({st.st_dev, st.st_ino});
    if (g.inputDebug) {
        std::fprintf(stderr, "RidgeDash input: opened %s\n", path.c_str());
    }
    return true;
}

void openInputs()
{
    if (const char* devicePath = std::getenv("RIDGEDASH_KEYBOARD_DEVICE")) {
        openInputDevice(devicePath, false);
        return;
    }
    if (const char* devicePath = std::getenv("APPLAUNCH_LINUX_KEYBOARD_DEVICE")) {
        openInputDevice(devicePath, false);
        return;
    }
    if (const char* devicePath = std::getenv("LV_LINUX_KEYBOARD_DEVICE")) {
        openInputDevice(devicePath, false);
        return;
    }

    if (openInputDevice("/dev/input/by-path/platform-3f804000.i2c-event", false)) {
        return;
    }

    bool opened = false;
    for (int i = 0; i < 32; ++i) {
        opened = openInputDevice("/dev/input/event" + std::to_string(i), true) || opened;
    }

    if (!opened) {
        std::fprintf(stderr, "RidgeDash input: no keyboard input device opened\n");
    }
}

const char* keyName(int key)
{
    switch (key) {
        case kGameKeyEscape:
            return "ESC";
        case kGameKeyEnter:
            return "ENTER";
        case kGameKeyLeft:
            return "LEFT";
        case kGameKeyRight:
            return "RIGHT";
        case kGameKeyDown:
            return "DOWN";
        case kGameKeyUp:
            return "UP";
        case kGameKeySpace:
            return "SPACE";
        case kGameKeyA:
            return "A";
        case kGameKeyC:
            return "C/RIGHT_ALT";
        case kGameKeyD:
            return "D";
        case kGameKeyF:
            return "F/UP_ALT";
        case kGameKeyG:
            return "G";
        case kGameKeyR:
            return "R";
        case kGameKeyS:
            return "S";
        case kGameKeyW:
            return "W";
        case kGameKeyX:
            return "X/DOWN_ALT";
        case kGameKeyZ:
            return "Z/LEFT_ALT";
        case kGameKeyFive:
            return "5";
        case kGameKeySeven:
            return "7";
        default:
            return "UNKNOWN";
    }
}

void pollInput()
{
    g.previousKeys = g.keys;

    for (int fd : g.inputFds) {
        input_event ev{};
        while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type != EV_KEY) {
                continue;
            }
            int key = mapLinuxKey(ev.code);
            if (g.inputDebug) {
                std::fprintf(stderr, "RidgeDash input: code=%u value=%d mapped=%s\n", ev.code, ev.value, keyName(key));
            }
            if (key == kGameKeyNull || key < 0 || key >= static_cast<int>(g.keys.size())) {
                continue;
            }
            g.keys[static_cast<size_t>(key)] = ev.value != 0;
        }
    }

    for (size_t i = 0; i < g.keys.size(); ++i) {
        g.pressedKeys[i] = g.keys[i] && !g.previousKeys[i];
    }
}

uint32_t packColor(Color color)
{
    const uint32_t r = color.r >> (8 - g.vinfo.red.length);
    const uint32_t green = color.g >> (8 - g.vinfo.green.length);
    const uint32_t b = color.b >> (8 - g.vinfo.blue.length);
    return (r << g.vinfo.red.offset) | (green << g.vinfo.green.offset) | (b << g.vinfo.blue.offset);
}

Color blend(Color src, Color dst)
{
    if (src.a == 255) {
        return src;
    }
    if (src.a == 0) {
        return dst;
    }
    const int a = src.a;
    return Color{static_cast<unsigned char>((src.r * a + dst.r * (255 - a)) / 255),
                 static_cast<unsigned char>((src.g * a + dst.g * (255 - a)) / 255),
                 static_cast<unsigned char>((src.b * a + dst.b * (255 - a)) / 255),
                 255};
}

void putPixel(int x, int y, Color color)
{
    if (x < 0 || y < 0 || x >= g.width || y >= g.height || color.a == 0) {
        return;
    }
    Color& dst = g.canvas[static_cast<size_t>(y * g.width + x)];
    dst = blend(color, dst);
}

void flushFramebuffer()
{
    if (!g.fb) {
        return;
    }

    const int copyWidth = std::min(g.width, static_cast<int>(g.vinfo.xres));
    const int copyHeight = std::min(g.height, static_cast<int>(g.vinfo.yres));
    for (int y = 0; y < copyHeight; ++y) {
        uint8_t* row = g.fb + static_cast<size_t>(y + g.vinfo.yoffset) * g.finfo.line_length;
        for (int x = 0; x < copyWidth; ++x) {
            const Color color = g.canvas[static_cast<size_t>(y * g.width + x)];
            const uint32_t packed = packColor(color);
            uint8_t* px = row + static_cast<size_t>(x + g.vinfo.xoffset) * g.vinfo.bits_per_pixel / 8;
            if (g.vinfo.bits_per_pixel == 16) {
                *reinterpret_cast<uint16_t*>(px) = static_cast<uint16_t>(packed);
            } else if (g.vinfo.bits_per_pixel == 32) {
                *reinterpret_cast<uint32_t*>(px) = packed;
            }
        }
    }
}

void drawLine(int x0, int y0, int x1, int y1, Color color)
{
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        putPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

uint32_t glyphBits(char c)
{
    switch (c) {
        case '0':
            return 0x1D3B7;
        case '1':
            return 0x0C925;
        case '2':
            return 0x1C8E7;
        case '3':
            return 0x1C8F1;
        case '4':
            return 0x12F92;
        case '5':
            return 0x1F0F1;
        case '6':
            return 0x1F3F7;
        case '7':
            return 0x1C892;
        case '8':
            return 0x1F3F7;
        case '9':
            return 0x1F2F1;
        case 'A':
            return 0x0EFA5;
        case 'B':
            return 0x1EBAF;
        case 'D':
            return 0x1D6B7;
        case 'E':
            return 0x1F0E7;
        case 'F':
            return 0x1F0E4;
        case 'G':
            return 0x1F1B7;
        case 'H':
            return 0x12FA5;
        case 'I':
            return 0x1C927;
        case 'L':
            return 0x10847;
        case 'M':
            return 0x15AAD;
        case 'N':
            return 0x15AB5;
        case 'O':
            return 0x0EBAE;
        case 'R':
            return 0x1EBA5;
        case 'S':
            return 0x1F0F1;
        case 'T':
            return 0x1C921;
        case 'V':
            return 0x12AA2;
        case 'Y':
            return 0x12E21;
        case '/':
            return 0x04440;
        case '-':
            return 0x000E0;
        default:
            return 0;
    }
}

} // namespace

void SetConfigFlags(unsigned int) {}

void InitWindow(int width, int height, const char*)
{
    g.width = width;
    g.height = height;
    g.canvas.assign(static_cast<size_t>(width * height), BLACK);
    g.inputDebug = envEnabled("RIDGEDASH_INPUT_DEBUG", false);

    char exePath[4096]{};
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len > 0) {
        exePath[len] = '\0';
        g.appDir = std::filesystem::path(exePath).parent_path().string() + "/";
    } else {
        g.appDir = "./";
    }

    const char* fbPath = std::getenv("RIDGEDASH_FBDEV");
    g.fbFd = open(fbPath ? fbPath : "/dev/fb0", O_RDWR | O_CLOEXEC);
    if (g.fbFd < 0 || ioctl(g.fbFd, FBIOGET_FSCREENINFO, &g.finfo) < 0 ||
        ioctl(g.fbFd, FBIOGET_VSCREENINFO, &g.vinfo) < 0) {
        std::fprintf(stderr, "RidgeDash FBDEV: failed to open framebuffer\n");
        g.windowReady = false;
        g.closeRequested = true;
        return;
    }

    g.fbSize = static_cast<size_t>(g.finfo.line_length) * g.vinfo.yres_virtual;
    g.fb = static_cast<uint8_t*>(mmap(nullptr, g.fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, g.fbFd, 0));
    if (g.fb == MAP_FAILED) {
        std::fprintf(stderr, "RidgeDash FBDEV: failed to mmap framebuffer\n");
        g.fb = nullptr;
        g.windowReady = false;
        g.closeRequested = true;
        return;
    }

    g.windowReady = true;
    openInputs();
    g.lastFrame = std::chrono::steady_clock::now();
    g.frameStart = g.lastFrame;
}

void SetTargetFPS(int fps)
{
    g.targetFps = std::max(1, fps);
}

bool IsWindowReady()
{
    return g.windowReady;
}

bool WindowShouldClose()
{
    pollInput();
    return g.closeRequested;
}

float GetFrameTime()
{
    const auto now = std::chrono::steady_clock::now();
    g.frameTime = std::chrono::duration<float>(now - g.lastFrame).count();
    g.lastFrame = now;
    return g.frameTime;
}

void BeginDrawing()
{
    g.frameStart = std::chrono::steady_clock::now();
}

void EndDrawing()
{
    flushFramebuffer();
    const auto target = std::chrono::duration<float>(1.0f / static_cast<float>(g.targetFps));
    const auto elapsed = std::chrono::steady_clock::now() - g.frameStart;
    if (elapsed < target) {
        std::this_thread::sleep_for(target - elapsed);
    }
}

void CloseWindow()
{
    g.textures.clear();
    for (int fd : g.inputFds) {
        close(fd);
    }
    g.inputFds.clear();
    if (g.fb) {
        munmap(g.fb, g.fbSize);
        g.fb = nullptr;
    }
    if (g.fbFd >= 0) {
        close(g.fbFd);
        g.fbFd = -1;
    }
}

bool IsKeyDown(int key)
{
    return key >= 0 && key < static_cast<int>(g.keys.size()) && g.keys[static_cast<size_t>(key)];
}

bool IsKeyPressed(int key)
{
    return key >= 0 && key < static_cast<int>(g.pressedKeys.size()) && g.pressedKeys[static_cast<size_t>(key)];
}

void ClearBackground(Color color)
{
    std::fill(g.canvas.begin(), g.canvas.end(), color);
}

void DrawRectangle(int posX, int posY, int width, int height, Color color)
{
    for (int y = std::max(0, posY); y < std::min(g.height, posY + height); ++y) {
        for (int x = std::max(0, posX); x < std::min(g.width, posX + width); ++x) {
            putPixel(x, y, color);
        }
    }
}

void DrawRectangleLines(int posX, int posY, int width, int height, Color color)
{
    drawLine(posX, posY, posX + width - 1, posY, color);
    drawLine(posX, posY + height - 1, posX + width - 1, posY + height - 1, color);
    drawLine(posX, posY, posX, posY + height - 1, color);
    drawLine(posX + width - 1, posY, posX + width - 1, posY + height - 1, color);
}

void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color)
{
    Texture2D blank{0, 1, 1, 1, 0};
    TextureData data;
    data.width = 1;
    data.height = 1;
    data.pixels = {color};
    g.textures[0] = data;
    DrawTexturePro(blank, Rectangle{0, 0, 1, 1}, rec, origin, rotation, WHITE);
}

void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color)
{
    const int radius = std::max(0, static_cast<int>(std::round(thick * 0.5f)) - 1);
    const int x0 = static_cast<int>(std::round(startPos.x));
    const int y0 = static_cast<int>(std::round(startPos.y));
    const int x1 = static_cast<int>(std::round(endPos.x));
    const int y1 = static_cast<int>(std::round(endPos.y));
    for (int oy = -radius; oy <= radius; ++oy) {
        for (int ox = -radius; ox <= radius; ++ox) {
            drawLine(x0 + ox, y0 + oy, x1 + ox, y1 + oy, color);
        }
    }
}

void DrawText(const char* text, int posX, int posY, int fontSize, Color color)
{
    const int scale = std::max(1, fontSize / 6);
    int cursor = posX;
    for (const char* p = text; *p; ++p) {
        if (*p == ' ') {
            cursor += 4 * scale;
            continue;
        }
        const uint32_t bits = glyphBits(*p >= 'a' && *p <= 'z' ? static_cast<char>(*p - 32) : *p);
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 3; ++x) {
                if ((bits & (1u << (14 - (y * 3 + x)))) == 0) {
                    continue;
                }
                DrawRectangle(cursor + x * scale, posY + y * scale, scale, scale, color);
            }
        }
        cursor += 4 * scale;
    }
}

Texture2D LoadTexture(const char* fileName)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(fileName, &width, &height, &channels, 4);
    if (!data) {
        return {};
    }

    TextureData texture;
    texture.width = width;
    texture.height = height;
    texture.pixels.resize(static_cast<size_t>(width * height));
    for (int i = 0; i < width * height; ++i) {
        texture.pixels[static_cast<size_t>(i)] = Color{data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]};
    }
    stbi_image_free(data);

    const unsigned int id = g.nextTextureId++;
    g.textures[id] = std::move(texture);
    return Texture2D{id, width, height, 1, 0};
}

void UnloadTexture(Texture2D texture)
{
    g.textures.erase(texture.id);
}

void SetTextureFilter(Texture2D, int) {}

void SetTextureWrap(Texture2D, int) {}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint)
{
    auto it = g.textures.find(texture.id);
    if (it == g.textures.end()) {
        return;
    }

    const TextureData& tex = it->second;
    const float radians = rotation / RAD2DEG;
    const float cs = std::cos(radians);
    const float sn = std::sin(radians);
    const int minX = static_cast<int>(std::floor(dest.x - origin.x - std::abs(dest.width) - 2.0f));
    const int maxX = static_cast<int>(std::ceil(dest.x - origin.x + std::abs(dest.width) + 2.0f));
    const int minY = static_cast<int>(std::floor(dest.y - origin.y - std::abs(dest.height) - 2.0f));
    const int maxY = static_cast<int>(std::ceil(dest.y - origin.y + std::abs(dest.height) + 2.0f));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float wx = static_cast<float>(x) - dest.x;
            const float wy = static_cast<float>(y) - dest.y;
            const float lx = wx * cs + wy * sn + origin.x;
            const float ly = -wx * sn + wy * cs + origin.y;
            if (lx < 0.0f || ly < 0.0f || lx >= dest.width || ly >= dest.height) {
                continue;
            }

            const int sx = static_cast<int>(source.x + lx / dest.width * source.width);
            const int sy = static_cast<int>(source.y + ly / dest.height * source.height);
            if (sx < 0 || sy < 0 || sx >= tex.width || sy >= tex.height) {
                continue;
            }

            Color color = tex.pixels[static_cast<size_t>(sy * tex.width + sx)];
            color.r = static_cast<unsigned char>(color.r * tint.r / 255);
            color.g = static_cast<unsigned char>(color.g * tint.g / 255);
            color.b = static_cast<unsigned char>(color.b * tint.b / 255);
            color.a = static_cast<unsigned char>(color.a * tint.a / 255);
            putPixel(x, y, color);
        }
    }
}

bool FileExists(const char* fileName)
{
    return std::filesystem::exists(fileName);
}

const char* GetApplicationDirectory()
{
    return g.appDir.c_str();
}

const char* TextFormat(const char* text, ...)
{
    static thread_local char buffer[256];
    va_list args;
    va_start(args, text);
    std::vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    return buffer;
}

#endif
