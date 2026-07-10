/**
 * @file evdev_input_compat.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#if defined(RIDGEDASH_USE_EVDEV_INPUT) && !defined(RIDGEDASH_USE_FBDEV)

#define RIDGEDASH_RAYLIB_COMPAT_NO_KEY_MACROS
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

#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

struct InputState {
    std::vector<int> fds;
    std::vector<std::pair<dev_t, ino_t>> nodes;
    std::array<bool, 512> keys{};
    std::array<bool, 512> previousKeys{};
    std::array<bool, 512> pressedKeys{};
    bool closeRequested = false;
    bool debug = false;
};

InputState gInput;

void requestCloseSignal(int)
{
    gInput.closeRequested = true;
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

bool openInputDevice(const std::string& path, bool requireKeyboard)
{
    struct stat st {};
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    for (const auto& node : gInput.nodes) {
        if (node.first == st.st_dev && node.second == st.st_ino) {
            return false;
        }
    }

    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (gInput.debug) {
            std::fprintf(stderr, "RidgeDash input: failed to open %s: %s\n", path.c_str(), std::strerror(errno));
        }
        return false;
    }

    if (requireKeyboard && !looksLikeKeyboard(fd)) {
        close(fd);
        return false;
    }

    if (envEnabled("RIDGEDASH_INPUT_GRAB", false) && ioctl(fd, EVIOCGRAB, 1) < 0 && gInput.debug) {
        std::fprintf(stderr, "RidgeDash input: failed to grab %s: %s\n", path.c_str(), std::strerror(errno));
    }

    gInput.fds.push_back(fd);
    gInput.nodes.push_back({st.st_dev, st.st_ino});
    if (gInput.debug) {
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

void enableTerminalSignals()
{
    termios settings{};
    if (tcgetattr(STDIN_FILENO, &settings) != 0) {
        return;
    }
    settings.c_lflag |= ISIG;
    tcsetattr(STDIN_FILENO, TCSANOW, &settings);
}

void pollInput()
{
    gInput.previousKeys = gInput.keys;

    for (int fd : gInput.fds) {
        input_event event{};
        while (read(fd, &event, sizeof(event)) == sizeof(event)) {
            if (event.type != EV_KEY) {
                continue;
            }
            const int key = mapLinuxKey(event.code);
            if (gInput.debug) {
                std::fprintf(
                    stderr, "RidgeDash input: code=%u value=%d mapped=%s\n", event.code, event.value, keyName(key));
            }
            if (key == kGameKeyNull || key < 0 || key >= static_cast<int>(gInput.keys.size())) {
                continue;
            }
            gInput.keys[static_cast<size_t>(key)] = event.value != 0;
        }
    }

    for (size_t i = 0; i < gInput.keys.size(); ++i) {
        gInput.pressedKeys[i] = gInput.keys[i] && !gInput.previousKeys[i];
    }
}

} // namespace

void RidgeDashInputInit()
{
    gInput.debug = envEnabled("RIDGEDASH_INPUT_DEBUG", false);
    std::signal(SIGINT, requestCloseSignal);
    std::signal(SIGTERM, requestCloseSignal);
    enableTerminalSignals();
    openInputs();
}

void RidgeDashInputShutdown()
{
    for (int fd : gInput.fds) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
    }
    gInput.fds.clear();
    gInput.nodes.clear();
}

bool RidgeDashWindowShouldClose()
{
    pollInput();
    return gInput.closeRequested || WindowShouldClose();
}

bool RidgeDashIsKeyDown(int key)
{
    return key >= 0 && key < static_cast<int>(gInput.keys.size()) && gInput.keys[static_cast<size_t>(key)];
}

bool RidgeDashIsKeyPressed(int key)
{
    return key >= 0 && key < static_cast<int>(gInput.pressedKeys.size()) &&
           gInput.pressedKeys[static_cast<size_t>(key)];
}

#endif
