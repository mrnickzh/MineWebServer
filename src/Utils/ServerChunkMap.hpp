#pragma once
#include <map>
#include <memory>

#include "Block.hpp"

class ServerChunkMap {
public:
    std::map<Vec3<float>, std::shared_ptr<Block>> blocks;

    void addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block);
    std::shared_ptr<Block> getBlock(Vec3<float> blockPos);
    void generate(Vec3<float> chunkPos);
};
