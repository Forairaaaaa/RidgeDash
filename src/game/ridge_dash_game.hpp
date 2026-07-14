/**
 * @file ridge_dash_game.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "game/world/environment.hpp"
#include "game/ui/game_input.hpp"
#include "game/ui/game_ui.hpp"
#include "game/ui/pause_menu_controller.hpp"
#include "game/audio/audio_system.hpp"
#include "game/pickups/pickups.hpp"
#include "game/run/run_controller.hpp"
#include "game/run/run_records.hpp"
#include "game/run/run_stats.hpp"
#include "game/world/terrain.hpp"
#include "game/run/trick_tracker.hpp"
#include "game/vehicle/vehicle.hpp"
#include "platform/raylib_compat.hpp"
#include <box2d/box2d.h>
#include <cstdint>
#include <random>

namespace ridge_dash {

class RidgeDashGame {
public:
    using DisplayScaleOption = ridge_dash::DisplayScaleOption;

    RidgeDashGame();
    ~RidgeDashGame();

    RidgeDashGame(const RidgeDashGame&) = delete;
    RidgeDashGame& operator=(const RidgeDashGame&) = delete;

    void reset();
    void update(float dt);
    void draw() const;
    bool shouldQuit() const;
    void setDisplayScaleOption(DisplayScaleOption option);
    DisplayScaleOption displayScaleOption() const;
    bool consumeDisplayScaleRequest(DisplayScaleOption& option);
    void setCrtEnabled(bool enabled);
    bool crtEnabled() const;
    bool consumeCrtRequest(bool& enabled);
    void setInterpolationEnabled(bool enabled);
    bool renderInterpolation() const;
    float renderAlpha() const;
    void playSfx(AudioSystem::Sfx id);

private:
    friend class PickupSystem;
    friend class PickupEffects;
    friend class FuelPickups;
    friend class CoinPickups;
    friend class FleaPickups;
    friend class RocketPickups;
    friend class CactusPickups;
    friend class SnowmanPickups;
    friend class GiantFleaPickups;
    friend class HelmetPickups;
    friend class SquidPickups;

    struct SpriteAssets {
        Texture2D carBodyDay{};
        Texture2D carBodyNight{};
        Texture2D driver{};
        Texture2D driverBlink{};
        Texture2D driverCried{};
        Texture2D driverScared{};
        Texture2D driverShocked{};
        Texture2D wheelDay{};
        Texture2D wheelNight{};
        Texture2D fuelCan{};
        Texture2D coin{};
        Texture2D flea{};
        Texture2D rocket{};
        Texture2D cactus{};
        Texture2D snowman{};
        Texture2D giantFlea{};
        Texture2D helmet{};
        Texture2D driverHelmeted{};
        Texture2D squidA{};
        Texture2D squidB{};
        Texture2D squidC{};
        Texture2D squidD{};
    };

    void loadSprites();
    void unloadSprites();
    void destroyWorld();
    void createWorld();
    void createVehicle();
    void updateWorldStreaming();
    void processInput(float dt);
    void stepPhysics(float dt);
    void handleSensorEvents();
    void updateGroundState();
    void updateTricks(float dt);
    void updateRunState(float dt);
    void updateCamera(float dt);
    void updateEnvironmentBiome(float dt);
    void updateDriverExpression(float dt);
    void updateEngineAudio(float dt);
    void updateBgmAudio(float dt);
    void updatePauseMenu(float dt);
    void enterPauseMenu();
    void exitPauseMenu();
    void requestGameExit();
    void submitRunRecord();

    void updatePickupOverlaps();
    void showScorePopup(int amount, const char* label);
    void updateDust(float dt);
    Vector2 worldToScreen(b2Vec2 p) const;
    void drawDust() const;
    void drawVehicle() const;
    bool carValid() const;
    float carDistance() const;
    bool helmetActive() const;

    b2WorldId _worldId = b2_nullWorldId;
    Vehicle _vehicle;
    TerrainSystem _terrain;
    PickupSystem _pickups;
    SpriteAssets _sprites;
    Environment _environment;
    GameUi _ui;
    GameInput _input;
    PauseMenuController _pauseMenu;
    AudioSystem _audio;
    std::mt19937 _rng;
    RunRecords _runRecords;
    RunController _runController;
    Vector2 _camera{};
    float _physicsRemainder = 0.0f;
    bool _interpolate = false;
    RunStats _runStats;
    TrickTracker _trickTracker;
    float _startX = 0.0f;
    uint32_t _runSeed = 0;
    bool _bgmIntense = false;             // current calm/intense BGM state (hysteresis)
    float _bgmCalmTimer = 0.0f;           // dwell time spent slow before switching back to calm
    bool _helmetActive = false;           // helmet revive protection active
    bool _helmetRescuedThisFrame = false; // prevent multi-segment head hits
    float _invincibleTimer = 0.0f;        // post-rescue invincibility (seconds)
    bool _quitRequested = false;
};

} // namespace ridge_dash
