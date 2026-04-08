#include "ServerPhysicsObject.hpp"

#include "Utils/Block.hpp"

ServerPhysicsObject::ServerPhysicsObject(std::shared_ptr<ServerEntity>& obj, float m) {
    object = obj;
    mass = m;
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 ServerPhysicsObject::getPosition() { return object->position; }
void ServerPhysicsObject::setPosition(glm::vec3 pos) { object->position = pos; }
glm::vec3 ServerPhysicsObject::getCollider() { return object->collider; }
