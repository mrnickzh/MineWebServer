#pragma once
#include <memory>


#include "../Utils/Vec.hpp"
#include "Utils/Block.hpp"
#include "Utils/ServerEntity.hpp"

class ServerPhysicsObject {
public:
    std::shared_ptr<ServerEntity> object;
    Vec3<float> velocity;
    float mass;

    ServerPhysicsObject(std::shared_ptr<ServerEntity>& obj, float m);
    Vec3<float> getPosition();
    void setPosition(Vec3<float> pos);
    Vec3<float> getCollider();
};

