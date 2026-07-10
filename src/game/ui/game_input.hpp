/**
 * @file game_input.hpp
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

class GameInput {
public:
    struct Drive {
        bool throttle = false;
        bool brake = false;
    };

    struct Commands {
        bool pause = false;
        bool reset = false;
        bool confirm = false;
        bool squid = false;
    };

    struct Menu {
        bool back = false;
        bool confirm = false;
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
    };

    void poll();

    const Drive& drive() const;
    const Commands& commands() const;
    const Menu& menu() const;

private:
    Drive _drive{};
    Commands _commands{};
    Menu _menu{};
    int _squidPressCount = 0;
};

} // namespace ridge_dash
