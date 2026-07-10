/**
 * @file display_scale.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-10
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace ridge_dash {

enum class DisplayScaleOption { Scale1, Scale2, Scale3, Scale4, Fullscreen };

DisplayScaleOption nextDisplayScaleOption(DisplayScaleOption option, int direction);
const char* displayScaleLabel(DisplayScaleOption option);

} // namespace ridge_dash
