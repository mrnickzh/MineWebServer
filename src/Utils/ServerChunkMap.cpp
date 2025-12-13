#include "ServerChunkMap.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <Server.hpp>

#include "PerlinNoise.hpp"

void ServerChunkMap::addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block) {
    blocks[blockPos] = block;
}

std::shared_ptr<Block> ServerChunkMap::getBlock(Vec3<float> blockPos) {
    return blocks[blockPos];
}

void ServerChunkMap::generate(Vec3<float> chunkPos) {
    if (chunkPos.y > 3.0f) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    Vec3<float> blockPos = Vec3((float) x, (float) y, (float) z);
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
                    Vec3<float> blockPos = Vec3((float) x, (float) y, (float) z);
                    std::shared_ptr<Block> block = std::make_shared<Block>(1, blockPos);
                    blocks[blockPos] = block;
                }
            }
        }
    } else {
        for (int x = 0; x < 8; x++) {
            for (int z = 0; z < 8; z++) {
                float maxy = (Server::getInstance().seedMap->perlinNoiseTerrain->generateOctaves(chunkPos.x * 8 + x, chunkPos.z * 8 + z, 4.f, 0.007f, 0.5f) + 1.0f) / 2.0f;

                maxy = -24.0f + (48.0f * maxy);
                for (int y = 0; y < 8; y++) {
                    Vec3<float> blockPos = Vec3((float) x, (float) y, (float) z);
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
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                Vec3<float> blockPos((float) x, (float) y, (float) z);

                int wx = chunkPos.x * 8 + x;
                int wy = chunkPos.y * 8 + y;
                int wz = chunkPos.z * 8 + z;


                float nigger = (Server::getInstance().seedMap->perlinNoiseCaves->generate3D(wx, wy, wz, .02f) + 1.0f) / 2.0f;

                if (nigger > 0.6f) blocks[blockPos] = std::make_shared<Block>(0, blockPos);
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


