#include "ServerChunkMap.hpp"

#include <ctime>

void ServerChunkMap::addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block) {
    blocks[blockPos] = block;
}

std::shared_ptr<Block> ServerChunkMap::getBlock(Vec3<float> blockPos) {
    return blocks[blockPos];
}

void ServerChunkMap::generate(int genid) {
    int minid = 0;
    int maxid = 3;

    srand((unsigned long long)std::chrono::system_clock::now().time_since_epoch().count());

    switch (genid) {
        case -1:
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    for (int z = 0; z < 8; z++) {
                        int id = minid + (rand() % (maxid - minid + 1));
                        Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
                        std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos);
                        blocks[blockPos] = block;
                    }
                }
            }
            break;
        case 0:
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    for (int z = 0; z < 8; z++) {
                        Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
                        std::shared_ptr<Block> block = std::make_shared<Block>(0, blockPos);
                        blocks[blockPos] = block;
                    }
                }
            }
            break;
    }
}


