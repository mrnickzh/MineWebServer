#pragma once

#include "../ServerPacket.hpp"
#include <iostream>

#include "EditChunkServer.hpp"
#include "EntityActionServer.hpp"
#include "PlayerAuthInputServer.hpp"
#include "NetworkSettingsPacket.hpp"
#include "RegisterModServer.hpp"
#include "Server.hpp"
#include "TransferModServer.hpp"
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

        NetworkSettingsPacketServer networkSettings;
        networkSettings.settings = session->networkSettings;
        Server::getInstance().sendPacket(session, &networkSettings);
        session->connectionState = ConnectionState::PLAY;

#ifdef BUILD_TYPE_DEDICATED
        for (auto mod : Server::getInstance().serverModManager->mods) {
            TransferModServer transferMod;
            transferMod.modName = mod.first;
            Server::getInstance().sendPacket(session, &transferMod);
        }
#endif

        for (auto mod : Server::getInstance().serverModManager->mods) {
            RegisterModServer regMod;
            regMod.modName = mod.first;
            Server::getInstance().sendPacket(session, &regMod);
        }

        std::shared_ptr<ServerEntity> entity = std::make_shared<ServerEntity>(session->uuid, 49, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), true, glm::vec3(0.25f, 0.75f, 0.25f));
        Server::getInstance().entities[session->uuid] = entity;
        Server::getInstance().serverPhysicsEngine->registerObject(entity, 1.0f);

        EntityActionServer packet;
        packet.uuid = session->uuid;
        packet.action = 0;
        packet.id = 49;
        for (auto& s : Server::getInstance().clients) {
            if (s.first != session) {
                Server::getInstance().sendPacket(s.first, &packet);

                EntityActionServer replicationpacket;
                replicationpacket.uuid = s.first->uuid;
                replicationpacket.action = 0;
                replicationpacket.id = 49;
                Server::getInstance().sendPacket(session, &replicationpacket);

                PlayerAuthInputServer inputpacket;
                inputpacket.uuid = s.first->uuid;
                inputpacket.position = entity->position;
                inputpacket.rotation = entity->rotation;
                inputpacket.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                Server::getInstance().sendPacket(session, &inputpacket);
            }
        }

        for (auto& s : Server::getInstance().entities) {
            if (s.second->id == 49) { continue; }
            EntityActionServer replicationpacket;
            replicationpacket.uuid = s.second->uuid;
            replicationpacket.action = 0;
            replicationpacket.id = 50;
            Server::getInstance().sendPacket(session, &replicationpacket);

            PlayerAuthInputServer inputpacket;
            inputpacket.uuid = s.second->uuid;
            inputpacket.position = entity->position;
            inputpacket.rotation = entity->rotation;
            inputpacket.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            Server::getInstance().sendPacket(session, &inputpacket);
        }
    }
};
