#include "ServerChunkMap.hpp"

#include <chrono>
#include <ctime>

#include "PerlinNoise.hpp"

void ServerChunkMap::addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block) {
    blocks[blockPos] = block;
}

std::shared_ptr<Block> ServerChunkMap::getBlock(Vec3<float> blockPos) {
    return blocks[blockPos];
}

void ServerChunkMap::generate(Vec3<float> chunkPos) {
    PerlinNoise perlinNoise(1234);

    if (chunkPos.y > 3.0f) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
                    std::shared_ptr<Block> block = std::make_shared<Block>(0, blockPos);
                    blocks[blockPos] = block;
                }
            }
        }
        return;
    }

    if (chunkPos.y < -3.0f) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
                    std::shared_ptr<Block> block = std::make_shared<Block>(1, blockPos);
                    blocks[blockPos] = block;
                }
            }
        }
        return;
    }

    for (int x = 0; x < 8; x++) {
        for (int z = 0; z < 8; z++) {
            float maxy = (perlinNoise.generateOctaves(chunkPos.x * 8 + x, chunkPos.z * 8 + z, 4.f, 0.007f, 0.5f) + 1.0f) / 2.0f;
            maxy = -24.0f + (48.0f * maxy);
            for (int y = 0; y < 8; y++) {
                Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
                int id = 2;
                if (chunkPos.y * 8 + y > maxy) {
                    id = 0;
                }
                if (chunkPos.y * 8 + y == std::floor(maxy)) {
                    id = 3;
                }
                std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos);
                blocks[blockPos] = block;
            }
        }
    }

    // switch (genid) {
    //     case -1:
    //         for (int x = 0; x < 8; x++) {
    //             for (int y = 0; y < 8; y++) {
    //                 for (int z = 0; z < 8; z++) {
    //                     int id = minid + (rand() % (maxid - minid + 1));
    //                     Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
    //                     std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos);
    //                     blocks[blockPos] = block;
    //                 }
    //             }
    //         }
    //         break;
    //     case 0:
    //         for (int x = 0; x < 8; x++) {
    //             for (int y = 0; y < 8; y++) {
    //                 for (int z = 0; z < 8; z++) {
    //                     Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
    //                     std::shared_ptr<Block> block = std::make_shared<Block>(0, blockPos);
    //                     blocks[blockPos] = block;
    //                 }
    //             }
    //         }
    //         break;
    // }
}


