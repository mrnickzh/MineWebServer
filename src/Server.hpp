#pragma once
#include <functional>
#include <map>
#include <vector>
#include <set>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/Vec.hpp"
#include "Utils/ServerChunkMap.hpp"
#include "Utils/ServerEntity.hpp"

class Server {
public:
    std::function<void(ClientSession*, std::vector<uint8_t>)> callback;
    std::map<ClientSession*, void*> clients;

    std::map<Vec3<float>, std::shared_ptr<ServerChunkMap>> chunks;
    std::set<ServerEntity> entities;

    static Server& getInstance() {
        static Server instance;
        return instance;
    }

#ifndef BUILD_TYPE_DEDICATED
    void saveWorld();
    void loadWorld();
#endif

    void setCallback(std::function<void(ClientSession*, std::vector<uint8_t>)> callback);
    void processPacket(ClientSession* session, std::vector<uint8_t> data);
    void sendPacket(ClientSession* session, ServerPacket* packet);
};
