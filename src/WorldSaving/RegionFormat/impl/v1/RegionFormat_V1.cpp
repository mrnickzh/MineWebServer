#include <Server.hpp>
#include <iostream>
#include "RegionFormat_V1.hpp"

#include <mutex>

void RegionFormat_V1::load(ByteBuf &buffer, glm::vec3 pos) {
    int size = buffer.readInt();

    // std::cout << "Region " << pos.x << " " << pos.y << " " << pos.z << std::endl;

    for (int i = 0; i < size; i++) {
        std::shared_ptr<ServerChunkMap> chunkMap = std::make_shared<ServerChunkMap>();
        float cx = buffer.readFloat();
        float cy = buffer.readFloat();
        float cz = buffer.readFloat();
        glm::vec3 regionChunk = glm::vec3(cx, cy, cz);

        // std::cout << "Chunk " << regionChunk.x << " " << regionChunk.y << " " << regionChunk.z << std::endl;

        for (int j = 0; j < 512; j++) {
            float bx = buffer.readFloat();
            float by = buffer.readFloat();
            float bz = buffer.readFloat();
            std::string mod = buffer.readString();
            std::string id = buffer.readString();

            int realid;
            if (mod == "base") {
                realid = std::stoi(id);
            }
            else {
                realid = Server::getInstance().serverModManager->mods[mod]->modBlocks[id];
            }
            glm::vec3 blockPos = glm::vec3(bx, by, bz);
            std::shared_ptr<Block> block = std::make_shared<Block>(realid, blockPos, glm::vec3(0.0f, 0.0f, 0.0f), (realid == 0 ? false : true), glm::vec3(0.5f, 0.5f, 0.5f));

            for (int l = 0; l < 6; l++) {
                block->lightLevels[l].x = buffer.readInt();
                block->lightLevels[l].y = buffer.readInt();
            }

            chunkMap->addBlock(blockPos, block);
        }

        int lightSize = buffer.readInt();
        for (int j = 0; j < lightSize; j++) {
            float lcx = buffer.readFloat();
            float lcy = buffer.readFloat();
            float lcz = buffer.readFloat();
            float lbx = buffer.readFloat();
            float lby = buffer.readFloat();
            float lbz = buffer.readFloat();
            int lvl = buffer.readInt();
            glm::vec3 lightChunkPos = glm::vec3(lcx, lcy, lcz);
            glm::vec3 lightBlockPos = glm::vec3(lbx, lby, lbz);
            chunkMap->lightSources[std::make_pair(lightChunkPos, lightBlockPos)] = lvl;
        }

        int ambientSize = buffer.readInt();
        for (int j = 0; j < ambientSize; j++) {
            float lcx = buffer.readFloat();
            float lcy = buffer.readFloat();
            float lcz = buffer.readFloat();
            float lbx = buffer.readFloat();
            float lby = buffer.readFloat();
            float lbz = buffer.readFloat();
            int lvl = buffer.readInt();
            glm::vec3 lightChunkPos = glm::vec3(lcx, lcy, lcz);
            glm::vec3 lightBlockPos = glm::vec3(lbx, lby, lbz);
            chunkMap->ambientSources[std::make_pair(lightChunkPos, lightBlockPos)] = lvl;
        }

        // std::cout << regionChunk.x << " " << regionChunk.y << " " << regionChunk.z << " regionchunk" << std::endl;
        {
            std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
            Server::getInstance().chunks[regionChunk] = chunkMap;
        }
    }
}

void RegionFormat_V1::save(ByteBuf &buffer, glm::vec3 pos) {
    int size = 0;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                glm::vec3 regionChunk = glm::vec3((pos.x * 8.0f) + (float)x, (pos.y * 8.0f) + (float)y, (pos.z * 8.0f) + (float)z);

                if (Server::getInstance().chunks.find(regionChunk) == Server::getInstance().chunks.end()) {
                    continue;
                }

                size++;
            }
        }
    }

    buffer.writeInt(size);

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                glm::vec3 regionChunk = glm::vec3((pos.x * 8.0f) + (float)x, (pos.y * 8.0f) + (float)y, (pos.z * 8.0f) + (float)z);

                if (Server::getInstance().chunks.find(regionChunk) == Server::getInstance().chunks.end()) {
                    // std::cout << "No chunk " << regionChunk.x << " " << regionChunk.y << " " << regionChunk.z << std::endl;
                    continue;
                }

                ServerChunkMap* map = Server::getInstance().chunks[regionChunk].get();
                buffer.writeFloat(regionChunk.x);
                buffer.writeFloat(regionChunk.y);
                buffer.writeFloat(regionChunk.z);
                for(auto& blockPair : map->blocks){
                    buffer.writeFloat(blockPair->position.x);
                    buffer.writeFloat(blockPair->position.y);
                    buffer.writeFloat(blockPair->position.z);
                    std::pair<std::string, std::string> blockMod = Server::getInstance().serverBlockRegistry->getBlock(blockPair->id).first;
                    buffer.writeString(blockMod.first);
                    buffer.writeString(blockMod.second);
                    for (int l = 0; l < 6; l++) {
                        buffer.writeInt(blockPair->lightLevels[l].x);
                        buffer.writeInt(blockPair->lightLevels[l].y);
                    }
                }

                buffer.writeInt((int)(map->lightSources.size()));
                for (auto& sources : map->lightSources) {
                    buffer.writeFloat(sources.first.first.x);
                    buffer.writeFloat(sources.first.first.y);
                    buffer.writeFloat(sources.first.first.z);
                    buffer.writeFloat(sources.first.second.x);
                    buffer.writeFloat(sources.first.second.y);
                    buffer.writeFloat(sources.first.second.z);
                    buffer.writeInt(sources.second);
                }

                buffer.writeInt((int)(map->ambientSources.size()));
                for (auto& sources : map->ambientSources) {
                    buffer.writeFloat(sources.first.first.x);
                    buffer.writeFloat(sources.first.first.y);
                    buffer.writeFloat(sources.first.first.z);
                    buffer.writeFloat(sources.first.second.x);
                    buffer.writeFloat(sources.first.second.y);
                    buffer.writeFloat(sources.first.second.z);
                    buffer.writeInt(sources.second);
                }
            }
        }
    }
}