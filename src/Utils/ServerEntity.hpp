#pragma once

#include <memory>
#include <string>

class ServerEntity {
public:
    std::string uuid;
    Vec3<float> position;
    Vec3<float> rotation;
    Vec3<float> velocity;
    ServerEntity(std::string uuid, Vec3<float> position, Vec3<float> rotation, Vec3<float> velocity) : uuid(uuid), position(position), rotation(rotation), velocity(velocity) {}

    bool operator==(const ServerEntity& other) const {
        return this->uuid == other.uuid;
    }

    bool operator!=(const ServerEntity& other) const {
        return this->uuid != other.uuid;
    }

    bool operator<(const ServerEntity& other) const {
        return this->uuid < other.uuid;
    }
};
