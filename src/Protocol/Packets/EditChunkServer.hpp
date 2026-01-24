#pragma once

#include "../ServerPacket.hpp"
#include "../../Utils/Vec.hpp"

class EditChunkServer : public ServerPacket {
public:
    int id;
    Vec3<float> chunkpos;
    Vec3<float> blockpos;
    std::set<Vec3<float>> L_affectedChunks;
    std::set<Vec3<float>> A_affectedChunks;

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

        // std::cout << id << std::endl;
        // std::cout << chunkpos.x << " " << chunkpos.y << " " << chunkpos.z << std::endl;
        // std::cout << blockpos.x << " " << blockpos.y << " " << blockpos.z << std::endl;

        Block prevblock = *(Server::getInstance().chunks[chunkpos]->getBlock(blockpos));
        std::shared_ptr<Block> block = std::make_shared<Block>(id, blockpos);
        Server::getInstance().chunks[chunkpos]->addBlock(blockpos, block);
        A_affectedChunks = Server::getInstance().chunks[chunkpos]->checkHeight(chunkpos, blockpos);
        L_affectedChunks = Server::getInstance().chunks[chunkpos]->checkLights(chunkpos, prevblock);
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

        std::set<Vec3<float>> affectedChunks;
        std::set<Vec3<float>> ambientChunks;

        for (auto c : L_affectedChunks) {
            affectedChunks.insert(c);
            Server::getInstance().chunks[c]->checkLights(c, Block(0, Vec3<float>(0.0f, 0.0f, 0.0f)));
        }
        for (auto c : A_affectedChunks) {
            ambientChunks.insert(c);
            std::set<Vec3<float>> chunks = Server::getInstance().chunks[c]->checkAmbient(c);
            for (auto& a : chunks) {
                ambientChunks.insert(a);
            }
        }
        for (auto c : ambientChunks) {
            Server::getInstance().chunks[c]->resetAmbient();
        }
        for (auto c : ambientChunks) {
            Server::getInstance().chunks[c]->checkAmbient(c);
            affectedChunks.insert(c);
        }
        // std::cout << affectedChunks.size() << " size" << std::endl;
        for (auto c : affectedChunks) {
            LightMapServer lightpacket;
            lightpacket.chunkpos = c;
            for (auto& s : Server::getInstance().clients) {
                Server::getInstance().sendPacket(s.first, &lightpacket);
            }
        }
    }
};