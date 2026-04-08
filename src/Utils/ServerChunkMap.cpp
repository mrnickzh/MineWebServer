#include "ServerChunkMap.hpp"

#include <chrono>
#include <ctime>
#include <deque>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <Server.hpp>

#include "PerlinNoise.hpp"

bool ServerChunkMap::checkValidPos(glm::vec3 pos) {
    if (pos.x >= 0.0f && pos.y >= 0.0f && pos.z >= 0.0f && pos.x <= 7.0f && pos.y <= 7.0f && pos.z <= 7.0f) {
        return true;
    }
    return false;
}

void ServerChunkMap::addBlock(glm::vec3 blockPos, std::shared_ptr<Block> block) {
    blocks[(int)(blockPos.x * 64 + blockPos.y * 8 + blockPos.z)] = block;
}

std::shared_ptr<Block> ServerChunkMap::getBlock(glm::vec3 blockPos) {
    // if (!checkValidPos(blockPos)) { printf("%f %f %f bad\n", blockPos.x, blockPos.y, blockPos.z); }
    return blocks[(int)(blockPos.x * 64 + blockPos.y * 8 + blockPos.z)];
}

void ServerChunkMap::generateOres(glm::vec3 chunkPos,  int oreBlockId, int clusterCount, int clusterSize, int minY, int maxY) {
    uint64_t seed =
            Server::getInstance().seedMap->seedOres
            ^ (uint64_t(chunkPos.x) * 341873128712ULL)
            ^ (uint64_t(chunkPos.z) * 132897987541ULL)
            ^ (uint64_t(chunkPos.y) * 42317861ULL)
            ^ (uint64_t(oreBlockId) * 31ULL);
    std::mt19937 rng(seed);

    for (int i = 0; i < clusterCount; i++) {
        int x = ((int)(rng() % 8));
        int y = ((int)(rng() % 8));
        int z = ((int)(rng() % 8));

        int worldY = int(chunkPos.y) * 8 + y;
        if (worldY < minY || worldY > maxY)
            continue;

        for (int j = 0; j < clusterSize; j++) {
            if (x < 0 || x >= 8 || y < 0 || y >= 8 || z < 0 || z >= 8)
                break;

            glm::vec3 pos((float)x, (float)y, (float)z);

            auto it = getBlock(pos);

            if(it->id != 1) continue;
            addBlock(pos, std::make_shared<Block>(oreBlockId, pos, glm::vec3(0.0f, 0.0f, 0.0f), true, glm::vec3(0.5f, 0.5f, 0.5f)));

            x += ((int)(rng() % 3)) - 1;
            y += ((int)(rng() % 3)) - 1;
            z += ((int)(rng() % 3)) - 1;
        }
    }
}

void ServerChunkMap::generate(glm::vec3 chunkPos) {
    if (chunkPos.y > 3.0f) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    glm::vec3 blockPos = glm::vec3((float) x, (float) y, (float) z);
                    std::shared_ptr<Block> block = std::make_shared<Block>(0, blockPos, glm::vec3(0.0f, 0.0f, 0.0f), false, glm::vec3(0.5f, 0.5f, 0.5f));
                    addBlock(blockPos, block);
                }
            }
        }
        return;
    }

    if (chunkPos.y < -3.0f) {
        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    glm::vec3 blockPos = glm::vec3((float) x, (float) y, (float) z);
                    std::shared_ptr<Block> block = std::make_shared<Block>(1, blockPos, glm::vec3(0.0f, 0.0f, 0.0f), true, glm::vec3(0.5f, 0.5f, 0.5f));
                    addBlock(blockPos, block);
                }
            }
        }
    } else {
        for (int x = 0; x < 8; x++) {
            for (int z = 0; z < 8; z++) {
                float maxy = (Server::getInstance().seedMap->perlinNoiseTerrain->generateOctaves(chunkPos.x * 8 + (float)x, chunkPos.z * 8 + (float)z, 4.f, 0.007f, 0.5f) + 1.0f) / 2.0f;

                maxy = -24.0f + (48.0f * maxy);
                for (int y = 0; y < 8; y++) {
                    glm::vec3 blockPos = glm::vec3((float)x, (float)y, (float)z);
                    int id = 2;
                    if (chunkPos.y * 8 + (float)y > maxy) {
                        id = 0;
                    }
                    if (chunkPos.y * 8 + (float)y == std::floor(maxy)) {
                        id = 3;
                        glm::vec3 ambientPos = glm::vec3((float)x, (float)y + 1, (float)z);
                        float height = chunkPos.y;
                        if (ambientPos.y > 7.0f) {
                            height += 1;
                            ambientPos.y = 0.0f;
                        }
                        // if (chunkPos.x == 0.0f && chunkPos.z == 0.0f && x == 0 && z == 0) { std::cout << ambientPos.y << ", " << y << std::endl; }
                        HeightMap::getInstance().addMap(glm::vec2(chunkPos.x, chunkPos.z), height, ambientPos);
                    }
                    std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos, glm::vec3(0.0f, 0.0f, 0.0f), (id == 0 ? false : true), glm::vec3(0.5f, 0.5f, 0.5f));
                    addBlock(blockPos, block);
                }
            }
        }
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                glm::vec3 blockPos((float) x, (float) y, (float) z);

                int wx = (int)chunkPos.x * 8 + x;
                int wy = (int)chunkPos.y * 8 + y;
                int wz = (int)chunkPos.z * 8 + z;

                float k = (Server::getInstance().seedMap->perlinNoiseCaves->generate3D((float)wx, (float)wy, (float)wz, 0.02f) + 1.0f) / 2.0f;

                if (k > 0.6f) {
                    addBlock(blockPos, std::make_shared<Block>(0, blockPos, glm::vec3(0.0f, 0.0f, 0.0f), false, glm::vec3(0.5f, 0.5f, 0.5f)));
                    checkHeight(chunkPos, blockPos);
                }
            }
        }
    }
    if (chunkPos.y < -4.0f) {
        generateOres(chunkPos, 4, 1, 3, -250, -35);
    }

    // if (HeightMap::getInstance().heightMaps.count(glm::vec2(chunkPos.x, chunkPos.z))) {
    //     for (auto& h : HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)]) {
    //         if (h.second.y - 1.0f < 0.0f) {
    //             if (Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z))) {
    //                 checkHeight(glm::vec3(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z), glm::vec3(h.second.x, 7.0f, h.second.z));
    //             }
    //         }
    //         else {
    //             checkHeight(chunkPos, glm::vec3(h.second.x, h.second.y - 1.0f, h.second.z));
    //         }
    //     }
    // }

    resetAmbient();
    checkAmbient(chunkPos);
}

void ServerChunkMap::resetLights() {
    for (auto& b : blocks) {
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        if (b->id != 0) {
            for (int i = 0; i < 6; i++) {
                b->lightLevels[i].x = 0;
            }
        }
    }
}

std::set<glm::vec3, vec3Comparator> ServerChunkMap::checkLights(glm::vec3 chunkPos, Block prevblock) {
    std::map<std::pair<glm::vec3, glm::vec3>, int, vec3PairComparator> lightResult;
    std::set<glm::vec3, vec3Comparator> affectedChunks;
    affectedChunks.insert(chunkPos);
    // int ch = 0;

    int lightIntensity = 5;
    std::map<std::pair<glm::vec3, glm::vec3>, int, vec3PairComparator> lightQueue;
    std::array<std::shared_ptr<Block>, 512> blockMap = blocks;
    int index = 0;
    for (auto& b : blockMap) {
        // ch++;
        // std::cout << ch << " ch" << std::endl;
        // std::cout << b.second->id << " id" << std::endl;
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        // if (b.second == nullptr) { continue; }
        if (b->id == 5) {
            glm::vec3 blockPos = b->position;
            lightQueue[std::make_pair(chunkPos, blockPos)] = lightIntensity;
            // printf("%f %f %f start\n", b.first.x, b.first.y, b.first.z);
        }
        if (b->position == prevblock.position && prevblock.id == 5) {
            glm::vec3 blockPos = b->position;
            lightQueue[std::make_pair(chunkPos, blockPos)] = -1;
            // printf("%f %f %f start\n", b.first.x, b.first.y, b.first.z);
        }
    }

    while (!lightQueue.empty() && lightIntensity > 1) {
        // std::cout << lightQueue.size() << std::endl;
        std::map<std::pair<glm::vec3, glm::vec3>, int, vec3PairComparator> tempQueue;
        for (auto& l : lightQueue) {
            // x
            // printf("%f %f %f chunk\n", l.first.first.x, l.first.first.y, l.first.first.z);
            // printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
            // if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
            //     printf("%f %f %f chunk %f %f %f block %d lvl\n", l.first.first.x, l.first.first.y, l.first.first.z, l.first.second.x, l.first.second.y, l.first.second.z, l.second);
            // }
            {
                float x = l.first.second.x - 1.0f;
                glm::vec3 lightPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (x < 0.0f) {
                    x = 7.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z)) && checkValidPos(glm::vec3(x, l.first.second.y, l.first.second.z))) {
                        lightPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                        lightChunk = glm::vec3(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z);
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                    block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                }
                if (block != nullptr && block->id == 0) {
                    std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                    if (!tempQueue.count(lightBlock)) {
                        tempQueue[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                    if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                        lightResult[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                }
            }
            {
                float x = l.first.second.x + 1.0f;
                glm::vec3 lightPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (x > 7.0f) {
                    x = 0.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z)) && checkValidPos(glm::vec3(x, l.first.second.y, l.first.second.z))) {
                        lightPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                        lightChunk = glm::vec3(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z);
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                }
                else if (checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                    block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                }
                if (block != nullptr && block->id == 0) {
                    std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                    if (!tempQueue.count(lightBlock)) {
                        tempQueue[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                    if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                        lightResult[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                }
            }
            // y
            {
                float y = l.first.second.y - 1.0f;
                glm::vec3 lightPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (y < 0.0f) {
                    y = 7.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z)) && checkValidPos(glm::vec3(l.first.second.x, y, l.first.second.z))) {
                        lightPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                        lightChunk = glm::vec3(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z);
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                }
                else if (checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                    block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                }
                if (block != nullptr && block->id == 0) {
                    std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                    if (!tempQueue.count(lightBlock)) {
                        tempQueue[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                    if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                        lightResult[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                }
            }
            {
                float y = l.first.second.y + 1.0f;
                glm::vec3 lightPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (y > 7.0f) {
                    y = 0.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z)) && checkValidPos(glm::vec3(l.first.second.x, y, l.first.second.z))) {
                        lightPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                        lightChunk = glm::vec3(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z);
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                }
                else if (checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                    block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                }
                if (block != nullptr && block->id == 0) {
                    std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                    if (!tempQueue.count(lightBlock)) {
                        tempQueue[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                    if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                        lightResult[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                }
            }
            // z
            {
                float z = l.first.second.z - 1.0f;
                glm::vec3 lightPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (z < 0.0f) {
                    z = 7.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f)) && checkValidPos(glm::vec3(l.first.second.x, l.first.second.y, z))) {
                        lightPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                        lightChunk = glm::vec3(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f);
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                }
                else if (checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                    block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                }
                // if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
                //     printf("%f %f %f chunk %f %f %f block %d lvl ZZZ\n", lightChunk.x, lightChunk.y, lightChunk.z, lightPos.x, lightPos.y, lightPos.z, l.second);
                //     printf("%d test1\n", (int)(block != nullptr));
                //     printf("%d test2\n", (int)(block->id == 0));
                // }
                if (block != nullptr && block->id == 0) {
                    std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                    if (!tempQueue.count(lightBlock)) {
                        tempQueue[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                    if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                        lightResult[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                }
            }
            {
                float z = l.first.second.z + 1.0f;
                glm::vec3 lightPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (z > 7.0f) {
                    z = 0.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f)) && checkValidPos(glm::vec3(l.first.second.x, l.first.second.y, z))) {
                        lightPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                        lightChunk = glm::vec3(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f);
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                }
                else if (checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                    block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                }
                if (block != nullptr && block->id == 0) {
                    std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                    if (!tempQueue.count(lightBlock)) {
                        tempQueue[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                    if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                        lightResult[lightBlock] = (l.second == -1 ? -1 : lightIntensity - 1);
                    }
                }
            }
        }
        lightIntensity -= 1;
        lightQueue = tempQueue;
    }

    // std::cout << "lightresult" << std::endl;
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
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x - 1.0f, r.first.second.y, r.first.second.z);
            if (resultBlock.x < 0.0f) {
                resultBlock.x = 7.0f;
                resultChunk.x -= 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            }
            if (block != nullptr && block->id != 0) {
                // std::cout << "front" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                if (block->lightLevels[3].x < r.second) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[3].x = r.second;
                }
                else if (r.second < 0) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[3].x = 0;
                }
            }
        }
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x + 1.0f, r.first.second.y, r.first.second.z);
            if (resultBlock.x > 7.0f) {
                resultBlock.x = 0.0f;
                resultChunk.x += 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            }
            if (block != nullptr && block->id != 0) {
                // std::cout << "back" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                if (block->lightLevels[2].x < r.second) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[2].x = r.second;
                }
                else if (r.second < 0) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[2].x = 0;
                }
            }
        }
        // y
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y - 1.0f, r.first.second.z);
            if (resultBlock.y < 0.0f) {
                resultBlock.y = 7.0f;
                resultChunk.y -= 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            }
            if (block != nullptr && block->id != 0) {
                // std::cout << "top" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                if (block->lightLevels[5].x < r.second) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[5].x = r.second;
                }
                else if (r.second < 0) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[5].x = 0;
                }
            }
        }
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y + 1.0f, r.first.second.z);
            if (resultBlock.y > 7.0f) {
                resultBlock.y = 0.0f;
                resultChunk.y += 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            }
            if (block != nullptr && block->id != 0) {
                // std::cout << "bottom" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                if (block->lightLevels[4].x < r.second) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[4].x = r.second;
                }
                else if (r.second < 0) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[4].x = 0;
                }
            }
        }
        // z
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y, r.first.second.z - 1.0f);
            if (resultBlock.z < 0.0f) {
                resultBlock.z = 7.0f;
                resultChunk.z -= 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[1].x < r.second)) {
                // std::cout << "right" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                if (block->lightLevels[1].x < r.second) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[1].x = r.second;
                }
                else if (r.second < 0) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[1].x = 0;
                }
            }
        }
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y, r.first.second.z + 1.0f);
            if (resultBlock.z > 7.0f) {
                resultBlock.z = 0.0f;
                resultChunk.z += 1.0f;
            }
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(resultBlock)) {
                block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            }
            if (block != nullptr && block->id != 0 && (block->lightLevels[0].x < r.second)) {
                // std::cout << "left" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                if (block->lightLevels[0].x < r.second) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[0].x = r.second;
                }
                else if (r.second < 0) {
                    affectedChunks.insert(resultChunk);
                    block->lightLevels[0].x = 0;
                }
            }
        }
    }

    return affectedChunks;
}

void ServerChunkMap::resetAmbient() {
    std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
    for (auto& b : blocks) {
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        if (b->id != 0) {
            for (int i = 0; i < 6; i++) {
                b->lightLevels[i].y = -5;
            }
        }
    }
}

std::set<glm::vec3, vec3Comparator> ServerChunkMap::checkAmbient(glm::vec3 chunkPos) {
    std::map<std::pair<glm::vec3, glm::vec3>, int, vec3PairComparator> darkResult;
    std::set<glm::vec3, vec3Comparator> affectedChunks;
    affectedChunks.insert(chunkPos);
    // int ch = 0;

    std::map<std::pair<glm::vec3, glm::vec3>, int, vec3PairComparator> darknessQueue;
    for (auto& a : HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)]) {
        if (a.first == chunkPos.y) {
            darknessQueue[std::pair(glm::vec3(chunkPos.x, a.first, chunkPos.z), glm::vec3(a.second.x, a.second.y, a.second.z))] = 0;
            darkResult[std::pair(glm::vec3(chunkPos.x, a.first, chunkPos.z), glm::vec3(a.second.x, a.second.y, a.second.z))] = 0;
        }
        if (a.first == chunkPos.y + 1.0f && a.second.y == 0.0f) {
            darknessQueue[std::pair(glm::vec3(chunkPos.x, a.first, chunkPos.z), glm::vec3(a.second.x, a.second.y, a.second.z))] = 0;
            darkResult[std::pair(glm::vec3(chunkPos.x, a.first, chunkPos.z), glm::vec3(a.second.x, a.second.y, a.second.z))] = 0;
        }
    }

    // ch++;
    // std::cout << ch << " ch" << std::endl;
    // std::cout << b.second->id << " id" << std::endl;
    // if (b.second == nullptr) { continue; }
    int darknessLevel = 0;
    // if (chunkPos.x == 0.0f && chunkPos.y == 0.0f && chunkPos.z == 0.0f) {
    //     printf("%f %f %f start\n", a.x, a.y, a.z);
    // }
    while (!darknessQueue.empty() && darknessLevel > -5) {
        // std::cout << darknessQueue.size() << " dqueue" << std::endl;
        std::map<std::pair<glm::vec3, glm::vec3>, int, vec3PairComparator> tempQueue;
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
                glm::vec3 darkPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                glm::vec3 darkChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (x < 0.0f) {
                    x = 7.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(darkChunk.x - 1.0f, darkChunk.y, darkChunk.z)) && checkValidPos(glm::vec3(x, l.first.second.y, l.first.second.z))) {
                        darkPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                        darkChunk = glm::vec3(darkChunk.x - 1.0f, darkChunk.y, darkChunk.z);
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(darkPos.x, darkPos.y, darkPos.z))) {
                    if (darkChunk == chunkPos) {
                        block = getBlock(darkPos);
                    }
                    else if (Server::getInstance().chunks.count(darkChunk)) {
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                if (block != nullptr && block->id == 0 && HeightMap::getInstance().heightMaps.count(glm::vec2(darkChunk.x, darkChunk.z))) {
                    auto it = std::find(HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).begin(), HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end(), std::pair(darkChunk.y, darkPos));
                    std::pair<glm::vec3, glm::vec3> darkBlock = std::make_pair(darkChunk, darkPos);
                    if ((!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) && it == HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end()) {
                        tempQueue[darkBlock] = darknessLevel - 1;
                        darkResult[darkBlock] = darknessLevel - 1;
                    }
                }
            }
            {
                float x = l.first.second.x + 1.0f;
                glm::vec3 darkPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                glm::vec3 darkChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (x > 7.0f) {
                    x = 0.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(darkChunk.x + 1.0f, darkChunk.y, darkChunk.z)) && checkValidPos(glm::vec3(x, l.first.second.y, l.first.second.z))) {
                        darkPos = glm::vec3(x, l.first.second.y, l.first.second.z);
                        darkChunk = glm::vec3(darkChunk.x + 1.0f, darkChunk.y, darkChunk.z);
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(darkPos.x, darkPos.y, darkPos.z))) {
                    if (darkChunk == chunkPos) {
                        block = getBlock(darkPos);
                    }
                    else if (Server::getInstance().chunks.count(darkChunk)) {
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                if (block != nullptr && block->id == 0 && HeightMap::getInstance().heightMaps.count(glm::vec2(darkChunk.x, darkChunk.z))) {
                    auto it = std::find(HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).begin(), HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end(), std::pair(darkChunk.y, darkPos));
                    std::pair<glm::vec3, glm::vec3> darkBlock = std::make_pair(darkChunk, darkPos);
                    if ((!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) && it == HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end()) {
                        tempQueue[darkBlock] = darknessLevel - 1;
                        darkResult[darkBlock] = darknessLevel - 1;
                    }
                }
            }
            // y
            {
                float y = l.first.second.y - 1.0f;
                glm::vec3 darkPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                glm::vec3 darkChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (y < 0.0f) {
                    y = 7.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(darkChunk.x, darkChunk.y - 1.0f, darkChunk.z)) && checkValidPos(glm::vec3(l.first.second.x, y, l.first.second.z))) {
                        darkPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                        darkChunk = glm::vec3(darkChunk.x, darkChunk.y - 1.0f, darkChunk.z);
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(darkPos.x, darkPos.y, darkPos.z))) {
                    if (darkChunk == chunkPos) {
                        block = getBlock(darkPos);
                    }
                    else if (Server::getInstance().chunks.count(darkChunk)) {
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                if (block != nullptr && block->id == 0 && HeightMap::getInstance().heightMaps.count(glm::vec2(darkChunk.x, darkChunk.z))) {
                    auto it = std::find(HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).begin(), HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end(), std::pair(darkChunk.y, darkPos));
                    std::pair<glm::vec3, glm::vec3> darkBlock = std::make_pair(darkChunk, darkPos);
                    if ((!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) && it == HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end()) {
                        tempQueue[darkBlock] = darknessLevel - 1;
                        darkResult[darkBlock] = darknessLevel - 1;
                    }
                }
            }
            {
                float y = l.first.second.y + 1.0f;
                glm::vec3 darkPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                glm::vec3 darkChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (y > 7.0f) {
                    y = 0.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(darkChunk.x, darkChunk.y + 1.0f, darkChunk.z)) && checkValidPos(glm::vec3(l.first.second.x, y, l.first.second.z))) {
                        darkPos = glm::vec3(l.first.second.x, y, l.first.second.z);
                        darkChunk = glm::vec3(darkChunk.x, darkChunk.y + 1.0f, darkChunk.z);
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(darkPos.x, darkPos.y, darkPos.z))) {
                    if (darkChunk == chunkPos) {
                        block = getBlock(darkPos);
                    }
                    else if (Server::getInstance().chunks.count(darkChunk)) {
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                if (block != nullptr && block->id == 0 && HeightMap::getInstance().heightMaps.count(glm::vec2(darkChunk.x, darkChunk.z))) {
                    auto it = std::find(HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).begin(), HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end(), std::pair(darkChunk.y, darkPos));
                    std::pair<glm::vec3, glm::vec3> darkBlock = std::make_pair(darkChunk, darkPos);
                    if ((!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) && it == HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end()) {
                        tempQueue[darkBlock] = darknessLevel - 1;
                        darkResult[darkBlock] = darknessLevel - 1;
                    }
                }
            }
            // z
            {
                float z = l.first.second.z - 1.0f;
                glm::vec3 darkPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                glm::vec3 darkChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (z < 0.0f) {
                    z = 7.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(darkChunk.x, darkChunk.y, darkChunk.z - 1.0f)) && checkValidPos(glm::vec3(l.first.second.x, l.first.second.y, z))) {
                        darkPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                        darkChunk = glm::vec3(darkChunk.x, darkChunk.y, darkChunk.z - 1.0f);
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(darkPos.x, darkPos.y, darkPos.z))) {
                    if (darkChunk == chunkPos) {
                        block = getBlock(darkPos);
                    }
                    else if (Server::getInstance().chunks.count(darkChunk)) {
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                if (block != nullptr && block->id == 0 && HeightMap::getInstance().heightMaps.count(glm::vec2(darkChunk.x, darkChunk.z))) {
                    auto it = std::find(HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).begin(), HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end(), std::pair(darkChunk.y, darkPos));
                    std::pair<glm::vec3, glm::vec3> darkBlock = std::make_pair(darkChunk, darkPos);
                    if ((!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) && it == HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end()) {
                        tempQueue[darkBlock] = darknessLevel - 1;
                        darkResult[darkBlock] = darknessLevel - 1;
                    }
                }
            }
            {
                float z = l.first.second.z + 1.0f;
                glm::vec3 darkPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                glm::vec3 darkChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;
                if (z > 7.0f) {
                    z = 0.0f;
                    if (Server::getInstance().chunks.count(glm::vec3(darkChunk.x, darkChunk.y, darkChunk.z + 1.0f)) && checkValidPos(glm::vec3(l.first.second.x, l.first.second.y, z))) {
                        darkPos = glm::vec3(l.first.second.x, l.first.second.y, z);
                        darkChunk = glm::vec3(darkChunk.x, darkChunk.y, darkChunk.z + 1.0f);
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                // if ((l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) && !checkValidPos(glm::vec3(lightPos.x, lightPos.y, lightPos.z))) {
                //     printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                // }
                else if (checkValidPos(glm::vec3(darkPos.x, darkPos.y, darkPos.z))) {
                    if (darkChunk == chunkPos) {
                        block = getBlock(darkPos);
                    }
                    else if (Server::getInstance().chunks.count(darkChunk)) {
                        block = Server::getInstance().chunks[darkChunk]->getBlock(darkPos);
                    }
                }
                if (block != nullptr && block->id == 0 && HeightMap::getInstance().heightMaps.count(glm::vec2(darkChunk.x, darkChunk.z))) {
                    auto it = std::find(HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).begin(), HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end(), std::pair(darkChunk.y, darkPos));
                    std::pair<glm::vec3, glm::vec3> darkBlock = std::make_pair(darkChunk, darkPos);
                    if ((!darkResult.count(darkBlock) || darkResult[darkBlock] < darknessLevel - 1) && it == HeightMap::getInstance().heightMaps.at(glm::vec2(darkChunk.x, darkChunk.z)).end()) {
                        tempQueue[darkBlock] = darknessLevel - 1;
                        darkResult[darkBlock] = darknessLevel - 1;
                    }
                }
            }
        }
        darknessLevel -= 1;
        darknessQueue = tempQueue;
    }

    // std::cout << "kekw" << std::endl;
    // std::cout << darkResult.size() << std::endl;
    for (auto& r : darkResult) {
        // x
        // printf("%d dark\n", r.second);
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x - 1.0f, r.first.second.y, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.x < 0.0f) {
                resultBlock.x = 7.0f;
                resultChunk.x -= 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            else if (checkValidPos(glm::vec3(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = getBlock(resultBlock);
                }
                else if (Server::getInstance().chunks.count(resultChunk)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0) {
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[3].y < r.second) {
                // std::cout << "front" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                block->lightLevels[3].y = r.second;
            }
        }
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x + 1.0f, r.first.second.y, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.x > 7.0f) {
                resultBlock.x = 0.0f;
                resultChunk.x += 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            else if (checkValidPos(glm::vec3(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = getBlock(resultBlock);
                }
                else if (Server::getInstance().chunks.count(resultChunk)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0) {
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[2].y < r.second) {
                // std::cout << "back" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                block->lightLevels[2].y = r.second;
            }
        }
        // y
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y - 1.0f, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.y < 0.0f) {
                resultBlock.y = 7.0f;
                resultChunk.y -= 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            else if (checkValidPos(glm::vec3(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = getBlock(resultBlock);
                }
                else if (Server::getInstance().chunks.count(resultChunk)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            // if (chunkPos.x == 0.0f && chunkPos.y == 0.0f && chunkPos.z == 0.0f && resultBlock.x == 0.0f && resultBlock.z == 0.0f) {
            //     printf("%f %f %f block %d lvl\n", resultBlock.x, resultBlock.y, resultBlock.z, r.second);
            //     printf("%d %d %d checks\n", (int)(block != nullptr), (int)(block->id != 0), (int)(block->lightLevels[5].y < r.second));
            // }
            if (block != nullptr && block->id != 0) {
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[5].y < r.second) {
                // std::cout << "top" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                block->lightLevels[5].y = r.second;
            }
        }
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y + 1.0f, r.first.second.z);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.y > 7.0f) {
                resultBlock.y = 0.0f;
                resultChunk.y += 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            else if (checkValidPos(glm::vec3(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = getBlock(resultBlock);
                }
                else if (Server::getInstance().chunks.count(resultChunk)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0) {
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[4].y < r.second) {
                // std::cout << "bottom" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                block->lightLevels[4].y = r.second;
            }
        }
        // z
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y, r.first.second.z - 1.0f);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.z < 0.0f) {
                resultBlock.z = 7.0f;
                resultChunk.z -= 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            else if (checkValidPos(glm::vec3(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = getBlock(resultBlock);
                }
                else if (Server::getInstance().chunks.count(resultChunk)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0) {
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[1].y < r.second) {
                // std::cout << "right" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                block->lightLevels[1].y = r.second;
            }
        }
        {
            glm::vec3 resultChunk = r.first.first;
            glm::vec3 resultBlock = glm::vec3(r.first.second.x, r.first.second.y, r.first.second.z + 1.0f);
            std::shared_ptr<Block> block = nullptr;
            if (resultBlock.z > 7.0f) {
                resultBlock.z = 0.0f;
                resultChunk.z += 1.0f;
                if (Server::getInstance().chunks.count(resultChunk) && checkValidPos(resultBlock)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            else if (checkValidPos(glm::vec3(resultBlock.x, resultBlock.y, resultBlock.z))) {
                if (resultChunk == chunkPos) {
                    block = getBlock(resultBlock);
                }
                else if (Server::getInstance().chunks.count(resultChunk)) {
                    block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
                }
            }
            if (block != nullptr && block->id != 0) {
                affectedChunks.insert(resultChunk);
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[0].y < r.second) {
                // std::cout << "left" << std::endl;
                std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
                block->lightLevels[0].y = r.second;
            }
        }
    }
    return affectedChunks;
}

std::set<glm::vec3, vec3Comparator> ServerChunkMap::checkHeight(glm::vec3 chunkPos, glm::vec3 blockPos) {
    std::set<glm::vec3, vec3Comparator> affectedChunks;
    affectedChunks.insert(chunkPos);

    if (blockPos.x == 0.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x - 1.0f, chunkPos.y, chunkPos.z))) {
        affectedChunks.insert(glm::vec3(chunkPos.x - 1.0f, chunkPos.y, chunkPos.z));
    }
    if (blockPos.x == 7.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x + 1.0f, chunkPos.y, chunkPos.z))) {
        affectedChunks.insert(glm::vec3(chunkPos.x + 1.0f, chunkPos.y, chunkPos.z));
    }

    if (blockPos.y == 0.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z))) {
        affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z));
    }
    if (blockPos.y == 7.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y + 1.0f, chunkPos.z))) {
        affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y + 1.0f, chunkPos.z));
    }

    if (blockPos.z == 0.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z - 1.0f))) {
        affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z - 1.0f));
    }
    if (blockPos.z == 7.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z + 1.0f))) {
        affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z + 1.0f));
    }

    if (!HeightMap::getInstance().heightMaps.count(glm::vec2(chunkPos.x, chunkPos.z))) { return affectedChunks; }

    auto res = std::find_if(HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)].begin(), HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)].end(), [&](auto& heightblock) {
        if (heightblock.second.x == blockPos.x && heightblock.second.z == blockPos.z) { return true; } return false;
    });
    if (res == HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)].end()) { return affectedChunks; }

    // printf("%f %f %f chunk\n", chunkPos.x, chunkPos.z, chunkPos.z);

    if (res->second.y + (res->first * 8.0f) <= blockPos.y + (chunkPos.y * 8.0f) && getBlock(blockPos)->id != 0) {
        float prevheight = res->first;
        if (blockPos.y + 1.0f > 7.0f) {
            res->first = chunkPos.y + 1.0f;
            res->second = glm::vec3(blockPos.x, 0.0f, blockPos.z);
        }
        else {
            res->first = chunkPos.y;
            res->second = glm::vec3(blockPos.x, blockPos.y + 1.0f, blockPos.z);
        }
        affectedChunks.insert(glm::vec3(chunkPos.x, prevheight, chunkPos.z));
        affectedChunks.insert(glm::vec3(chunkPos.x, res->first, chunkPos.z));
        return affectedChunks;
    }
    if (res->second.y + (res->first * 8.0f) > blockPos.y + (chunkPos.y * 8.0f) && getBlock(blockPos)->id == 0) {
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
            glm::vec3 solidPos = glm::vec3(res->second.x, solidHeight, res->second.z);
            glm::vec3 solidChunk = glm::vec3(chunkPos.x, chunkHeight, chunkPos.z);

            if ((solidChunk.y == chunkPos.y && (getBlock(solidPos)->id != 0)) || (solidChunk.y != chunkPos.y && !Server::getInstance().chunks.count(solidChunk)) || (Server::getInstance().chunks.count(solidChunk) && Server::getInstance().chunks[solidChunk]->getBlock(solidPos)->id != 0)) {
                if (solidPos.y + 1.0f > 7.0f) {
                    solidChunk.y += 1.0f;
                    solidPos.y = 0.0f;
                }
                else {
                    solidPos.y += 1.0f;
                }
                res->first = solidChunk.y;
                res->second = glm::vec3(solidPos.x, solidPos.y, solidPos.z);
                affectedChunks.insert(glm::vec3(chunkPos.x, prevheight, chunkPos.z));
                affectedChunks.insert(glm::vec3(chunkPos.x, res->first, chunkPos.z));
                return affectedChunks;
            }
        }
    }
    return affectedChunks;
}