#pragma once

#include "core/types.h"
#include <random>

namespace Sylva {

class FeatureGenerator {
public:
    FeatureGenerator(const WorldParams& params);
    void generateTrees(class Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);
    void generateRocks(class Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);
    void generateVegetation(class Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);
    void generateDecorations(class Chunk* chunk, std::mt19937& rng, std::uniform_int_distribution<int>& randPos);

private:
    WorldParams m_params;
};

} // namespace Sylva