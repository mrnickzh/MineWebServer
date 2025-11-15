#pragma once
#include <functional>
#include <vector>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"

class Server {
public:
    std::function<void(ClientSession, std::vector<uint8_t>)> callback;

    static Server& getInstance() {
        static Server instance;
        return instance;
    }

    void setCallback(std::function<void(ClientSession, std::vector<uint8_t>)> callback);
    void processPacket(ClientSession session, std::vector<uint8_t> data);
    void sendPacket(ClientSession session, ServerPacket* packet);
};
