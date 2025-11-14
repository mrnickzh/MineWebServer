#pragma once
#include <functional>
#include <vector>

#include "../../MineWebClient/src/Protocol/Packet.hpp"

class Server {
public:
    std::function<void(std::vector<uint8_t>)> callback;

    static Server& getInstance() {
        static Server instance;
        return instance;
    }

    void setCallback(std::function<void(std::vector<uint8_t>)> callback);
    void processPacket(std::vector<uint8_t> data);
    void sendPacket(Packet* packet);
};
