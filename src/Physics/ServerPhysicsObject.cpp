#include "ServerPhysicsObject.hpp"

#include "Utils/Block.hpp"

ServerPhysicsObject::ServerPhysicsObject(std::shared_ptr<ServerEntity>& obj, float m) {
    object = obj;
    mass = m;
    velocity = Vec3<float>(0.0f, 0.0f, 0.0f);
}

Vec3<float> ServerPhysicsObject::getPosition() { return object->position; }
void ServerPhysicsObject::setPosition(Vec3<float> pos) { object->position = pos; }
Vec3<float> ServerPhysicsObject::getCollider() { return object->collider; }
