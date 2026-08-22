#include "ServerModBridge.hpp"

#include "Server.hpp"
#include "Protocol/Packets/EditChunkServer.hpp"

void ServerModBridge::setBlock(int x, int y, int z, int id) {
    glm::vec3 chunkPos = glm::vec3();
    glm::vec3 blockPos = glm::vec3();
    chunkPos.x = floor((float)(x) / 8.0f);
    chunkPos.y = floor((float)(y) / 8.0f);
    chunkPos.z = floor((float)(z) / 8.0f);
    blockPos.x = (float)(x % 8);
    blockPos.y = (float)(y % 8);
    blockPos.z = (float)(z % 8);
    blockPos.x += (chunkPos.x < 0.0f ? (blockPos.x == 0.0f ? 0.0f : 8.0f) : 0.0f);
    blockPos.y += (chunkPos.y < 0.0f ? (blockPos.y == 0.0f ? 0.0f : 8.0f) : 0.0f);
    blockPos.z += (chunkPos.z < 0.0f ? (blockPos.z == 0.0f ? 0.0f : 8.0f) : 0.0f);

    std::cout << chunkPos.x << " " << chunkPos.y << " " << chunkPos.z << std::endl;
    std::cout << blockPos.x << " " << blockPos.y << " " << blockPos.z << std::endl;

    Block prevblock = Block(0, glm::vec3(blockPos.x, blockPos.y, blockPos.z), glm::vec3(0.0f, 0.0f, 0.0f), false, glm::vec3(0.5f, 0.5f, 0.5f));
    {
        std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
        prevblock = *(Server::getInstance().chunks[chunkPos]->getBlock(blockPos));
        Block block = Block(id, blockPos, glm::vec3(0.0f, 0.0f, 0.0f), (id == 0 ? false : true), glm::vec3(0.5f, 0.5f, 0.5f));
        Server::getInstance().chunks[chunkPos]->addBlock(blockPos, block);
    }

    if (Server::getInstance().lightUpdateQueueMutex.try_lock()) {
        Server::getInstance().lightUpdateQueue.push_back(std::pair(chunkPos, prevblock));
        Server::getInstance().lightUpdateQueueMutex.unlock();
    }
    else if (Server::getInstance().lightUpdateFallbackQueueMutex.try_lock()) {
        Server::getInstance().lightUpdateFallbackQueue.push_back(std::pair(chunkPos, prevblock));
        Server::getInstance().lightUpdateFallbackQueueMutex.unlock();
    }
    else {
        std::cout << "ERROR" << std::endl;
    }

    EditChunkServer packet;
    packet.id = id;
    packet.chunkpos = chunkPos;
    packet.blockpos = blockPos;
    for (auto& s : Server::getInstance().clients) {
        Server::getInstance().sendPacket(s.first, &packet);
    }
}
