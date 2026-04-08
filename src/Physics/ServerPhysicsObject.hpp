#pragma once
#include <memory>

#include "Utils/Block.hpp"
#include "Utils/ServerEntity.hpp"

class ServerPhysicsObject {
public:
    std::shared_ptr<ServerEntity> object;
    glm::vec3 velocity;
    float mass;
    bool frozen = false;

    ServerPhysicsObject(std::shared_ptr<ServerEntity>& obj, float m);
    glm::vec3 getPosition();
    void setPosition(glm::vec3 pos);
    glm::vec3 getCollider();
};

