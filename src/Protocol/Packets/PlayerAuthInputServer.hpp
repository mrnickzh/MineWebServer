#pragma once

#include "Server.hpp"
#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerEntity.hpp"

class PlayerAuthInputServer : public ServerPacket {
public:
    std::string uuid;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 velocity;

    void receive(ByteBuf &buffer) override {
        float px = buffer.readFloat();
        float py = buffer.readFloat();
        float pz = buffer.readFloat();
        position = glm::vec3(px, py, pz);
        float rx = buffer.readFloat();
        float ry = buffer.readFloat();
        float rz = buffer.readFloat();
        rotation = glm::vec3(rx, ry, rz);
        float vx = buffer.readFloat();
        float vy = buffer.readFloat();
        float vz = buffer.readFloat();
        velocity = glm::vec3(vx, vy, vz);
    }

    void send(ByteBuf &buffer) override {
        buffer.writeString(uuid);

        buffer.writeFloat(position.x);
        buffer.writeFloat(position.y);
        buffer.writeFloat(position.z);

        buffer.writeFloat(rotation.x);
        buffer.writeFloat(rotation.y);
        buffer.writeFloat(rotation.z);

        buffer.writeFloat(velocity.x);
        buffer.writeFloat(velocity.y);
        buffer.writeFloat(velocity.z);
    }

    void process(ClientSession* session) override {

        std::shared_ptr<ServerEntity> e;

        {
            std::lock_guard<std::mutex> lock(Server::getInstance().serverEntityMutex);
            e = Server::getInstance().entities[session->uuid];
            e->position = position;
            e->rotation = rotation;
            Server::getInstance().serverPhysicsEngine->setVelocity(e, velocity);
        }

        // std::cout << uuid << std::endl;

        for (auto& s : Server::getInstance().clients) {
            if (s.first != session) {
                PlayerAuthInputServer packet;
                packet.uuid = session->uuid;
                packet.position = position;
                packet.rotation = rotation;
                packet.velocity = velocity;
                Server::getInstance().sendPacket(s.first, &packet);
            }
        }
    }
};
