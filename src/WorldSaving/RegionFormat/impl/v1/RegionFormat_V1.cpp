//
// Created by onlym on 12/9/2025.
//

#include <Server.hpp>
#include <iostream>
#include "RegionFormat_V1.hpp"

void RegionFormat_V1::load(ByteBuf &buffer, Vec3<float> pos) {
    std::shared_ptr<ServerChunkMap> chunkMap = std::make_shared<ServerChunkMap>();

    int size = buffer.readInt();
    for (int i = 0; i < size; ++i) {
        float x = buffer.readFloat();
        float y = buffer.readFloat();
        float z = buffer.readFloat();
        int id = buffer.readInt();

        Vec3<float> blockPos = Vec3(x, y, z);
        std::shared_ptr<Block> block = std::make_shared<Block>(id, blockPos);
        chunkMap->blocks[blockPos] = block;
    }

    Server::getInstance().chunks[pos] = chunkMap;
}

void RegionFormat_V1::save(ByteBuf &buffer, Vec3<float> pos) {
    if (Server::getInstance().chunks.find(pos) == Server::getInstance().chunks.end()) {
        printf("Иди нахуй жирный негр\n");
        return;
    }

    ServerChunkMap* map = Server::getInstance().chunks[pos].get();
    buffer.writeInt(map->blocks.size());
    for(auto& blockPair : map->blocks){
        buffer.writeFloat(blockPair.first.x); // некит НАХУЯ ФЛОАТ СУКА НАХ???????????????????????????
        buffer.writeFloat(blockPair.first.y);
        buffer.writeFloat(blockPair.first.z);
        buffer.writeInt(blockPair.second->id);
    }
}