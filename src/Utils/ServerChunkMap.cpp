#include "ServerChunkMap.hpp"

#include <chrono>
#include <ctime>
#include <deque>
#include <iostream>
#include <algorithm>
#include <Server.hpp>

#include "PerlinNoise.hpp"

bool checkValidPos(Vec3<float> pos) {
    if (pos.x >= 0.0f && pos.y >= 0.0f && pos.z >= 0.0f && pos.x <= 7.0f && pos.y <= 7.0f && pos.z <= 7.0f) {
        return true;
    }
    return false;
}

void ServerChunkMap::addBlock(Vec3<float> blockPos, std::shared_ptr<Block> block) {
    blocks[blockPos] = block;
}

std::shared_ptr<Block> ServerChunkMap::getBlock(Vec3<float> blockPos) {
    if (!checkValidPos(blockPos)) { printf("%f %f %f bad\n", blockPos.x, blockPos.y, blockPos.z); }
    return blocks[blockPos];
}

void ServerChunkMap::generateOres(Vec3<float> chunkPos,  int oreBlockId, int clusterCount, int clusterSize, int minY, int maxY) {
    uint64_t seed =
            Server::getInstance().seedMap->seedOres
            ^ (uint64_t(chunkPos.x) * 341873128712ULL)
            ^ (uint64_t(chunkPos.z) * 132897987541ULL)
            ^ (uint64_t(chunkPos.y) * 42317861ULL)
            ^ (uint64_t(oreBlockId) * 31ULL);
    std::mt19937 rng(seed);

    std::uniform_int_distribution<int> distX(0, 7);
    std::uniform_int_distribution<int> distY(0, 7);
    std::uniform_int_distribution<int> distZ(0, 7);
    std::uniform_int_distribution<int> step(-1, 1);

    for (int i = 0; i < clusterCount; i++) {
        int x = distX(rng);
        int y = distY(rng);
        int z = distZ(rng);

        int worldY = int(chunkPos.y) * 8 + y;
        if (worldY < minY || worldY > maxY)
            continue;

        for (int j = 0; j < clusterSize; j++) {
            if (x < 0 || x >= 8 || y < 0 || y >= 8 || z < 0 || z >= 8)
                break;

            Vec3<float> pos((float)x, (float)y, (float)z);

            auto it = blocks.find(pos);

            if(it->second->id != 1) continue;
            it->second = std::make_shared<Block>(oreBlockId, pos);

            x += step(rng);
            y += step(rng);
            z += step(rng);
        }
    }
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
                float maxy = (Server::getInstance().seedMap->perlinNoiseTerrain->generateOctaves(chunkPos.x * 8 + (float)x, chunkPos.z * 8 + (float)z, 4.f, 0.007f, 0.5f) + 1.0f) / 2.0f;

                maxy = -24.0f + (48.0f * maxy);
                for (int y = 0; y < 8; y++) {
                    Vec3<float> blockPos = Vec3((float)x, (float)y, (float)z);
                    int id = 2;
                    if (chunkPos.y * 8 + (float)y > maxy) {
                        id = 0;
                    }
                    if (chunkPos.y * 8 + (float)y == std::floor(maxy)) {
                        id = 3;
                        Vec3<float> ambientPos = Vec3<float>((float)x, (float)y + 1, (float)z);
                        float height = chunkPos.y;
                        if (ambientPos.y > 7.0f) {
                            height += 1;
                            ambientPos.y = 0.0f;
                        }
                        // if (chunkPos.x == 0.0f && chunkPos.z == 0.0f && x == 0 && z == 0) { std::cout << ambientPos.y << ", " << y << std::endl; }
                        HeightMap::getInstance().addMap(Vec2<float>(chunkPos.x, chunkPos.z), height, ambientPos);
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

                int wx = (int)chunkPos.x * 8 + x;
                int wy = (int)chunkPos.y * 8 + y;
                int wz = (int)chunkPos.z * 8 + z;

                float k = (Server::getInstance().seedMap->perlinNoiseCaves->generate3D((float)wx, (float)wy, (float)wz, 0.02f) + 1.0f) / 2.0f;

                if (k > 0.6f) {
                    blocks[blockPos] = std::make_shared<Block>(0, blockPos);
                    checkHeight(chunkPos, blockPos);
                }
            }
        }
    }
    if (chunkPos.y < -4.0f) {
        generateOres(chunkPos, 4, 1, 3, -250, -35);
    }

    // if (HeightMap::getInstance().heightMaps.count(Vec2<float>(chunkPos.x, chunkPos.z))) {
    //     for (auto& h : HeightMap::getInstance().heightMaps[Vec2<float>(chunkPos.x, chunkPos.z)]) {
    //         if (h.second.y - 1.0f < 0.0f) {
    //             if (Server::getInstance().chunks.count(Vec3<float>(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z))) {
    //                 checkHeight(Vec3<float>(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z), Vec3<float>(h.second.x, 7.0f, h.second.z));
    //             }
    //         }
    //         else {
    //             checkHeight(chunkPos, Vec3<float>(h.second.x, h.second.y - 1.0f, h.second.z));
    //         }
    //     }
    // }

    resetAmbient();
    checkAmbient(chunkPos);
}

void ServerChunkMap::resetLights() {
    for (auto& b : blocks) {
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        if (b.second->id != 0) {
            for (int i = 0; i < 6; i++) {
                b.second->lightLevels[i].x = 0;
            }
        }
    }
}

std::set<Vec3<float>> ServerChunkMap::checkLights(Vec3<float> chunkPos, Block prevblock) {
    std::map<std::pair<Vec3<float>, Vec3<float>>, int> lightResult;
    std::set<Vec3<float>> affectedChunks;
    affectedChunks.insert(chunkPos);
    // int ch = 0;

    if (prevblock.id == 5) {
        int lightIntensity = 5;
        std::map<std::pair<Vec3<float>, Vec3<float>>, int> lightQueue;
        lightQueue[std::make_pair(chunkPos, prevblock.position)] = 0;
        while (!lightQueue.empty() && lightIntensity > 1) {
            // std::cout << lightQueue.size() << std::endl;
            std::map<std::pair<Vec3<float>, Vec3<float>>, int> tempQueue;
            for (auto& l : lightQueue) {
                // x
                // printf("%f %f %f chunk\n", l.first.first.x, l.first.first.y, l.first.first.z);
                // printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
                //     printf("%f %f %f chunk %f %f %f block %d lvl\n", l.first.first.x, l.first.first.y, l.first.first.z, l.first.second.x, l.first.second.y, l.first.second.z, l.second);
                // }
                {
                    float x = l.first.second.x - 1.0f;
                    Vec3<float> lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                    Vec3<float> lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (x < 0.0f) {
                        x = 7.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z)) && checkValidPos(Vec3<float>(x, l.first.second.y, l.first.second.z))) {
                            lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                            lightChunk = Vec3<float>(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z);
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                            affectedChunks.insert(lightChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!tempQueue.count(lightBlock)) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                        }
                        if (!lightResult.count(lightBlock)) {
                            lightResult[lightBlock] = 0;
                        }
                    }
                }
                {
                    float x = l.first.second.x + 1.0f;
                    Vec3<float> lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                    Vec3<float> lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (x > 7.0f) {
                        x = 0.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z)) && checkValidPos(Vec3<float>(x, l.first.second.y, l.first.second.z))) {
                            lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                            lightChunk = Vec3<float>(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z);
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                            affectedChunks.insert(lightChunk);
                        }
                    }
                    else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!tempQueue.count(lightBlock)) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                        }
                        if (!lightResult.count(lightBlock)) {
                            lightResult[lightBlock] = 0;
                        }
                    }
                }
                // y
                {
                    float y = l.first.second.y - 1.0f;
                    Vec3<float> lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                    Vec3<float> lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (y < 0.0f) {
                        y = 7.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z)) && checkValidPos(Vec3<float>(l.first.second.x, y, l.first.second.z))) {
                            lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                            lightChunk = Vec3<float>(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z);
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                            affectedChunks.insert(lightChunk);
                        }
                    }
                    else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!tempQueue.count(lightBlock)) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                        }
                        if (!lightResult.count(lightBlock)) {
                            lightResult[lightBlock] = 0;
                        }
                    }
                }
                {
                    float y = l.first.second.y + 1.0f;
                    Vec3<float> lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                    Vec3<float> lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (y > 7.0f) {
                        y = 0.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z)) && checkValidPos(Vec3<float>(l.first.second.x, y, l.first.second.z))) {
                            lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                            lightChunk = Vec3<float>(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z);
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                            affectedChunks.insert(lightChunk);
                        }
                    }
                    else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!tempQueue.count(lightBlock)) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                        }
                        if (!lightResult.count(lightBlock)) {
                            lightResult[lightBlock] = 0;
                        }
                    }
                }
                // z
                {
                    float z = l.first.second.z - 1.0f;
                    Vec3<float> lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                    Vec3<float> lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (z < 0.0f) {
                        z = 7.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f)) && checkValidPos(Vec3<float>(l.first.second.x, l.first.second.y, z))) {
                            lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                            lightChunk = Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f);
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                            affectedChunks.insert(lightChunk);
                        }
                    }
                    else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    // if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
                    //     printf("%f %f %f chunk %f %f %f block %d lvl ZZZ\n", lightChunk.x, lightChunk.y, lightChunk.z, lightPos.x, lightPos.y, lightPos.z, l.second);
                    //     printf("%d test1\n", (int)(block != nullptr));
                    //     printf("%d test2\n", (int)(block->id == 0));
                    // }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!tempQueue.count(lightBlock)) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                        }
                        if (!lightResult.count(lightBlock)) {
                            lightResult[lightBlock] = 0;
                        }
                    }
                }
                {
                    float z = l.first.second.z + 1.0f;
                    Vec3<float> lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                    Vec3<float> lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (z > 7.0f) {
                        z = 0.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f)) && checkValidPos(Vec3<float>(l.first.second.x, l.first.second.y, z))) {
                            lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                            lightChunk = Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f);
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                            affectedChunks.insert(lightChunk);
                        }
                    }
                    else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!tempQueue.count(lightBlock)) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                        }
                        if (!lightResult.count(lightBlock)) {
                            lightResult[lightBlock] = 0;
                        }
                    }
                }
            }
            lightIntensity -= 1;
            lightQueue = tempQueue;
        }
    }

    std::map<Vec3<float>, std::shared_ptr<Block>> blockMap = blocks;
    for (auto& b : blockMap) {
        // ch++;
        // std::cout << ch << " ch" << std::endl;
        // std::cout << b.second->id << " id" << std::endl;
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        // if (b.second == nullptr) { continue; }
        if (b.second->id == 5) {
            int lightIntensity = 5;
            Vec3<float> blockPos = b.first;
            // printf("%f %f %f start\n", b.first.x, b.first.y, b.first.z);
            std::map<std::pair<Vec3<float>, Vec3<float>>, int> lightQueue;
            lightQueue[std::make_pair(chunkPos, blockPos)] = lightIntensity;
            while (!lightQueue.empty() && lightIntensity > 1) {
                // std::cout << lightQueue.size() << std::endl;
                std::map<std::pair<Vec3<float>, Vec3<float>>, int> tempQueue;
                for (auto& l : lightQueue) {
                    // x
                    // printf("%f %f %f chunk\n", l.first.first.x, l.first.first.y, l.first.first.z);
                    // printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
                    //     printf("%f %f %f chunk %f %f %f block %d lvl\n", l.first.first.x, l.first.first.y, l.first.first.z, l.first.second.x, l.first.second.y, l.first.second.z, l.second);
                    // }
                    {
                        float x = l.first.second.x - 1.0f;
                        Vec3<float> lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (x < 0.0f) {
                            x = 7.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z)) && checkValidPos(Vec3<float>(x, l.first.second.y, l.first.second.z))) {
                                lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                                lightChunk = Vec3<float>(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z);
                                block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                                affectedChunks.insert(lightChunk);
                            }
                        }
                        // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                        //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                        // }
                        else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        if (block != nullptr && block->id == 0) {
                            std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                            if (!tempQueue.count(lightBlock)) {
                                tempQueue[lightBlock] = lightIntensity - 1;
                            }
                            if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                                lightResult[lightBlock] = lightIntensity - 1;
                            }
                        }
                    }
                    {
                        float x = l.first.second.x + 1.0f;
                        Vec3<float> lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (x > 7.0f) {
                            x = 0.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z)) && checkValidPos(Vec3<float>(x, l.first.second.y, l.first.second.z))) {
                                lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                                lightChunk = Vec3<float>(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z);
                                block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                                affectedChunks.insert(lightChunk);
                            }
                        }
                        else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        if (block != nullptr && block->id == 0) {
                            std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                            if (!tempQueue.count(lightBlock)) {
                                tempQueue[lightBlock] = lightIntensity - 1;
                            }
                            if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                                lightResult[lightBlock] = lightIntensity - 1;
                            }
                        }
                    }
                    // y
                    {
                        float y = l.first.second.y - 1.0f;
                        Vec3<float> lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (y < 0.0f) {
                            y = 7.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z)) && checkValidPos(Vec3<float>(l.first.second.x, y, l.first.second.z))) {
                                lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                                lightChunk = Vec3<float>(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z);
                                block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                                affectedChunks.insert(lightChunk);
                            }
                        }
                        else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        if (block != nullptr && block->id == 0) {
                            std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                            if (!tempQueue.count(lightBlock)) {
                                tempQueue[lightBlock] = lightIntensity - 1;
                            }
                            if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                                lightResult[lightBlock] = lightIntensity - 1;
                            }
                        }
                    }
                    {
                        float y = l.first.second.y + 1.0f;
                        Vec3<float> lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (y > 7.0f) {
                            y = 0.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z)) && checkValidPos(Vec3<float>(l.first.second.x, y, l.first.second.z))) {
                                lightPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                                lightChunk = Vec3<float>(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z);
                                block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                                affectedChunks.insert(lightChunk);
                            }
                        }
                        else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        if (block != nullptr && block->id == 0) {
                            std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                            if (!tempQueue.count(lightBlock)) {
                                tempQueue[lightBlock] = lightIntensity - 1;
                            }
                            if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                                lightResult[lightBlock] = lightIntensity - 1;
                            }
                        }
                    }
                    // z
                    {
                        float z = l.first.second.z - 1.0f;
                        Vec3<float> lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (z < 0.0f) {
                            z = 7.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f)) && checkValidPos(Vec3<float>(l.first.second.x, l.first.second.y, z))) {
                                lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                                lightChunk = Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f);
                                block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                                affectedChunks.insert(lightChunk);
                            }
                        }
                        else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        // if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
                        //     printf("%f %f %f chunk %f %f %f block %d lvl ZZZ\n", lightChunk.x, lightChunk.y, lightChunk.z, lightPos.x, lightPos.y, lightPos.z, l.second);
                        //     printf("%d test1\n", (int)(block != nullptr));
                        //     printf("%d test2\n", (int)(block->id == 0));
                        // }
                        if (block != nullptr && block->id == 0) {
                            std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                            if (!tempQueue.count(lightBlock)) {
                                tempQueue[lightBlock] = lightIntensity - 1;
                            }
                            if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                                lightResult[lightBlock] = lightIntensity - 1;
                            }
                        }
                    }
                    {
                        float z = l.first.second.z + 1.0f;
                        Vec3<float> lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (z > 7.0f) {
                            z = 0.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f)) && checkValidPos(Vec3<float>(l.first.second.x, l.first.second.y, z))) {
                                lightPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                                lightChunk = Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f);
                                block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                                affectedChunks.insert(lightChunk);
                            }
                        }
                        else if (checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        if (block != nullptr && block->id == 0) {
                            std::pair<Vec3<float>, Vec3<float>> lightBlock = std::make_pair(lightChunk, lightPos);
                            if (!tempQueue.count(lightBlock)) {
                                tempQueue[lightBlock] = lightIntensity - 1;
                            }
                            if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                                lightResult[lightBlock] = lightIntensity - 1;
                            }
                        }
                    }
                }
                lightIntensity -= 1;
                lightQueue = tempQueue;
            }
        }
    }

    // std::cout << "kekw" << std::endl;
    // std::cout << lightResult.size() << std::endl;
    for (auto& r : lightResult) {
        // EditChunkServer packet;
        // packet.id = 1;
        // packet.chunkpos = r.first.first;
        // packet.blockpos = r.first.second;
        // for (auto& s : Server::getInstance().clients) {
        //     Server::getInstance().sendPacket(s.first, &packet);
        // }
        // x
        // if (r.first.first.x == 0.0f && r.first.first.z == -1.0f) {
        //     printf("%f %f %f block %d guh\n", r.first.second.x, r.first.second.y, r.first.second.z, r.second);
        // }
        // printf("%f %f %f\n", r.first.second.x, r.first.second.y, r.first.second.z);
        // printf("%d light\n", r.second);
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x - 1.0f, r.first.second.y, r.first.second.z);
            if (resultBlock.x < 0.0f) {
                resultBlock.x = 7.0f;
                resultChunk.x -= 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[3].x < r.second || r.second == 0)) {
                // std::cout << "front" << std::endl;
                block->lightLevels[3].x = r.second;
            }
        }
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x + 1.0f, r.first.second.y, r.first.second.z);
            if (resultBlock.x > 7.0f) {
                resultBlock.x = 0.0f;
                resultChunk.x += 1.0f;
                affectedChunks.insert(resultChunk);
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[2].x < r.second || r.second == 0)) {
                // std::cout << "back" << std::endl;
                block->lightLevels[2].x = r.second;
            }
        }
        // y
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y - 1.0f, r.first.second.z);
            if (resultBlock.y < 0.0f) {
                resultBlock.y = 7.0f;
                resultChunk.y -= 1.0f;
                affectedChunks.insert(resultChunk);
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[5].x < r.second || r.second == 0)) {
                // std::cout << "top" << std::endl;
                block->lightLevels[5].x = r.second;
            }
        }
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y + 1.0f, r.first.second.z);
            if (resultBlock.y > 7.0f) {
                resultBlock.y = 0.0f;
                resultChunk.y += 1.0f;
                affectedChunks.insert(resultChunk);
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[4].x < r.second || r.second == 0)) {
                // std::cout << "bottom" << std::endl;
                block->lightLevels[4].x = r.second;
            }
        }
        // z
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y, r.first.second.z - 1.0f);
            if (resultBlock.z < 0.0f) {
                resultBlock.z = 7.0f;
                resultChunk.z -= 1.0f;
                affectedChunks.insert(resultChunk);
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[1].x < r.second || r.second == 0)) {
                // std::cout << "right" << std::endl;
                block->lightLevels[1].x = r.second;
            }
        }
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y, r.first.second.z + 1.0f);
            if (resultBlock.z > 7.0f) {
                resultBlock.z = 0.0f;
                resultChunk.z += 1.0f;
                affectedChunks.insert(resultChunk);
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[0].x < r.second || r.second == 0)) {
                // std::cout << "left" << std::endl;
                block->lightLevels[0].x = r.second;
            }
        }
    }

    return affectedChunks;
}

void ServerChunkMap::resetAmbient() {
    for (auto& b : blocks) {
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        if (b.second->id != 0) {
            for (int i = 0; i < 6; i++) {
                b.second->lightLevels[i].y = -5;
            }
        }
    }
}

std::set<Vec3<float>> ServerChunkMap::checkAmbient(Vec3<float> chunkPos) {
    std::map<std::pair<Vec3<float>, Vec3<float>>, int> darkResult;
    std::set<Vec3<float>> affectedChunks;
    affectedChunks.insert(chunkPos);
    // int ch = 0;

    std::set<Vec3<float>> ambients;
    for (auto& a : HeightMap::getInstance().heightMaps[Vec2<float>(chunkPos.x, chunkPos.z)]) {
        ambients.insert(Vec3<float>(a.second.x, ((chunkPos.y * 8.0f) - (a.first * 8.0f)) + a.second.y, a.second.z));
        darkResult[std::pair(chunkPos, Vec3<float>(a.second.x, ((chunkPos.y * 8.0f) - (a.first * 8.0f)) + a.second.y, a.second.z))] = 0;
    }

    for (auto& a : ambients) {
        // ch++;
        // std::cout << ch << " ch" << std::endl;
        // std::cout << b.second->id << " id" << std::endl;
        // if (b.second == nullptr) { continue; }
        int darknessLevel = 0;
        // if (chunkPos.x == 0.0f && chunkPos.y == 0.0f && chunkPos.z == 0.0f) {
        //     printf("%f %f %f start\n", a.x, a.y, a.z);
        // }
        std::map<std::pair<Vec3<float>, Vec3<float>>, int> darknessQueue;
        darknessQueue[std::make_pair(chunkPos, a)] = darknessLevel;
        while (!darknessQueue.empty() && darknessLevel > -5) {
            // std::cout << lightQueue.size() << std::endl;
            std::map<std::pair<Vec3<float>, Vec3<float>>, int> tempQueue;
            // if (chunkPos.x == 0.0f && chunkPos.y == 0.0f && chunkPos.z == 0.0f && a.x == 0.0f && a.z == 0.0f) {
            //     printf("start\n");
            // }
            for (auto& l : darknessQueue) {
                // x
                // printf("%f %f %f chunk\n", l.first.first.x, l.first.first.y, l.first.first.z);
                // printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // if (chunkPos.x == 0.0f && chunkPos.y == 0.0f && chunkPos.z == 0.0f && l.first.x == 0.0f && l.first.z == 0.0f) {
                //     printf("%f %f %f block %d lvl\n", l.first.x, l.first.y, l.first.z, l.second);
                // }
                {
                    float x = l.first.second.x - 1.0f;
                    Vec3<float> darkPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                    Vec3<float> darkChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (x < 0.0f) {
                        x = 7.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(darkChunk.x - 1.0f, darkChunk.y, darkChunk.z)) && checkValidPos(Vec3<float>(x, l.first.second.y, l.first.second.z))) {
                            darkPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                            darkChunk = Vec3<float>(darkChunk.x - 1.0f, darkChunk.y, darkChunk.z);
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                            affectedChunks.insert(darkChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        if (darkChunk == chunkPos) {
                            block = blocks[darkPos];
                        }
                        else {
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> darkBlock = std::make_pair(darkChunk, darkPos);
                        if (!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) {
                            tempQueue[darkBlock] = darknessLevel - 1;
                            darkResult[darkBlock] = darknessLevel - 1;
                        }
                    }
                }
                {
                    float x = l.first.second.x + 1.0f;
                    Vec3<float> darkPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                    Vec3<float> darkChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (x > 7.0f) {
                        x = 0.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(darkChunk.x + 1.0f, darkChunk.y, darkChunk.z)) && checkValidPos(Vec3<float>(x, l.first.second.y, l.first.second.z))) {
                            darkPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                            darkChunk = Vec3<float>(darkChunk.x + 1.0f, darkChunk.y, darkChunk.z);
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                            affectedChunks.insert(darkChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        if (darkChunk == chunkPos) {
                            block = blocks[darkPos];
                        }
                        else {
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> darkBlock = std::make_pair(darkChunk, darkPos);
                        if (!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) {
                            tempQueue[darkBlock] = darknessLevel - 1;
                            darkResult[darkBlock] = darknessLevel - 1;
                        }
                    }
                }
                // y
                {
                    float y = l.first.second.y - 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                    Vec3<float> darkChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (y < 0.0f) {
                        y = 7.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(darkChunk.x, darkChunk.y - 1.0f, darkChunk.z)) && checkValidPos(Vec3<float>(l.first.second.x, y, l.first.second.z))) {
                            darkPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                            darkChunk = Vec3<float>(darkChunk.x, darkChunk.y - 1.0f, darkChunk.z);
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                            affectedChunks.insert(darkChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        if (darkChunk == chunkPos) {
                            block = blocks[darkPos];
                        }
                        else {
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> darkBlock = std::make_pair(darkChunk, darkPos);
                        if (!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) {
                            tempQueue[darkBlock] = darknessLevel - 1;
                            darkResult[darkBlock] = darknessLevel - 1;
                        }
                    }
                }
                {
                    float y = l.first.second.y + 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                    Vec3<float> darkChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (y > 7.0f) {
                        y = 0.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(darkChunk.x, darkChunk.y + 1.0f, darkChunk.z)) && checkValidPos(Vec3<float>(l.first.second.x, y, l.first.second.z))) {
                            darkPos = Vec3<float>(l.first.second.x, y, l.first.second.z);
                            darkChunk = Vec3<float>(darkChunk.x, darkChunk.y + 1.0f, darkChunk.z);
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                            affectedChunks.insert(darkChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        if (darkChunk == chunkPos) {
                            block = blocks[darkPos];
                        }
                        else {
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> darkBlock = std::make_pair(darkChunk, darkPos);
                        if (!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) {
                            tempQueue[darkBlock] = darknessLevel - 1;
                            darkResult[darkBlock] = darknessLevel - 1;
                        }
                    }
                }
                // z
                {
                    float z = l.first.second.z - 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                    Vec3<float> darkChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (z < 0.0f) {
                        z = 7.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(darkChunk.x, darkChunk.y, darkChunk.z - 1.0f)) && checkValidPos(Vec3<float>(l.first.second.x, l.first.second.y, z))) {
                            darkPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                            darkChunk = Vec3<float>(darkChunk.x, darkChunk.y, darkChunk.z - 1.0f);
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                            affectedChunks.insert(darkChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        if (darkChunk == chunkPos) {
                            block = blocks[darkPos];
                        }
                        else {
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> darkBlock = std::make_pair(darkChunk, darkPos);
                        if (!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) {
                            tempQueue[darkBlock] = darknessLevel - 1;
                            darkResult[darkBlock] = darknessLevel - 1;
                        }
                    }
                }
                {
                    float z = l.first.second.z + 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                    Vec3<float> darkChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    if (z > 7.0f) {
                        z = 0.0f;
                        if (Server::getInstance().chunks.count(Vec3<float>(darkChunk.x, darkChunk.y, darkChunk.z + 1.0f)) && checkValidPos(Vec3<float>(l.first.second.x, l.first.second.y, z))) {
                            darkPos = Vec3<float>(l.first.second.x, l.first.second.y, z);
                            darkChunk = Vec3<float>(darkChunk.x, darkChunk.y, darkChunk.z + 1.0f);
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                            affectedChunks.insert(darkChunk);
                        }
                    }
                    // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(Vec3<float>(lightPos.x, lightPos.y, lightPos.z))) {
                    //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    // }
                    else if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        if (darkChunk == chunkPos) {
                            block = blocks[darkPos];
                        }
                        else {
                            block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<Vec3<float>, Vec3<float>> darkBlock = std::make_pair(darkChunk, darkPos);
                        if (!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) {
                            tempQueue[darkBlock] = darknessLevel - 1;
                            darkResult[darkBlock] = darknessLevel - 1;
                        }
                    }
                }
            }
            darknessLevel -= 1;
            darknessQueue = tempQueue;
        }
    }

    // std::cout << "kekw" << std::endl;
    // std::cout << darkResult.size() << std::endl;
    for (auto& r : darkResult) {
        // x
        // printf("%d dark\n", r.second);
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x - 1.0f, r.first.second.y, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.x < 0.0f) {
                resultBlock.x = 7.0f;
                resultChunk.x -= 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                    affectedChunks.insert(resultChunk);
                }
            }
            else if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = blocks[resultBlock];
                }
                else {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[3].y < r.second) {
                // std::cout << "front" << std::endl;
                block->lightLevels[3].y = r.second;
            }
        }
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x + 1.0f, r.first.second.y, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.x > 7.0f) {
                resultBlock.x = 0.0f;
                resultChunk.x += 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                    affectedChunks.insert(resultChunk);
                }
            }
            else if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = blocks[resultBlock];
                }
                else {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[2].y < r.second) {
                // std::cout << "back" << std::endl;
                block->lightLevels[2].y = r.second;
            }
        }
        // y
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y - 1.0f, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.y < 0.0f) {
                resultBlock.y = 7.0f;
                resultChunk.y -= 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                    affectedChunks.insert(resultChunk);
                }
            }
            else if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = blocks[resultBlock];
                }
                else {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            // if (chunkPos.x == 0.0f && chunkPos.y == 0.0f && chunkPos.z == 0.0f && resultBlock.x == 0.0f && resultBlock.z == 0.0f) {
            //     printf("%f %f %f block %d lvl\n", resultBlock.x, resultBlock.y, resultBlock.z, r.second);
            //     printf("%d %d %d checks\n", (int)(block != nullptr), (int)(block->id != 0), (int)(block->lightLevels[5].y < r.second));
            // }
            if (block != nullptr && block->id != 0 && block->lightLevels[5].y < r.second) {
                // std::cout << "top" << std::endl;
                block->lightLevels[5].y = r.second;
            }
        }
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y + 1.0f, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.y > 7.0f) {
                resultBlock.y = 0.0f;
                resultChunk.y += 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                    affectedChunks.insert(resultChunk);
                }
            }
            else if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = blocks[resultBlock];
                }
                else {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[4].y < r.second) {
                // std::cout << "bottom" << std::endl;
                block->lightLevels[4].y = r.second;
            }
        }
        // z
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y, r.first.second.z - 1.0f);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.z < 0.0f) {
                resultBlock.z = 7.0f;
                resultChunk.z -= 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                    affectedChunks.insert(resultChunk);
                }
            }
            else if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = blocks[resultBlock];
                }
                else {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[1].y < r.second) {
                // std::cout << "right" << std::endl;
                block->lightLevels[1].y = r.second;
            }
        }
        {
            Vec3<float> resultChunk = r.first.first;
            Vec3<float> resultBlock = Vec3<float>(r.first.second.x, r.first.second.y, r.first.second.z + 1.0f);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.z > 7.0f) {
                resultBlock.z = 0.0f;
                resultChunk.z += 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                    affectedChunks.insert(resultChunk);
                }
            }
            else if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = blocks[resultBlock];
                }
                else {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[0].y < r.second) {
                // std::cout << "left" << std::endl;
                block->lightLevels[0].y = r.second;
            }
        }
    }
    return affectedChunks;
}

std::set<Vec3<float>> ServerChunkMap::checkHeight(Vec3<float> chunkPos, Vec3<float> blockPos) {
    std::set<Vec3<float>> affectedChunks;
    affectedChunks.insert(chunkPos);

    // if (blockPos.x == 0.0f && Server::getInstance().chunks.count(Vec3<float>(chunkPos.x - 1.0f, chunkPos.y, chunkPos.z))) {
    //     affectedChunks.insert(Vec3<float>(chunkPos.x - 1.0f, chunkPos.y, chunkPos.z));
    // }
    // if (blockPos.x == 7.0f && Server::getInstance().chunks.count(Vec3<float>(chunkPos.x + 1.0f, chunkPos.y, chunkPos.z))) {
    //     affectedChunks.insert(Vec3<float>(chunkPos.x + 1.0f, chunkPos.y, chunkPos.z));
    // }
    //
    // if (blockPos.y == 0.0f && Server::getInstance().chunks.count(Vec3<float>(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z))) {
    //     affectedChunks.insert(Vec3<float>(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z));
    // }
    // if (blockPos.y == 7.0f && Server::getInstance().chunks.count(Vec3<float>(chunkPos.x, chunkPos.y + 1.0f, chunkPos.z))) {
    //     affectedChunks.insert(Vec3<float>(chunkPos.x, chunkPos.y + 1.0f, chunkPos.z));
    // }
    //
    // if (blockPos.z == 0.0f && Server::getInstance().chunks.count(Vec3<float>(chunkPos.x, chunkPos.y, chunkPos.z - 1.0f))) {
    //     affectedChunks.insert(Vec3<float>(chunkPos.x, chunkPos.y, chunkPos.z - 1.0f));
    // }
    // if (blockPos.z == 7.0f && Server::getInstance().chunks.count(Vec3<float>(chunkPos.x, chunkPos.y, chunkPos.z + 1.0f))) {
    //     affectedChunks.insert(Vec3<float>(chunkPos.x, chunkPos.y, chunkPos.z + 1.0f));
    // }

    if (!HeightMap::getInstance().heightMaps.count(Vec2<float>(chunkPos.x, chunkPos.z))) { return affectedChunks; }

    auto res = std::find_if(HeightMap::getInstance().heightMaps[Vec2<float>(chunkPos.x, chunkPos.z)].begin(), HeightMap::getInstance().heightMaps[Vec2<float>(chunkPos.x, chunkPos.z)].end(), [&](auto& heightblock) {
        if (heightblock.second.x == blockPos.x && heightblock.second.z == blockPos.z) { return true; } return false;
    });
    if (res == HeightMap::getInstance().heightMaps[Vec2<float>(chunkPos.x, chunkPos.z)].end()) { return affectedChunks; }

    // printf("%f %f %f chunk\n", chunkPos.x, chunkPos.z, chunkPos.z);

    if (res->second.y + (res->first * 8.0f) <= blockPos.y + (chunkPos.y * 8.0f) && blocks[blockPos]->id != 0) {
        float prevheight = res->first;
        if (blockPos.y + 1.0f > 7.0f) {
            res->first = chunkPos.y + 1.0f;
            res->second = Vec3<float>(blockPos.x, 0.0f, blockPos.z);
        }
        else {
            res->first = chunkPos.y;
            res->second = Vec3<float>(blockPos.x, blockPos.y + 1.0f, blockPos.z);
        }
        affectedChunks.insert(Vec3<float>(chunkPos.x, prevheight, chunkPos.z));
        affectedChunks.insert(Vec3<float>(chunkPos.x, res->first, chunkPos.z));
        return affectedChunks;
    }
    if (res->second.y + (res->first * 8.0f) > blockPos.y + (chunkPos.y * 8.0f) && blocks[blockPos]->id == 0) {
        float prevheight = res->first;
        float solidHeight = res->second.y;
        float chunkHeight = res->first;
        while (true) {
            if (solidHeight - 1.0f < 0.0f) {
                chunkHeight -= 1.0f;
                solidHeight = 7.0f;
            }
            else {
                solidHeight -= 1.0f;
            }
            Vec3<float> solidPos = Vec3<float>(res->second.x, solidHeight, res->second.z);
            Vec3<float> solidChunk = Vec3<float>(chunkPos.x, chunkHeight, chunkPos.z);

            if ((solidChunk.y == chunkPos.y && (blocks[solidPos]->id != 0)) || (solidChunk.y != chunkPos.y && !Server::getInstance().chunks.count(solidChunk)) || (Server::getInstance().chunks.count(solidChunk) && Server::getInstance().chunks[solidChunk]->blocks[solidPos]->id != 0)) {
                if (solidPos.y + 1.0f > 7.0f) {
                    solidChunk.y += 1.0f;
                    solidPos.y = 0.0f;
                }
                else {
                    solidPos.y += 1.0f;
                }
                res->first = solidChunk.y;
                res->second = Vec3<float>(solidPos.x, solidPos.y, solidPos.z);
                affectedChunks.insert(Vec3<float>(chunkPos.x, prevheight, chunkPos.z));
                affectedChunks.insert(Vec3<float>(chunkPos.x, res->first, chunkPos.z));
                return affectedChunks;
            }
        }
    }
    return affectedChunks;
}