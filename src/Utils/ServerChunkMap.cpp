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
    HeightMap::getInstance().addMap(glm::vec2(chunkPos.x, chunkPos.z));

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
                        HeightMap::getInstance().addSource(glm::vec2(chunkPos.x, chunkPos.z), height, ambientPos);
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

std::set<glm::vec3, vec3Comparator> ServerChunkMap::simulateLightSource(glm::vec3 chunkPos, Block source, bool collision, int setarrays) {
    std::set<glm::vec3, vec3Comparator> affectedChunks;
    affectedChunks.insert(chunkPos);

    int lightIntensity = 11;
    std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> lightQueue;
    lightQueue[std::make_pair(chunkPos, source.position)] = lightIntensity;

    while (!lightQueue.empty() && lightIntensity > 1) {
        std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> tempQueue;
        for (auto& l : lightQueue) {
            for (int side = 0; side < 6; side++) {
                int direction[6] = {0, 0, 0, 0, 0, 0};
                direction[side] = 1;
                {
                    glm::vec3 lightPos = glm::vec3(l.first.second.x + (float)direction[0] - (float)direction[1], l.first.second.y + (float)direction[2] - (float)direction[3], l.first.second.z + (float)direction[4] - (float)direction[5]);
                    glm::vec3 lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    bool swappedChunk = false;
                    if (lightPos.x < 0.0f) { lightPos.x = 7.0f; lightChunk.x -= 1.0f; swappedChunk = true; }
                    if (lightPos.x > 7.0f) { lightPos.x = 0.0f; lightChunk.x += 1.0f; swappedChunk = true; }
                    if (lightPos.y < 0.0f) { lightPos.y = 7.0f; lightChunk.y -= 1.0f; swappedChunk = true; }
                    if (lightPos.y > 7.0f) { lightPos.y = 0.0f; lightChunk.y += 1.0f; swappedChunk = true; }
                    if (lightPos.z < 0.0f) { lightPos.z = 7.0f; lightChunk.z -= 1.0f; swappedChunk = true; }
                    if (lightPos.z > 7.0f) { lightPos.z = 0.0f; lightChunk.z += 1.0f; swappedChunk = true; }

                    if (swappedChunk && Server::getInstance().chunks.count(lightChunk)) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        affectedChunks.insert(lightChunk);
                        if (setarrays != NONE) {
                            if (setarrays == ADD) { Server::getInstance().chunks[lightChunk]->lightSources[std::make_pair(chunkPos, source.position)] = 10; }
                            else if (setarrays == REMOVE) { Server::getInstance().chunks[lightChunk]->lightSources.erase(std::make_pair(chunkPos, source.position)); }
                        }
                    }
                    if (!swappedChunk) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && (block->id == 0 || !collision)) {
                        std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                        tempQueue[lightBlock] = lightIntensity - 1;
                    }
                }
            }
        }
        lightIntensity -= 1;
        lightQueue = tempQueue;
    }
    return affectedChunks;
}

void ServerChunkMap::checkLights(glm::vec3 chunkPos) {
    std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> lightResult;

    int lightIntensity = 10;
    std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> lightQueue;

    for (auto& l : lightSources) {
        lightQueue[l.first] = l.second;
    }

    while (!lightQueue.empty() && lightIntensity > 1) {
        std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> tempQueue;
        for (auto& l : lightQueue) {
            for (int side = 0; side < 6; side++) {
                int direction[6] = {0, 0, 0, 0, 0, 0};
                direction[side] = 1;
                {
                    glm::vec3 lightPos = glm::vec3(l.first.second.x + (float)direction[0] - (float)direction[1], l.first.second.y + (float)direction[2] - (float)direction[3], l.first.second.z + (float)direction[4] - (float)direction[5]);
                    glm::vec3 lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    bool swappedChunk = false;
                    if (lightPos.x < 0.0f) { lightPos.x = 7.0f; lightChunk.x -= 1.0f; swappedChunk = true; }
                    if (lightPos.x > 7.0f) { lightPos.x = 0.0f; lightChunk.x += 1.0f; swappedChunk = true; }
                    if (lightPos.y < 0.0f) { lightPos.y = 7.0f; lightChunk.y -= 1.0f; swappedChunk = true; }
                    if (lightPos.y > 7.0f) { lightPos.y = 0.0f; lightChunk.y += 1.0f; swappedChunk = true; }
                    if (lightPos.z < 0.0f) { lightPos.z = 7.0f; lightChunk.z -= 1.0f; swappedChunk = true; }
                    if (lightPos.z > 7.0f) { lightPos.z = 0.0f; lightChunk.z += 1.0f; swappedChunk = true; }

                    if (swappedChunk && Server::getInstance().chunks.count(lightChunk)) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (!swappedChunk) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                            lightResult[lightBlock] = lightIntensity - 1;
                        }
                    }
                }
            }
        }
        lightIntensity -= 1;
        lightQueue = tempQueue;
    }

    for (auto& l : lightResult) {
        for (int side = 0; side < 6; side++) {
            int direction[6] = {0, 0, 0, 0, 0, 0};
            int blockside[6] = {2, 3, 4, 5, 0, 1};
            direction[side] = 1;
            {
                glm::vec3 lightPos = glm::vec3(l.first.second.x + (float)direction[0] - (float)direction[1], l.first.second.y + (float)direction[2] - (float)direction[3], l.first.second.z + (float)direction[4] - (float)direction[5]);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;

                if (lightPos.x < 0.0f) { lightPos.x = 7.0f; lightChunk.x -= 1.0f; }
                if (lightPos.x > 7.0f) { lightPos.x = 0.0f; lightChunk.x += 1.0f; }
                if (lightPos.y < 0.0f) { lightPos.y = 7.0f; lightChunk.y -= 1.0f; }
                if (lightPos.y > 7.0f) { lightPos.y = 0.0f; lightChunk.y += 1.0f; }
                if (lightPos.z < 0.0f) { lightPos.z = 7.0f; lightChunk.z -= 1.0f; }
                if (lightPos.z > 7.0f) { lightPos.z = 0.0f; lightChunk.z += 1.0f; }

                if (chunkPos == lightChunk && checkValidPos(lightPos)) {
                    block = getBlock(lightPos);
                }

                if (block != nullptr && block->id != 0) {
                    block->lightLevels[blockside[side]].x = l.second;
                }
            }
        }
    }
}

void ServerChunkMap::resetAmbient() {
    for (auto& b : blocks) {
        // printf("%f %f %f block\n", b.first.x, b.first.y, b.first.z);
        if (b->id != 0) {
            for (int i = 0; i < 6; i++) {
                b->lightLevels[i].y = -10;
            }
        }
    }
}

std::set<glm::vec3, vec3Comparator> ServerChunkMap::simulateAmbientSource(glm::vec3 chunkPos, AbsPos source, bool collision, int setarrays) {
    std::set<glm::vec3, vec3Comparator> affectedChunks;
    affectedChunks.insert(chunkPos);

    int lightIntensity = 0;
    std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> lightQueue;
    lightQueue[std::make_pair(source.first, source.second)] = lightIntensity;

    while (!lightQueue.empty() && lightIntensity > -11) {
        std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> tempQueue;
        for (auto& l : lightQueue) {
            for (int side = 0; side < 6; side++) {
                int direction[6] = {0, 0, 0, 0, 0, 0};
                direction[side] = 1;
                {
                    glm::vec3 lightPos = glm::vec3(l.first.second.x + (float)direction[0] - (float)direction[1], l.first.second.y + (float)direction[2] - (float)direction[3], l.first.second.z + (float)direction[4] - (float)direction[5]);
                    glm::vec3 lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    bool swappedChunk = false;
                    if (lightPos.x < 0.0f) { lightPos.x = 7.0f; lightChunk.x -= 1.0f; swappedChunk = true; }
                    if (lightPos.x > 7.0f) { lightPos.x = 0.0f; lightChunk.x += 1.0f; swappedChunk = true; }
                    if (lightPos.y < 0.0f) { lightPos.y = 7.0f; lightChunk.y -= 1.0f; swappedChunk = true; }
                    if (lightPos.y > 7.0f) { lightPos.y = 0.0f; lightChunk.y += 1.0f; swappedChunk = true; }
                    if (lightPos.z < 0.0f) { lightPos.z = 7.0f; lightChunk.z -= 1.0f; swappedChunk = true; }
                    if (lightPos.z > 7.0f) { lightPos.z = 0.0f; lightChunk.z += 1.0f; swappedChunk = true; }

                    if (swappedChunk && Server::getInstance().chunks.count(lightChunk)) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        affectedChunks.insert(lightChunk);
                        if (setarrays != NONE) {
                            if (setarrays == ADD) { Server::getInstance().chunks[lightChunk]->ambientSources[std::make_pair(source.first, source.second)] = 0; }
                            else if (setarrays == REMOVE) { Server::getInstance().chunks[lightChunk]->ambientSources.erase(std::make_pair(source.first, source.second)); }
                        }
                    }
                    if (!swappedChunk) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (block != nullptr && (block->id == 0 || !collision)) {
                        std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                        tempQueue[lightBlock] = lightIntensity - 1;
                    }
                }
            }
        }
        lightIntensity -= 1;
        lightQueue = tempQueue;
    }
    return affectedChunks;
}

void ServerChunkMap::checkAmbient(glm::vec3 chunkPos) {
    std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> lightResult;

    int lightIntensity = 0;
    std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> lightQueue;

    for (auto& l : ambientSources) {
        lightQueue[l.first] = l.second;
    }

    for (auto& a : HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)]) {
        if (a.first == chunkPos.y || (a.first == chunkPos.y + 1.0f && a.second.y == 0.0f)) {
            lightQueue[std::make_pair(glm::vec3(chunkPos.x, a.first, chunkPos.z), glm::vec3(a.second.x, a.second.y, a.second.z))] = 0;
            lightResult[std::make_pair(glm::vec3(chunkPos.x, a.first, chunkPos.z), glm::vec3(a.second.x, a.second.y, a.second.z))] = 0;
        }
    }

    while (!lightQueue.empty() && lightIntensity > -10) {
        std::unordered_map<AbsPos, int, vec3PairHash<float>, vec3PairEquals> tempQueue;
        for (auto& l : lightQueue) {
            for (int side = 0; side < 6; side++) {
                int direction[6] = {0, 0, 0, 0, 0, 0};
                direction[side] = 1;
                {
                    glm::vec3 lightPos = glm::vec3(l.first.second.x + (float)direction[0] - (float)direction[1], l.first.second.y + (float)direction[2] - (float)direction[3], l.first.second.z + (float)direction[4] - (float)direction[5]);
                    glm::vec3 lightChunk = l.first.first;
                    std::shared_ptr<Block> block = nullptr;
                    bool swappedChunk = false;
                    if (lightPos.x < 0.0f) { lightPos.x = 7.0f; lightChunk.x -= 1.0f; swappedChunk = true; }
                    if (lightPos.x > 7.0f) { lightPos.x = 0.0f; lightChunk.x += 1.0f; swappedChunk = true; }
                    if (lightPos.y < 0.0f) { lightPos.y = 7.0f; lightChunk.y -= 1.0f; swappedChunk = true; }
                    if (lightPos.y > 7.0f) { lightPos.y = 0.0f; lightChunk.y += 1.0f; swappedChunk = true; }
                    if (lightPos.z < 0.0f) { lightPos.z = 7.0f; lightChunk.z -= 1.0f; swappedChunk = true; }
                    if (lightPos.z > 7.0f) { lightPos.z = 0.0f; lightChunk.z += 1.0f; swappedChunk = true; }

                    // printf("%f %f %f chunk %lu\n", lightChunk.x, lightChunk.y, lightChunk.z, Server::getInstance().chunks.count(lightChunk));

                    if (swappedChunk && Server::getInstance().chunks.count(lightChunk)) {
                        block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                    }
                    if (!swappedChunk) {
                        if (lightChunk != chunkPos && Server::getInstance().chunks.count(lightChunk)) {
                            block = Server::getInstance().chunks[lightChunk]->getBlock(lightPos);
                        }
                        else {
                            block = getBlock(lightPos);
                        }
                    }
                    if (block != nullptr && block->id == 0) {
                        std::pair<glm::vec3, glm::vec3> lightBlock = std::make_pair(lightChunk, lightPos);
                        if (!lightResult.count(lightBlock) || lightResult[lightBlock] < lightIntensity - 1) {
                            tempQueue[lightBlock] = lightIntensity - 1;
                            lightResult[lightBlock] = lightIntensity - 1;
                        }
                    }
                }
            }
        }
        lightIntensity -= 1;
        lightQueue = tempQueue;
    }

    for (auto& l : lightResult) {
        for (int side = 0; side < 6; side++) {
            int direction[6] = {0, 0, 0, 0, 0, 0};
            int blockside[6] = {2, 3, 4, 5, 0, 1};
            direction[side] = 1;
            {
                glm::vec3 lightPos = glm::vec3(l.first.second.x + (float)direction[0] - (float)direction[1], l.first.second.y + (float)direction[2] - (float)direction[3], l.first.second.z + (float)direction[4] - (float)direction[5]);
                glm::vec3 lightChunk = l.first.first;
                std::shared_ptr<Block> block = nullptr;

                if (lightPos.x < 0.0f) { lightPos.x = 7.0f; lightChunk.x -= 1.0f; }
                if (lightPos.x > 7.0f) { lightPos.x = 0.0f; lightChunk.x += 1.0f; }
                if (lightPos.y < 0.0f) { lightPos.y = 7.0f; lightChunk.y -= 1.0f; }
                if (lightPos.y > 7.0f) { lightPos.y = 0.0f; lightChunk.y += 1.0f; }
                if (lightPos.z < 0.0f) { lightPos.z = 7.0f; lightChunk.z -= 1.0f; }
                if (lightPos.z > 7.0f) { lightPos.z = 0.0f; lightChunk.z += 1.0f; }

                if (chunkPos == lightChunk && checkValidPos(lightPos)) {
                    block = getBlock(lightPos);
                }

                if (block != nullptr && block->id != 0) {
                    block->lightLevels[blockside[side]].y = l.second;
                }
            }
        }
    }
}

std::pair<std::set<glm::vec3, vec3Comparator>, std::pair<AbsPos, AbsPos>> ServerChunkMap::checkHeight(glm::vec3 chunkPos, glm::vec3 blockPos) {
    std::set<glm::vec3, vec3Comparator> affectedChunks;
    affectedChunks.insert(chunkPos);

    // if (blockPos.x == 0.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x - 1.0f, chunkPos.y, chunkPos.z))) {
    //     affectedChunks.insert(glm::vec3(chunkPos.x - 1.0f, chunkPos.y, chunkPos.z));
    // }
    // if (blockPos.x == 7.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x + 1.0f, chunkPos.y, chunkPos.z))) {
    //     affectedChunks.insert(glm::vec3(chunkPos.x + 1.0f, chunkPos.y, chunkPos.z));
    // }
    //
    // if (blockPos.y == 0.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z))) {
    //     affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y - 1.0f, chunkPos.z));
    // }
    // if (blockPos.y == 7.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y + 1.0f, chunkPos.z))) {
    //     affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y + 1.0f, chunkPos.z));
    // }
    //
    // if (blockPos.z == 0.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z - 1.0f))) {
    //     affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z - 1.0f));
    // }
    // if (blockPos.z == 7.0f && Server::getInstance().chunks.count(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z + 1.0f))) {
    //     affectedChunks.insert(glm::vec3(chunkPos.x, chunkPos.y, chunkPos.z + 1.0f));
    // }

    std::pair<AbsPos, AbsPos> noChanges = std::make_pair(std::make_pair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, -1.0f, -1.0f)), std::make_pair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, -1.0f, -1.0f)));

    if (!HeightMap::getInstance().heightMaps.count(glm::vec2(chunkPos.x, chunkPos.z))) { return std::make_pair(affectedChunks, noChanges); }

    auto res = std::find_if(HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)].begin(), HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)].end(), [&](auto& heightblock) {
        if (heightblock.second.x == blockPos.x && heightblock.second.z == blockPos.z) { return true; } return false;
    });
    if (res == HeightMap::getInstance().heightMaps[glm::vec2(chunkPos.x, chunkPos.z)].end()) { return std::make_pair(affectedChunks, noChanges); }

    // printf("%f %f %f chunk %f %f %f block !!!\n", chunkPos.x, chunkPos.y, chunkPos.z, blockPos.x, blockPos.y, blockPos.z);
    auto prevblock = std::make_pair(glm::vec3(chunkPos.x, res->first, chunkPos.z), res->second);
    // printf("%f %f %f chunk %f %f %f block ???\n", prevblock.first.x, prevblock.first.y, prevblock.first.z, prevblock.second.x, prevblock.second.y, prevblock.second.z);
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
        auto currentblock = std::make_pair(glm::vec3(chunkPos.x, res->first, chunkPos.z), res->second);
        return std::make_pair(affectedChunks, std::make_pair(currentblock, prevblock));
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
                auto currentblock = std::make_pair(glm::vec3(chunkPos.x, res->first, chunkPos.z), res->second);
                return std::make_pair(affectedChunks, std::make_pair(currentblock, prevblock));
            }
        }
    }
    return std::make_pair(affectedChunks, noChanges);
}