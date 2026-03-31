#pragma once

#include <functional>
#include <map>
#include <vector>

#include "ServerPacket.hpp"
#include "Utils/ZLibUtils.hpp"

namespace ServerPacketHelper {

    inline std::map<int, std::function<ServerPacket*()>> packetCreators;

    inline int getPacketId(ServerPacket* packet) {
        for (const auto& pair : packetCreators) {
            ServerPacket* pkt = pair.second();
            if (typeid(*packet) == typeid(*pkt)) {
                delete pkt;
                return pair.first;
            }
            delete pkt;
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

    inline void decodePacket(ClientSession* session, std::vector<uint8_t> data) {
        if (session->connectionState != HANDSHAKE_EXCHANGE) {
            if (data[0] == 0xFF) {
                data.erase(data.begin());
            } else {
                switch (session->networkSettings.compressionType) {
                    case CompressionType::ZLIB:
                        data = ZLibUtils::decompress_data(data);
                        break;
                }
            }
        }

        ByteBuf buffer(65536);

        buffer.fromByteArray(data);
        int id = buffer.readInt();
        ServerPacket* packet = createPacket(id);
        if (packet) {
            packet->receive(buffer);
            packet->process(session);
            delete packet;
        }
    }
}
