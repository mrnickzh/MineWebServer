#pragma once

#include "../../MineWebClient/src/Protocol/Packet.hpp"
#include <iostream>

#include "AddMapObjectServer.hpp"
#include "Server.hpp"

class HandShakePacketServer : public Packet {
public:
    std::string name;

    void send(ByteBuf &buffer) override {}

    void receive(ByteBuf &buffer) override {
        for (int i = -1; i < 2; i++) {
            for (int j = -1; j < 2; j++) {
                AddMapObjectServer packet;
                packet.id = 0;
                packet.position = Vec3<float>((float)i, 0, (float)j);
                packet.rotation = Vec3<float>(0, 0, 0);
                Server::getInstance().sendPacket(&packet);
            }
        }
    }
};
