#pragma once

#include "../Utils/ByteBuf.hpp"

class ServerPacket {
public:
    virtual ~ServerPacket() = default;
    virtual void send(ByteBuf& buffer) = 0;
    virtual void receive(ByteBuf& buffer) = 0;
    virtual void process() = 0;
};