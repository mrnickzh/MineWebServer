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
                        Vec3<float> ambientPos = Vec3<float>((float)x, (float)y + 1, (float)z);
                        float height = chunkPos.y;
                        if (ambientPos.y > 7.0f) {
                            height += 1;
                        }
                        HeightMap::getInstance().addMap(Vec2<float>(chunkPos.x, chunkPos.z),  height, ambientPos);
                    }
                    if (chunkPos.y * 8 + (float)y == std::floor(maxy)) {
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

                int wx = (int)chunkPos.x * 8 + x;
                int wy = (int)chunkPos.y * 8 + y;
                int wz = (int)chunkPos.z * 8 + z;

                float k = (Server::getInstance().seedMap->perlinNoiseCaves->generate3D((float)wx, (float)wy, (float)wz, 0.02f) + 1.0f) / 2.0f;

                if (k > 0.6f) blocks[blockPos] = std::make_shared<Block>(0, blockPos);
            }
        }
    }
    if (chunkPos.y < -4.0f) {
        generateOres(chunkPos, 4, 1, 3, -250, -35);
    }


    checkAmbient(chunkPos);
}

std::set<Vec3<float>> ServerChunkMap::checkLights(Vec3<float> chunkPos) {
    std::map<std::pair<Vec3<float>, Vec3<float>>, int> lightResult;
    std::set<Vec3<float>> affectedChunks;
    affectedChunks.insert(chunkPos);
    // int ch = 0;

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
            std::deque<std::pair<std::pair<Vec3<float>, Vec3<float>>, int>> lightQueue;
            lightQueue.push_back(std::make_pair(std::make_pair(chunkPos, blockPos), lightIntensity));
            while (!lightQueue.empty() && lightIntensity > 1) {
                std::cout << lightQueue.size() << std::endl;
                std::deque<std::pair<std::pair<Vec3<float>, Vec3<float>>, int>> tempQueue;
                for (auto& l : lightQueue) {
                    // x
                    // printf("%f %f %f chunk\n", l.first.first.x, l.first.first.y, l.first.first.z);
                    // printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                    if (l.first.first.x == 0.0f && l.first.first.y == 0.0f && l.first.first.z == -1.0f) {
                        printf("%f %f %f chunk %f %f %f block %d lvl\n", l.first.first.x, l.first.first.y, l.first.first.z, l.first.second.x, l.first.second.y, l.first.second.z, l.second);
                    }
                    {
                        float x = l.first.second.x - 1.0f;
                        Vec3<float> lightPos = Vec3<float>(x, l.first.second.y, l.first.second.z);
                        Vec3<float> lightChunk = l.first.first;
                        std::shared_ptr<Block> block = nullptr;
                        if (x < 0.0f) {
                            x = 7.0f;
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x - 1.0f, lightChunk.y, lightChunk.z))) {
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
                            tempQueue.push_back(std::make_pair(lightBlock, lightIntensity - 1));
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
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x + 1.0f, lightChunk.y, lightChunk.z))) {
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
                            tempQueue.push_back(std::make_pair(lightBlock, lightIntensity - 1));
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
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y - 1.0f, lightChunk.z))) {
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
                            tempQueue.push_back(std::make_pair(lightBlock, lightIntensity - 1));
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
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y + 1.0f, lightChunk.z))) {
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
                            tempQueue.push_back(std::make_pair(lightBlock, lightIntensity - 1));
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
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z - 1.0f))) {
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
                            tempQueue.push_back(std::make_pair(lightBlock, lightIntensity - 1));
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
                            if (Server::getInstance().chunks.count(Vec3<float>(lightChunk.x, lightChunk.y, lightChunk.z + 1.0f))) {
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
                            tempQueue.push_back(std::make_pair(lightBlock, lightIntensity - 1));
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

    std::cout << "kekw" << std::endl;
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
            std::shared_ptr<Block> block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            if (block->id != 0 && block->lightLevels[3].x < r.second) {
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
            }
            std::shared_ptr<Block> block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            if (block->id != 0 && block->lightLevels[2].x < r.second) {
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
            }
            std::shared_ptr<Block> block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            if (block->id != 0 && block->lightLevels[5].x < r.second) {
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
            }
            std::shared_ptr<Block> block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            if (block->id != 0 && block->lightLevels[4].x < r.second) {
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
            }
            std::shared_ptr<Block> block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            if (block->id != 0 && block->lightLevels[1].x < r.second) {
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
            }
            std::shared_ptr<Block> block = Server::getInstance().chunks[resultChunk]->getBlock(resultBlock);
            if (block->id != 0 && block->lightLevels[0].x < r.second) {
                // std::cout << "left" << std::endl;
                block->lightLevels[0].x = r.second;
            }
        }
    }

    return affectedChunks;
}

void ServerChunkMap::checkAmbient(Vec3<float> chunkPos) {
    std::map<Vec3<float>, int> darkResult;
    // int ch = 0;

    std::set<Vec3<float>> ambients;
    for (auto& a : HeightMap::getInstance().heightMaps[Vec2<float>(chunkPos.x, chunkPos.z)]) {
        ambients.insert(Vec3<float>(a.second.x, ((chunkPos.y * 8.0f) - (a.first * 8.0f)) + a.second.y, a.second.z));
    }

    for (auto& a : ambients) {
        // ch++;
        // std::cout << ch << " ch" << std::endl;
        // std::cout << b.second->id << " id" << std::endl;
        // if (b.second == nullptr) { continue; }
        int darknessLevel = 0;
        // printf("%f %f %f start\n", b.first.x, b.first.y, b.first.z);
        std::deque<std::pair<Vec3<float>, int>> darknessQueue;
        darknessQueue.push_back(std::make_pair(a, darknessLevel));
        while (!darknessQueue.empty() && darknessLevel > -5) {
            // std::cout << lightQueue.size() << std::endl;
            std::deque<std::pair<Vec3<float>, int>> tempQueue;
            for (auto& l : darknessQueue) {
                // x
                // printf("%f %f %f chunk\n", l.first.first.x, l.first.first.y, l.first.first.z);
                // printf("%f %f %f block\n", l.first.second.x, l.first.second.y, l.first.second.z);
                {
                    float x = l.first.x - 1.0f;
                    Vec3<float> darkPos = Vec3<float>(x, l.first.y, l.first.z);
                    std::shared_ptr<Block> block = nullptr;
                    if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        block = blocks[darkPos];
                    }
                    if (block != nullptr && block->id == 0 && ambients.find(darkPos) == ambients.end()) {
                        tempQueue.push_back(std::make_pair(darkPos, darknessLevel));
                        if (!darkResult.count(darkPos) || darkResult[darkPos] < darknessLevel) {
                            darkResult[darkPos] = darknessLevel;
                        }
                    }
                }
                {
                    float x = l.first.x + 1.0f;
                    Vec3<float> darkPos = Vec3<float>(x, l.first.y, l.first.z);
                    std::shared_ptr<Block> block = nullptr;
                    if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        block = blocks[darkPos];
                    }
                    if (block != nullptr && block->id == 0 && ambients.find(darkPos) == ambients.end()) {
                        tempQueue.push_back(std::make_pair(darkPos, darknessLevel));
                        if (!darkResult.count(darkPos) || darkResult[darkPos] < darknessLevel) {
                            darkResult[darkPos] = darknessLevel;
                        }
                    }
                }
                // y
                {
                    float y = l.first.y - 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.x, y, l.first.z);
                    std::shared_ptr<Block> block = nullptr;
                    if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        block = blocks[darkPos];
                    }
                    if (block != nullptr && block->id == 0 && ambients.find(darkPos) == ambients.end()) {
                        tempQueue.push_back(std::make_pair(darkPos, darknessLevel));
                        if (!darkResult.count(darkPos) || darkResult[darkPos] < darknessLevel) {
                            darkResult[darkPos] = darknessLevel;
                        }
                    }
                }
                {
                    float y = l.first.y + 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.x, y, l.first.z);
                    std::shared_ptr<Block> block = nullptr;
                    if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        block = blocks[darkPos];
                    }
                    if (block != nullptr && block->id == 0 && ambients.find(darkPos) == ambients.end()) {
                        tempQueue.push_back(std::make_pair(darkPos, darknessLevel));
                        if (!darkResult.count(darkPos) || darkResult[darkPos] < darknessLevel) {
                            darkResult[darkPos] = darknessLevel;
                        }
                    }
                }
                // z
                {
                    float z = l.first.z - 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.z, l.first.y, z);
                    std::shared_ptr<Block> block = nullptr;
                    if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        block = blocks[darkPos];
                    }
                    if (block != nullptr && block->id == 0 && ambients.find(darkPos) == ambients.end()) {
                        tempQueue.push_back(std::make_pair(darkPos, darknessLevel));
                        if (!darkResult.count(darkPos) || darkResult[darkPos] < darknessLevel) {
                            darkResult[darkPos] = darknessLevel;
                        }
                    }
                }
                {
                    float z = l.first.z + 1.0f;
                    Vec3<float> darkPos = Vec3<float>(l.first.z, l.first.y, z);
                    std::shared_ptr<Block> block = nullptr;
                    if (checkValidPos(Vec3<float>(darkPos.x, darkPos.y, darkPos.z))) {
                        block = blocks[darkPos];
                    }
                    if (block != nullptr && block->id == 0 && ambients.find(darkPos) == ambients.end()) {
                        tempQueue.push_back(std::make_pair(darkPos, darknessLevel));
                        if (!darkResult.count(darkPos) || darkResult[darkPos] < darknessLevel) {
                            darkResult[darkPos] = darknessLevel;
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
            Vec3<float> resultBlock = Vec3<float>(r.first.x - 1.0f, r.first.y, r.first.z);
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                block = blocks[resultBlock];
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[3].y < r.second) {
                // std::cout << "front" << std::endl;
                block->lightLevels[3].y = r.second;
            }
        }
        {
            Vec3<float> resultBlock = Vec3<float>(r.first.x + 1.0f, r.first.y, r.first.z);
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                block = blocks[resultBlock];
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[2].y < r.second) {
                // std::cout << "back" << std::endl;
                block->lightLevels[2].y = r.second;
            }
        }
        // y
        {
            Vec3<float> resultBlock = Vec3<float>(r.first.x, r.first.y - 1.0f, r.first.z);
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                block = blocks[resultBlock];
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[5].y < r.second) {
                // std::cout << "top" << std::endl;
                block->lightLevels[5].y = r.second;
            }
        }
        {
            Vec3<float> resultBlock = Vec3<float>(r.first.x, r.first.y + 1.0f, r.first.z);
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                block = blocks[resultBlock];
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[4].y < r.second) {
                // std::cout << "bottom" << std::endl;
                block->lightLevels[4].y = r.second;
            }
        }
        // z
        {
            Vec3<float> resultBlock = Vec3<float>(r.first.x, r.first.y, r.first.z - 1.0f);
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                block = blocks[resultBlock];
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[1].y < r.second) {
                // std::cout << "right" << std::endl;
                block->lightLevels[1].y = r.second;
            }
        }
        {
            Vec3<float> resultBlock = Vec3<float>(r.first.x, r.first.y, r.first.z + 1.0f);
            std::shared_ptr<Block> block = nullptr;
            if (checkValidPos(Vec3<float>(resultBlock.x, resultBlock.y, resultBlock.z))) {
                block = blocks[resultBlock];
            }
            if (block != nullptr && block->id != 0 && block->lightLevels[0].y < r.second) {
                // std::cout << "left" << std::endl;
                block->lightLevels[0].y = r.second;
            }
        }
    }
}