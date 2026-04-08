#pragma once

#include <memory>
#include <string>

class ServerEntity {
public:
    std::string uuid;
    int id;
    glm::vec3 position;
    glm::vec3 rotation;
    bool cancollide;
    glm::vec3 collider;
    ServerEntity(std::string uuid, int id, glm::vec3 position, glm::vec3 rotation, bool cancollide, glm::vec3 collider) : uuid(uuid), id(id), position(position), rotation(rotation), cancollide(cancollide), collider(collider) {}

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
