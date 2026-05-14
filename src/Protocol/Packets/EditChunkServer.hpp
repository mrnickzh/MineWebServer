#pragma once

#include "../ServerPacket.hpp"

#define GLM_FORCE_PURE
#include "../../../lib/glm/glm.hpp"
#include "../../../lib/glm/gtc/matrix_transform.hpp"
#include "../../../lib/glm/gtc/type_ptr.hpp"

class EditChunkServer : public ServerPacket {
public:
    int id;
    glm::vec3 chunkpos;
    glm::vec3 blockpos;
    std::set<glm::vec3, vec3Comparator> L_affectedChunks;
    std::set<glm::vec3, vec3Comparator> A_affectedChunks;

    void receive(ByteBuf &buffer) override {
        id = buffer.readInt();
        float cx = buffer.readFloat();
        float cy = buffer.readFloat();
        float cz = buffer.readFloat();
        chunkpos = glm::vec3(cx, cy, cz);
        float bx = buffer.readFloat();
        float by = buffer.readFloat();
        float bz = buffer.readFloat();
        blockpos = glm::vec3(bx, by, bz);

        // std::cout << id << std::endl;
        // std::cout << chunkpos.x << " " << chunkpos.y << " " << chunkpos.z << std::endl;
        // std::cout << blockpos.x << " " << blockpos.y << " " << blockpos.z << std::endl;

        Block prevblock = Block(0, glm::vec3(blockpos.x, blockpos.y, blockpos.z), glm::vec3(0.0f, 0.0f, 0.0f), false, glm::vec3(0.5f, 0.5f, 0.5f));
        {
            std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
            prevblock = *(Server::getInstance().chunks[chunkpos]->getBlock(blockpos));
            std::shared_ptr<Block> block = std::make_shared<Block>(id, blockpos, glm::vec3(0.0f, 0.0f, 0.0f), (id == 0 ? false : true), glm::vec3(0.5f, 0.5f, 0.5f));
            Server::getInstance().chunks[chunkpos]->addBlock(blockpos, block);

            std::string modName = Server::getInstance().serverBlockRegistry->getBlock(id).first.first;
            if (modName != "base") {
                Server::getInstance().serverModManager->mods[modName]->doEvent("block_removed");
            }
        }
        // A_affectedChunks = Server::getInstance().chunks[chunkpos]->checkHeight(chunkpos, blockpos);
        // L_affectedChunks = Server::getInstance().chunks[chunkpos]->checkLights(chunkpos, prevblock);
        if (Server::getInstance().lightUpdateQueueMutex.try_lock()) {
            Server::getInstance().lightUpdateQueue.push_back(std::pair(chunkpos, prevblock));
            Server::getInstance().lightUpdateQueueMutex.unlock();
        }
        else if (Server::getInstance().lightUpdateFallbackQueueMutex.try_lock()) {
            Server::getInstance().lightUpdateFallbackQueue.push_back(std::pair(chunkpos, prevblock));
            Server::getInstance().lightUpdateFallbackQueueMutex.unlock();
        }
        else {
            std::cout << "ERROR" << std::endl;
        }
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

        // std::set<Vec3<float>> affectedChunks;
        // std::set<Vec3<float>> ambientChunks;
        //
        // for (auto c : L_affectedChunks) {
        //     affectedChunks.insert(c);
        //     Server::getInstance().chunks[c]->checkLights(c, Block(0, glm::vec3(0.0f, 0.0f, 0.0f)));
        // }
        // for (auto c : A_affectedChunks) {
        //     ambientChunks.insert(c);
        //     std::set<Vec3<float>> chunks = Server::getInstance().chunks[c]->checkAmbient(c);
        //     for (auto& a : chunks) {
        //         ambientChunks.insert(a);
        //     }
        // }
        // for (auto c : ambientChunks) {
        //     Server::getInstance().chunks[c]->resetAmbient();
        // }
        // for (auto c : ambientChunks) {
        //     Server::getInstance().chunks[c]->checkAmbient(c);
        //     affectedChunks.insert(c);
        // }
        // std::cout << affectedChunks.size() << " size" << std::endl;
        // for (auto c : affectedChunks) {
        //     LightMapServer lightpacket;
        //     lightpacket.chunkpos = c;
        //     for (auto& s : Server::getInstance().clients) {
        //         Server::getInstance().sendPacket(s.first, &lightpacket);
        //     }
        // }
    }
};