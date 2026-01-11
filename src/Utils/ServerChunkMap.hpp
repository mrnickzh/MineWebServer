#pragma once
#include <map>
#include <memory>
#include <set>

#include "Block.hpp"
#include "PerlinNoise.hpp"

class ServerChunkMap {
public:
    std::map<Vec3<float>, std::shared_ptr<Block>> blocks;

    void addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block);
    std::shared_ptr<Block> getBlock(Vec3<float> blockPos);
    void generate(Vec3<float> chunkPos);
    void generateOres(Vec3<float> chunkPos, int oreBlockId, int clusterCount, int clusterSize, int minY, int maxY);
    std::set<Vec3<float>> checkLights(Vec3<float> chunkPos);
    void checkAmbient(Vec3<float> chunkPos);
};
