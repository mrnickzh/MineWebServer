#pragma once

#include <iostream>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"

class EntityActionServer : public ServerPacket {

public:
    std::string uuid;
    int action;

    void send(ByteBuf &buffer) override {
        buffer.writeString(uuid);
        buffer.writeInt(action);
    }

    void receive(ByteBuf &buffer) override {}

    void process(ClientSession* session) override {}
};
