//
// Created by onlym on 3/31/2026.
//

#pragma once
#include "Protocol/NetworkSettings.hpp"
#include "Protocol/ServerPacket.hpp"

class NetworkSettingsPacketServer : public ServerPacket {
public:
    NetworkSettings settings;

    void receive(ByteBuf &buffer) override {}
    void send(ByteBuf &buffer) override {
        buffer.writeByte(settings.compressionType);
        buffer.writeInt(settings.compressionThreshold);
    }

    void process(ClientSession *session) override {}
};
