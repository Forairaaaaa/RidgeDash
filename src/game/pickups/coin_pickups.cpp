/**
 * @file coin_pickups.cpp
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
#include "game/render_helpers.hpp"

#include <cmath>
#include <random>

namespace ridge_dash {
namespace {

bool nearPoint(Vector2 a, Vector2 b, float distance)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= distance * distance;
}

} // namespace

using namespace game_config;

void CoinPickups::clear()
{
    _items.clear();
    _nextX = 0.0f;
}

void CoinPickups::reset(RidgeDashGame& game)
{
    _items.clear();
    _items.reserve(32);

    std::uniform_real_distribution<float> gapDist(18.0f, 36.0f);
    _nextX = 18.0f + gapDist(game._rng);
    stream(game, game._startX + kCoinGenerateAhead);
}

void CoinPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(18.0f, 36.0f);
    std::uniform_int_distribution<int> countDist(1, 3);
    while (_nextX < targetX) {
        const TerrainSample clusterTerrain = game._terrain.sampleAt(_nextX, 12.0f, game._rng);

        const int count = countDist(game._rng);
        const float clusterStart = _nextX - 1.2f;
        const float clusterEnd = _nextX + static_cast<float>(count - 1) * 1.08f + 1.2f;
        if (game._pickups.fuel().activeInRange(clusterStart - 3.4f, clusterEnd + 3.4f)) {
            _nextX += 5.4f;
            continue;
        }
        if (game._pickups.flea().activeInRange(clusterStart - 3.2f, clusterEnd + 3.2f)) {
            _nextX += 4.9f;
            continue;
        }
        if (game._pickups.rocket().activeNear(_nextX, 5.8f + static_cast<float>(count) * 0.7f)) {
            _nextX += 6.4f;
            continue;
        }
        if (game._pickups.cactus().activeNear(_nextX, 4.8f + static_cast<float>(count) * 0.7f)) {
            _nextX += 5.8f;
            continue;
        }
        if (game._pickups.snowman().activeNear(_nextX, 4.8f + static_cast<float>(count) * 0.7f)) {
            _nextX += 5.8f;
            continue;
        }

        const float center = static_cast<float>(count - 1) * 0.5f;
        for (int i = 0; i < count; ++i) {
            const float x = _nextX + static_cast<float>(i) * 1.08f;
            const TerrainSample terrain = i == 0 ? clusterTerrain : game._terrain.sampleAt(x, 1.2f, game._rng);
            const float lift = count == 1 ? 0.0f : (1.0f - std::abs(static_cast<float>(i) - center) / center) * 0.18f;
            create(game, terrain, -1.02f - lift);
        }
        _nextX += gapDist(game._rng);
    }
}

void CoinPickups::create(RidgeDashGame& game, const TerrainSample& terrain, float yOffset)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    _items.push_back(Item{});
    Item& coin = _items.back();
    coin.pos = {terrain.x, terrain.y + yOffset};

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {coin.pos.x, coin.pos.y};
    coin.bodyId = b2CreateBody(game._worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    b2Circle circle = {{0.0f, 0.0f}, kCoinRadius};
    coin.shapeId = b2CreateCircleShape(coin.bodyId, &shapeDef, &circle);
}

void CoinPickups::trim(float minX)
{
    auto it = _items.begin();
    while (it != _items.end()) {
        if (it->active && it->pos.x >= minX) {
            ++it;
            continue;
        }
        if (b2Body_IsValid(it->bodyId)) {
            b2DestroyBody(it->bodyId);
        }
        it = _items.erase(it);
    }
}

bool CoinPickups::collect(RidgeDashGame& game, Item& coin)
{
    if (!coin.active) {
        return false;
    }

    game._pickups.effects().spawn(coin.pos, PickupEffects::Kind::Coin, game._runSeed);
    coin.active = false;
    game._runStats.addCoin(kCoinScore);
    game.showScorePopup(static_cast<int>(kCoinScore), "");
    if (b2Body_IsValid(coin.bodyId)) {
        b2DestroyBody(coin.bodyId);
    }
    coin.bodyId = b2_nullBodyId;
    coin.shapeId = b2_nullShapeId;
    return true;
}

bool CoinPickups::collectByShape(RidgeDashGame& game, b2ShapeId pickupShape, b2ShapeId otherShape)
{
    if (!game._vehicle.shapeBelongsToVehicle(otherShape)) {
        return false;
    }
    for (Item& coin : _items) {
        if (coin.active && b2Shape_IsValid(coin.shapeId) && B2_ID_EQUALS(coin.shapeId, pickupShape)) {
            return collect(game, coin);
        }
    }
    return false;
}

bool CoinPickups::collectOverlaps(RidgeDashGame& game, const Vector2* points, int count, float speedBonus)
{
    bool collected = false;
    for (Item& coin : _items) {
        if (!coin.active) {
            continue;
        }
        for (int i = 0; i < count; ++i) {
            if (nearPoint(points[i], coin.pos, kCoinPickupDistance + speedBonus)) {
                collected = collect(game, coin) || collected;
                break;
            }
        }
    }
    return collected;
}

bool CoinPickups::activeNear(float x, float distance) const
{
    for (const Item& coin : _items) {
        if (coin.active && std::abs(coin.pos.x - x) <= distance) {
            return true;
        }
    }
    return false;
}

void CoinPickups::draw(const RidgeDashGame& game) const
{
    for (const Item& coin : _items) {
        if (!coin.active) {
            continue;
        }
        const Vector2 p = game.worldToScreen({coin.pos.x, coin.pos.y});
        if (p.x < -16.0f || p.x > kScreenWidth + 16.0f) {
            continue;
        }
        const int ix = static_cast<int>(std::round(p.x));
        const int iy = static_cast<int>(std::round(p.y));
        if (textureLoaded(game._sprites.coin)) {
            drawSpriteCentered(
                game._sprites.coin, {static_cast<float>(ix), static_cast<float>(iy)}, 10.0f, 10.0f, 0.0f);
        } else {
            DrawRectangle(ix - 4, iy - 4, 8, 8, Color{247, 204, 72, 255});
            DrawRectangle(ix - 2, iy - 2, 4, 4, Color{255, 236, 130, 255});
            DrawRectangleLines(ix - 4, iy - 4, 8, 8, Color{156, 104, 36, 255});
        }
    }
}

} // namespace ridge_dash
