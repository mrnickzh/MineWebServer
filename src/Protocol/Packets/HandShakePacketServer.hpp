#pragma once

#include "../ServerPacket.hpp"
#include <iostream>

#include "EditChunkServer.hpp"
#include "EntityActionServer.hpp"
#include "PlayerAuthInputServer.hpp"
#include "Server.hpp"
#include "Utils/uuid.hpp"

class HandShakePacketServer : public ServerPacket {
public:
    std::string name;

    void send(ByteBuf &buffer) override {}

    void receive(ByteBuf &buffer) override {
        name = buffer.readString();
    }

    void process(ClientSession* session) override {
        session->username = name;
        session->uuid = uuid::v4::UUID::New().String();

        ServerEntity entity = ServerEntity(session->uuid, Vec3<float>(0.0f, 1.0f, 0.0f), Vec3<float>(0.0f, 0.0f, 0.0f), Vec3<float>(0.0f, 0.0f, 0.0f));
        Server::getInstance().entities.insert(entity);

        EntityActionServer packet;
        packet.uuid = session->uuid;
        packet.action = 0;
        for (auto& s : Server::getInstance().clients) {
            if (s.first != session) {
                Server::getInstance().sendPacket(s.first, &packet);

                EntityActionServer replicationpacket;
                replicationpacket.uuid = s.first->uuid;
                replicationpacket.action = 0;
                Server::getInstance().sendPacket(session, &replicationpacket);

                PlayerAuthInputServer inputpacket;
                inputpacket.uuid = s.first->uuid;
                inputpacket.position = entity.position;
                inputpacket.rotation = entity.rotation;
                inputpacket.velocity = entity.velocity;
                Server::getInstance().sendPacket(session, &inputpacket);
            }
        }
    }
};
