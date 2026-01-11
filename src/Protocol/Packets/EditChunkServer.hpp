#pragma once

#include "../ServerPacket.hpp"
#include "../../Utils/Vec.hpp"

class EditChunkServer : public ServerPacket {
public:
    int id;
    Vec3<float> chunkpos;
    Vec3<float> blockpos;
    std::set<Vec3<float>> affectedChunks;

    void receive(ByteBuf &buffer) override {
        id = buffer.readInt();
        float cx = buffer.readFloat();
        float cy = buffer.readFloat();
        float cz = buffer.readFloat();
        chunkpos = Vec3<float>(cx, cy, cz);
        float bx = buffer.readFloat();
        float by = buffer.readFloat();
        float bz = buffer.readFloat();
        blockpos = Vec3<float>(bx, by, bz);

        if (by < Server::getInstance().worldMinY || by > Server::getInstance().worldMaxY) { return; }

        // std::cout << id << std::endl;
        // std::cout << chunkpos.x << " " << chunkpos.y << " " << chunkpos.z << std::endl;
        // std::cout << blockpos.x << " " << blockpos.y << " " << blockpos.z << std::endl;

        std::shared_ptr<Block> block = std::make_shared<Block>(id, blockpos);
        Server::getInstance().chunks[chunkpos]->addBlock(blockpos, block);
        affectedChunks = Server::getInstance().chunks[chunkpos]->checkLights(chunkpos);
    }

    void send(ByteBuf &buffer) override {
        buffer.writeInt(id);
        buffer.writeFloat(chunkpos.x);
        buffer.writeFloat(chunkpos.y);
        buffer.writeFloat(chunkpos.z);
        buffer.writeFloat(blockpos.x);
        buffer.writeFloat(blockpos.y);
        buffer.writeFloat(blockpos.z);
    }

    void process(ClientSession* session) override {
        EditChunkServer packet;
        packet.id = id;
        packet.chunkpos = chunkpos;
        packet.blockpos = blockpos;
        for (auto& s : Server::getInstance().clients) {
            Server::getInstance().sendPacket(s.first, &packet);
        }
        for (auto c : affectedChunks) {
            LightMapServer lightpacket;
            lightpacket.chunkpos = c;
            for (auto& s : Server::getInstance().clients) {
                Server::getInstance().sendPacket(s.first, &lightpacket);
            }
        }
    }
};