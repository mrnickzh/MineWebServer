#pragma once

#include <functional>
#include <map>
#include <vector>

#include "ServerPacket.hpp"

namespace ServerPacketHelper {

    inline std::map<int, std::function<ServerPacket*()>> packetCreators;

    inline int getPacketId(ServerPacket* packet) {
        for (const auto& pair : packetCreators) {
            if (typeid(*packet) == typeid(*pair.second())) {

                return pair.first;
            }
        }
        return -1;
    }

    inline void registerPacket(int id, std::function<ServerPacket*()> creator) {
        packetCreators[id] = creator;
    }

    inline ServerPacket* createPacket(int id) {
        if (packetCreators.find(id) != packetCreators.end()) {
            return packetCreators[id]();
        }
        return nullptr;
    }

    inline std::vector<uint8_t> encodePacket(ServerPacket* packet) {
        ByteBuf buffer(65536);
        int id = getPacketId(packet);
        buffer.writeInt(id);
        packet->send(buffer);
        return buffer.toByteArray();
    }

    inline ServerPacket* decodePacket(ClientSession* session, const std::vector<uint8_t> data) {
        ByteBuf buffer(65536);

        buffer.fromByteArray(data);
        int id = buffer.readInt();
        ServerPacket* packet = createPacket(id);
        if (packet) {
            packet->receive(buffer);
            packet->process(session);
        }
        return packet;
    }
}