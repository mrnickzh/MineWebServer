#include <Server.hpp>
#include <iostream>
#include "RegionFormat_V1.hpp"

void RegionFormat_V1::load(ByteBuf &buffer, Vec3<float> pos) {
    int size = buffer.readInt();

    // std::cout << "Region " << pos.x << " " << pos.y << " " << pos.z << std::endl;

    for (int i = 0; i < size; i++) {
        std::shared_ptr<ServerChunkMap> chunkMap = std::make_shared<ServerChunkMap>();
        float cx = buffer.readFloat();
        float cy = buffer.readFloat();
        float cz = buffer.readFloat();
        Vec3<float> regionChunk = Vec3<float>(cx, cy, cz);

        // std::cout << "Chunk " << regionChunk.x << " " << regionChunk.y << " " << regionChunk.z << std::endl;

        for (int j = 0; j < 512; j++) {
            float bx = buffer.readFloat();
            float by = buffer.readFloat();
            float bz = buffer.readFloat();
            int id = buffer.readInt();

            Vec3<float> blockPos = Vec3(bx, by, bz);
            std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos);
            chunkMap->blocks[blockPos] = block;
        }

        // std::cout << regionChunk.x << " " << regionChunk.y << " " << regionChunk.z << " regionchunk" << std::endl;
        Server::getInstance().chunks[regionChunk] = chunkMap;
    }
}

void RegionFormat_V1::save(ByteBuf &buffer, Vec3<float> pos) {
    int size = 0;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            for (int z = 0; z < 8; z++) {
                Vec3<float> regionChunk = Vec3<float>((pos.x * 8.0f) + (float)x, (pos.y * 8.0f) + (float)y, (pos.z * 8.0f) + (float)z);

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
                Vec3<float> regionChunk = Vec3<float>((pos.x * 8.0f) + (float)x, (pos.y * 8.0f) + (float)y, (pos.z * 8.0f) + (float)z);

                if (Server::getInstance().chunks.find(regionChunk) == Server::getInstance().chunks.end()) {
                    // std::cout << "No chunk " << regionChunk.x << " " << regionChunk.y << " " << regionChunk.z << std::endl;
                    continue;
                }

                ServerChunkMap* map = Server::getInstance().chunks[regionChunk].get();
                buffer.writeFloat(regionChunk.x);
                buffer.writeFloat(regionChunk.y);
                buffer.writeFloat(regionChunk.z);
                for(auto& blockPair : map->blocks){
                    buffer.writeFloat(blockPair.first.x);
                    buffer.writeFloat(blockPair.first.y);
                    buffer.writeFloat(blockPair.first.z);
                    buffer.writeInt(blockPair.second->id);
                }
            }
        }
    }
}