#include "ServerChunkMap.hpp"

void ServerChunkMap::addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block) {
    blocks[blockPos] = block;
}

std::shared_ptr<Block> ServerChunkMap::getBlock(Vec3<float> blockPos) {
    return blocks[blockPos];
}

void ServerChunkMap::generate() {
    int minid = 0;
    int maxid = 3;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                int id = minid + (rand() % static_cast<int>(maxid - minid + 1));
                Vec3<float> blockPos = Vec3<float>((float)x, (float)y, (float)z);
                std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos);
                blocks[blockPos] = block;
            }
        }
    }
}


