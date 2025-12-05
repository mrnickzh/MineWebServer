#pragma once

#include "Server.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerChunkMap.hpp"
#include "Utils/Vec.hpp"

class GenerateChunkServer : public ServerPacket {
    public:
        Vec3<float> chunkpos;

    void receive(ByteBuf &buffer) override {
        float cx = buffer.readFloat();
        float cy = buffer.readFloat();
        float cz = buffer.readFloat();
        chunkpos = Vec3<float>(cx, cy, cz);
    }
    void send(ByteBuf &buffer) override {
        std::shared_ptr<ServerChunkMap> chunkMap = std::make_shared<ServerChunkMap>();

        if (Server::getInstance().chunks.find(chunkpos) == Server::getInstance().chunks.end()) {
            if (chunkpos.y < 0)
                chunkMap->generate(-1);
            else
                chunkMap->generate(0);
            Server::getInstance().chunks[chunkpos] = chunkMap;
        }

        chunkMap = Server::getInstance().chunks[chunkpos];

        buffer.writeFloat(chunkpos.x);
        buffer.writeFloat(chunkpos.y);
        buffer.writeFloat(chunkpos.z);

        for (int x = 0; x < 8; x++) {
            for (int y = 0; y < 8; y++) {
                for (int z = 0; z < 8; z++) {
                    std::shared_ptr<Block> block = chunkMap->getBlock(Vec3<float>((float)x, (float)y, (float)z));
                    buffer.writeInt(block->id);
                    buffer.writeFloat(block->position.x);
                    buffer.writeFloat(block->position.y);
                    buffer.writeFloat(block->position.z);
                }
            }
        }
    }

    void process(ClientSession* session) override {
        GenerateChunkServer packet;
        packet.chunkpos = chunkpos;
        Server::getInstance().sendPacket(session, &packet);
    }
};
