#pragma once

#include <WorldSaving/RegionRegistory.hpp>
#include "Server.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerChunkMap.hpp"
#include "Utils/Vec.hpp"

class LightMapServer : public ServerPacket {
    public:
        Vec3<float> chunkpos;

    void receive(ByteBuf &buffer) override {}

    void send(ByteBuf &buffer) override {
        std::shared_ptr<ServerChunkMap> chunkMap = Server::getInstance().chunks[chunkpos];

        buffer.writeFloat(chunkpos.x);
        buffer.writeFloat(chunkpos.y);
        buffer.writeFloat(chunkpos.z);

        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    std::shared_ptr<Block> block = chunkMap->getBlock(Vec3<float>((float)x, (float)y, (float)z));
                    buffer.writeFloat(block->position.x);
                    buffer.writeFloat(block->position.y);
                    buffer.writeFloat(block->position.z);
                    for (int i = 0; i < 6; i++) {
                        // if (block->lightLevels[i].x != 0) { printf("%f %f %f block %d side %d level\n", block->position.x, block->position.y, block->position.z, i, block->lightLevels[i].x); }
                        buffer.writeInt(block->lightLevels[i].x);
                        buffer.writeInt(block->lightLevels[i].y);
                    }
                }
            }
        }
    }

    void process(ClientSession* session) override {}
};
