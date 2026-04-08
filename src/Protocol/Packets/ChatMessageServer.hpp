#pragma once

#include "EntityActionServer.hpp"
#include "PlayerAuthInputServer.hpp"
#include "Server.hpp"
#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerEntity.hpp"
#include "Utils/StrSplit.hpp"
#include "Utils/uuid.hpp"

class ChatMessageServer : public ServerPacket {
public:
    std::string message;

    void receive(ByteBuf &buffer) override {
        message = buffer.readString();
    }

    void send(ByteBuf &buffer) override {
        buffer.writeString(message);
    }

    void process(ClientSession* session) override {
        if (message[0] == '/') {
            message.erase(0, 1);
            std::vector<std::string> args = StrSplit::str_split(message, "/");
            if (args.size() < 1) { return; }
            if (args[0] == "entity") {
                std::string euuid = uuid::v4::UUID::New().String();
                std::shared_ptr<ServerEntity> entity = std::make_shared<ServerEntity>(euuid, 50, glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), true, glm::vec3(0.25f, 0.75f, 0.25f));
                Server::getInstance().entities[euuid] = entity;
                Server::getInstance().serverPhysicsEngine->registerObject(entity, 1.0f);

                EntityActionServer packet;
                packet.uuid = euuid;
                packet.action = 0;
                packet.id = 50;
                PlayerAuthInputServer inputpacket;
                inputpacket.uuid = euuid;
                inputpacket.position = entity->position;
                inputpacket.rotation = entity->rotation;
                inputpacket.velocity = glm::vec3(0.0f, 0.0f, 0.0f);

                for (auto& s : Server::getInstance().clients) {
                    Server::getInstance().sendPacket(s.first, &packet);
                    Server::getInstance().sendPacket(s.first, &inputpacket);
                }

                ChatMessageServer chatpacket;
                chatpacket.message = "Entity created";
                Server::getInstance().sendPacket(session, &packet);
            }
        }
        else {
            for (auto& s : Server::getInstance().clients) {
                ChatMessageServer packet;
                packet.message = "[" + session->username + "]: " + message;
                Server::getInstance().sendPacket(s.first, &packet);
            }
        }
    }
};
