/**
 * @file biome.cpp
 * @author Forairaaaaa
 * @brief Biome renderer factory — returns the singleton for each biome type.
 * @version 0.1
 * @date 2026-07-14
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "game/world/biome/biome.hpp"

namespace ridge_dash {

const IBiomeRenderer& biomeRendererFor(TerrainBiome biome)
{
    static const MountainRenderer kMountain;
    static const StoneRenderer kStone;
    static const DesertRenderer kDesert;
    static const SnowRenderer kSnow;

    switch (biome) {
        case TerrainBiome::Stone:
            return kStone;
        case TerrainBiome::Desert:
            return kDesert;
        case TerrainBiome::Snow:
            return kSnow;
        case TerrainBiome::Mountain:
        default:
            return kMountain;
    }
}

} // namespace ridge_dash
