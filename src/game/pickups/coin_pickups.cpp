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
#include <vector>

namespace ridge_dash {
namespace {

constexpr float kPi = 3.14159265358979323846f;

bool nearPoint(Vector2 a, Vector2 b, float distance)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy <= distance * distance;
}

// Coin shape patterns. Points are built in a local frame where +x is forward and
// +y is UP; the caller anchors and flips into world space (world y is down).
enum class CoinShape {
    Line,     // horizontal / sloped row
    Wave,     // sine wave row
    Circle,   // ring
    Arch,     // upward arc (half-circle), good for jumps/rockets
    Triangle, // triangle outline
    Diamond,  // diamond outline
    Rect,     // rectangle outline
    Count
};

// Append `n` evenly spaced points along a segment (local frame, +y up).
void addSegment(std::vector<Vector2>& out, Vector2 a, Vector2 b, int n)
{
    if (n <= 1) {
        out.push_back(a);
        return;
    }
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n - 1);
        out.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
    }
}

// Number of coins to span a length at roughly one-diameter spacing.
int spanCount(float length)
{
    return std::max(2, static_cast<int>(std::round(length / game_config::kCoinSpacing)) + 1);
}

// Build a shape's local-frame points. `scale` roughly controls size; `slope` tilts
// line/wave shapes to follow the terrain.
std::vector<Vector2> buildShapePoints(CoinShape shape, float scale, float slope, std::mt19937& rng)
{
    using game_config::kCoinSpacing;
    std::vector<Vector2> pts;
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    switch (shape) {
        case CoinShape::Line: {
            const float len = 3.0f + scale * 4.0f;
            const int n = spanCount(len);
            addSegment(pts, {0.0f, 0.0f}, {len, len * slope}, n);
            break;
        }
        case CoinShape::Wave: {
            const float len = 4.0f + scale * 5.0f;
            const float amp = 0.9f + scale * 0.8f;
            const int n = spanCount(len);
            for (int i = 0; i < n; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                const float x = t * len;
                pts.push_back({x, x * slope + std::sin(t * kPi * 2.0f) * amp});
            }
            break;
        }
        case CoinShape::Circle: {
            const float r = 1.2f + scale * 1.3f;
            const int n = std::max(8, spanCount(2.0f * kPi * r));
            for (int i = 0; i < n; ++i) {
                const float a = (static_cast<float>(i) / static_cast<float>(n)) * kPi * 2.0f;
                pts.push_back({std::cos(a) * r, r + std::sin(a) * r});
            }
            break;
        }
        case CoinShape::Arch: {
            const float r = 1.6f + scale * 2.0f;
            const int n = std::max(5, spanCount(kPi * r));
            for (int i = 0; i < n; ++i) {
                const float a = kPi * (static_cast<float>(i) / static_cast<float>(n - 1)); // 0..pi
                pts.push_back({-std::cos(a) * r + r, std::sin(a) * r});
            }
            break;
        }
        case CoinShape::Triangle: {
            const float s = 2.4f + scale * 2.6f;
            const int n = spanCount(s);
            const Vector2 a{0.0f, 0.0f};
            const Vector2 b{s, 0.0f};
            const Vector2 c{s * 0.5f, s * 0.9f};
            addSegment(pts, a, b, n);
            addSegment(pts, b, c, n);
            addSegment(pts, c, a, n);
            break;
        }
        case CoinShape::Diamond: {
            const float s = 2.0f + scale * 2.2f;
            const int n = spanCount(s);
            const Vector2 top{0.0f, s};
            const Vector2 right{s, 0.0f};
            const Vector2 bot{0.0f, -s};
            const Vector2 left{-s, 0.0f};
            addSegment(pts, top, right, n);
            addSegment(pts, right, bot, n);
            addSegment(pts, bot, left, n);
            addSegment(pts, left, top, n);
            break;
        }
        case CoinShape::Rect: {
            const float w = 3.0f + scale * 3.0f;
            const float h = 1.8f + scale * 2.0f;
            const int nx = spanCount(w);
            const int ny = spanCount(h);
            addSegment(pts, {0.0f, 0.0f}, {w, 0.0f}, nx);
            addSegment(pts, {w, 0.0f}, {w, h}, ny);
            addSegment(pts, {w, h}, {0.0f, h}, nx);
            addSegment(pts, {0.0f, h}, {0.0f, 0.0f}, ny);
            break;
        }
        default:
            break;
    }

    (void)unit;
    (void)rng;
    return pts;
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
    _items.reserve(128);

    std::uniform_real_distribution<float> gapDist(kCoinClusterGapMin, kCoinClusterGapMax);
    _nextX = 16.0f + gapDist(game._rng);
    stream(game, game._startX + kCoinGenerateAhead);
}

void CoinPickups::stream(RidgeDashGame& game, float targetX)
{
    std::uniform_real_distribution<float> gapDist(kCoinClusterGapMin, kCoinClusterGapMax);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_real_distribution<float> scaleDist(0.3f, 1.0f);

    while (_nextX < targetX) {
        // Local terrain slope (world y is down, so a downhill-forward slope is
        // positive here after we flip to "up is positive").
        const float hL = game._terrain.heightAt(_nextX - kCoinSlopeStep);
        const float hR = game._terrain.heightAt(_nextX + kCoinSlopeStep);
        const float slope = (hL - hR) / (2.0f * kCoinSlopeStep); // +: rising ahead (ramp up)
        const bool steepRamp = slope > kCoinSteepSlope;

        // Coupling: a nearby rocket, or a launch ramp, biases toward airborne arcs.
        const bool nearRocket = game._pickups.rocket().activeNear(_nextX, kCoinRocketRadius);
        float airChance = kCoinAirChance;
        if (nearRocket) {
            airChance += kCoinRocketAirBoost;
        }
        if (steepRamp) {
            airChance += 0.25f;
        }
        const bool inAir = unit(game._rng) < airChance;

        // Choose a shape. Airborne clusters favour arcs/rings; ground clusters favour
        // lines/waves/polygons.
        CoinShape shape;
        if (inAir) {
            const CoinShape airShapes[] = {CoinShape::Arch, CoinShape::Circle, CoinShape::Wave, CoinShape::Diamond};
            shape = airShapes[std::uniform_int_distribution<int>(0, 3)(game._rng)];
        } else {
            const CoinShape groundShapes[] = {
                CoinShape::Line, CoinShape::Wave, CoinShape::Triangle, CoinShape::Rect, CoinShape::Diamond};
            shape = groundShapes[std::uniform_int_distribution<int>(0, 4)(game._rng)];
        }

        const float scale = scaleDist(game._rng);
        // Line/wave follow the slope; other shapes stay upright.
        const float shapeSlope =
            (shape == CoinShape::Line || shape == CoinShape::Wave) ? clampf(slope, -1.2f, 1.2f) : 0.0f;
        std::vector<Vector2> local = buildShapePoints(shape, scale, shapeSlope, game._rng);
        if (local.empty()) {
            _nextX += gapDist(game._rng);
            continue;
        }

        // Anchor height above the ground under the cluster centre.
        std::uniform_real_distribution<float> airLift(kCoinAirLiftMin, kCoinAirLiftMax);
        const float lift = inAir ? airLift(game._rng) : kCoinGroundLift;
        const float anchorX = _nextX;
        const float groundY = game._terrain.heightAt(anchorX);
        const float anchorY = groundY - lift; // world y up = smaller

        // Cluster x-extent for overlap checks.
        float minLocalX = local.front().x;
        float maxLocalX = local.front().x;
        for (const Vector2& p : local) {
            minLocalX = std::min(minLocalX, p.x);
            maxLocalX = std::max(maxLocalX, p.x);
        }
        const float clusterStart = anchorX + minLocalX;
        const float clusterEnd = anchorX + maxLocalX;

        // Keep clusters off solid pickups (rockets couple instead of block).
        if (game._pickups.fuel().activeInRange(clusterStart - 3.0f, clusterEnd + 3.0f) ||
            game._pickups.flea().activeInRange(clusterStart - 3.0f, clusterEnd + 3.0f) ||
            game._pickups.cactus().activeNear((clusterStart + clusterEnd) * 0.5f, 5.0f) ||
            game._pickups.snowman().activeNear((clusterStart + clusterEnd) * 0.5f, 5.0f)) {
            _nextX += 5.0f;
            continue;
        }

        // Ensure the WHOLE shape clears the terrain without deforming it: find the
        // point that would sink deepest below its local ground clearance, then lift
        // the entire cluster up by that much (world y is down; smaller = higher).
        float extraLift = 0.0f;
        for (const Vector2& p : local) {
            const float wx = anchorX + p.x;
            const float wy = anchorY - p.y;
            const float ceil = game._terrain.heightAt(wx) - kCoinMinClearance; // highest allowed world y
            extraLift = std::max(extraLift, wy - ceil); // >0 means this point is below the ceiling
        }
        const float placeY = anchorY - extraLift;

        // Place each coin (shape kept intact, just shifted up as a whole).
        for (const Vector2& p : local) {
            createAt(game, {anchorX + p.x, placeY - p.y});
        }

        _nextX = clusterEnd + gapDist(game._rng);
    }
}

void CoinPickups::create(RidgeDashGame& game, const TerrainSample& terrain, float yOffset)
{
    createAt(game, {terrain.x, terrain.y + yOffset});
}

void CoinPickups::createAt(RidgeDashGame& game, Vector2 worldPos)
{
    if (!b2World_IsValid(game._worldId)) {
        return;
    }

    _items.push_back(Item{});
    Item& coin = _items.back();
    coin.pos = worldPos;

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
    game._ui.triggerDistanceCelebration();
    game.playSfx(AudioSystem::Sfx::Coin);
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

void CoinPickups::forceSpawnAt(RidgeDashGame& game, float x)
{
    const float y = game._terrain.heightAt(x);
    createAt(game, {x, y - 0.8f});
}

} // namespace ridge_dash
