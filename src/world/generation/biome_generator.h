#pragma once

#include "core/types.h"
#include <utility>
#include <random>

namespace Sylva {

class BiomeGenerator {
public:
    BiomeGenerator(const WorldParams& params);
    std::pair<float, float> generateBiomeData(float worldX, float worldZ, std::mt19937& rng) const;
    void applyEnvironmentalEffects(class Chunk* chunk, int x_local, int z_local, float height, float humidity, float temperature) const;
private:
    WorldParams m_params;
};

} // namespace Sylva 