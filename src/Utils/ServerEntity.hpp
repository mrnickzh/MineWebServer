#pragma once

#include <memory>
#include <string>

class ServerEntity {
public:
    std::string uuid;
    Vec3<float> position;
    Vec3<float> rotation;
    bool cancollide;
    Vec3<float> collider;
    ServerEntity(std::string uuid, Vec3<float> position, Vec3<float> rotation, bool cancollide, Vec3<float> collider) : uuid(uuid), position(position), rotation(rotation), cancollide(cancollide), collider(collider) {}

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
