#pragma once

#include "Server.hpp"
#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerEntity.hpp"

class PlayerAuthInputServer : public ServerPacket {
public:
    std::string uuid;
    Vec3<float> position;
    Vec3<float> rotation;
    Vec3<float> velocity;

    void receive(ByteBuf &buffer) override {
        float px = buffer.readFloat();
        float py = buffer.readFloat();
        float pz = buffer.readFloat();
        position = Vec3<float>(px, py, pz);
        float rx = buffer.readFloat();
        float ry = buffer.readFloat();
        float rz = buffer.readFloat();
        rotation = Vec3<float>(rx, ry, rz);
        float vx = buffer.readFloat();
        float vy = buffer.readFloat();
        float vz = buffer.readFloat();
        velocity = Vec3<float>(vx, vy, vz);
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
        Server::getInstance().entities.erase(ServerEntity(session->uuid, position, rotation, velocity));
        Server::getInstance().entities.insert(ServerEntity(session->uuid, position, rotation, velocity));

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
