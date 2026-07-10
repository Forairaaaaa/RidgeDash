/**
 * @file display_scale.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/ui/display_scale.hpp"

namespace ridge_dash {

DisplayScaleOption nextDisplayScaleOption(DisplayScaleOption option, int direction)
{
    int value = static_cast<int>(option) + direction;
    if (value < 0) {
        value = 4;
    } else if (value > 4) {
        value = 0;
    }
    return static_cast<DisplayScaleOption>(value);
}

const char* displayScaleLabel(DisplayScaleOption option)
{
    switch (option) {
        case DisplayScaleOption::Scale1:
            return "1X";
        case DisplayScaleOption::Scale2:
            return "2X";
        case DisplayScaleOption::Scale3:
            return "3X";
        case DisplayScaleOption::Scale4:
            return "4X";
        case DisplayScaleOption::Fullscreen:
            return "FULL";
        default:
            return "3X";
    }
}

} // namespace ridge_dash
