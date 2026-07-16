/**
 * @file fuel_pickups.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2026-07-11
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/pickups/pickups.hpp"

#include "game/game_config.hpp"
#include "game/ridge_dash_game.hpp"
#include "game/pickups/pickup_base_impl.hpp"
#include "game/render_helpers.hpp"

#include <cmath>
#include <random>

namespace ridge_dash {

using namespace game_config;

void FuelPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(16);

    std::uniform_real_distribution<float> gapDist(40.0f, 72.0f);
    _nextX = 28.0f + gapDist(game._rng) * 0.45f;
    stream(game, game._startX + kFuelGenerateAhead);
}

void FuelPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(40.0f, 72.0f);
    while (_nextX < targetX) {
        const TerrainSample terrain = game._terrain.sampleAt(_nextX, 8.0f, game._rng);
        doCreate(game, terrain);
        _nextX += gapDist(game._rng);
    }
}

void FuelPickups::doCreate(RidgeDashGame& game, const TerrainSample& terrain)
{
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * 3.14159265358979323846f);

    _items.push_back(Item{});
    Item& fuel = _items.back();
    fuel.basePos = {terrain.x, terrain.y - 1.25f};
    fuel.pos = fuel.basePos;
    fuel.idlePhase = phaseDist(game._rng);

    createSensorBody(game, fuel.pos, kFuelCanRadius, fuel.bodyId, fuel.shapeId);
}

void FuelPickups::update(float dt)
{
    for (Item& fuel : _items) {
        if (!fuel.active) {
            continue;
        }
        fuel.idleTime += dt;
        fuel.pos = {fuel.basePos.x, fuel.basePos.y + std::sin(fuel.idleTime * 3.4f + fuel.idlePhase) * 0.14f};
        if (b2Body_IsValid(fuel.bodyId)) {
            b2Body_SetTransform(fuel.bodyId, {fuel.pos.x, fuel.pos.y}, b2MakeRot(0.0f));
        }
    }
}

bool FuelPickups::doCollect(RidgeDashGame& game, Item& fuel)
{
    if (!fuel.active) {
        return false;
    }

    game._pickups.effects().spawn(fuel.pos, PickupEffects::Kind::Fuel, game._runSeed);
    fuel.active = false;
    game._runController.refillFuel();
    game._ui.triggerFuelCelebration();
    game.playSfx(AudioSystem::Sfx::Fuel);
    if (b2Body_IsValid(fuel.bodyId)) {
        b2DestroyBody(fuel.bodyId);
    }
    fuel.bodyId = b2_nullBodyId;
    fuel.shapeId = b2_nullShapeId;
    return true;
}

bool FuelPickups::activeInRange(float minX, float maxX) const
{
    for (const Item& fuel : _items) {
        if (fuel.active && fuel.pos.x >= minX && fuel.pos.x <= maxX) {
            return true;
        }
    }
    return false;
}

float FuelPickups::pickupDistance() const
{
    return game_config::kFuelPickupDistance;
}

void FuelPickups::draw(const RidgeDashGame& game) const
{
    for (const Item& fuel : _items) {
        if (!fuel.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({fuel.pos.x, fuel.pos.y});
        if (p.x < -20.0f || p.x > kScreenWidth + 20.0f) {
            continue;
        }
        const int ix = static_cast<int>(std::round(p.x));
        const int iy = static_cast<int>(std::round(p.y));
        if (textureLoaded(game._sprites.fuelCan)) {
            drawSpriteCentered(
                game._sprites.fuelCan, {static_cast<float>(ix), static_cast<float>(iy)}, 15.0f, 19.0f, 0.0f);
        } else {
            DrawRectangle(ix - 5, iy - 7, 10, 14, Color{196, 48, 58, 255});
            DrawRectangle(ix - 3, iy - 10, 6, 4, Color{242, 202, 95, 255});
            DrawRectangle(ix + 2, iy - 4, 3, 7, Color{241, 100, 70, 255});
            DrawRectangleLines(ix - 5, iy - 7, 10, 14, Color{82, 32, 39, 255});
        }
    }
}

} // namespace ridge_dash
