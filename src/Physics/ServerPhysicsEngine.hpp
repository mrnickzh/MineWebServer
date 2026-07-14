#pragma once
#include <map>
#include <vector>

#include "../Utils/ServerChunkMap.hpp"

#include "ServerPhysicsObject.hpp"

struct ServerRaycastResult {
    bool hit;
    float distance;
    glm::vec3 blockpos;
    glm::vec3 chunkpos;
    Block* object;
    glm::vec3 prevblockpos;
    glm::vec3 prevchunkpos;
    Block* prevobject;
};

class ServerPhysicsEngine {
public:
    std::unordered_map<glm::vec3, std::vector<std::shared_ptr<ServerPhysicsObject>>, vec3Hash<float>, vec3Equals> registeredObjects;
    std::unordered_map<glm::vec3, std::shared_ptr<ServerChunkMap>, vec3Hash<float>, vec3Equals>* chunkmap;

    ServerPhysicsEngine(std::unordered_map<glm::vec3, std::shared_ptr<ServerChunkMap>, vec3Hash<float>, vec3Equals>* worldmap);
    void registerObject(std::shared_ptr<ServerEntity> object, float mass);
    void unregisterObject(std::shared_ptr<ServerEntity> object);
    void addVelocityRotation(std::shared_ptr<ServerEntity> object, glm::vec3 velocity);
    void addVelocityClampedRotation(std::shared_ptr<ServerEntity> object, glm::vec3 velocity, glm::vec3 limit);
    void addVelocity(std::shared_ptr<ServerEntity> object, glm::vec3 velocity);
    void addVelocityClamped(std::shared_ptr<ServerEntity> object, glm::vec3 velocity, glm::vec3 limit);
    void setVelocity(std::shared_ptr<ServerEntity> object, glm::vec3 velocity);
    bool isOnFoot(std::shared_ptr<ServerPhysicsObject> object);
    ServerRaycastResult raycast(float length, glm::vec3 startpos, glm::vec3 rotation);
    glm::vec3 getVelocity(std::shared_ptr<ServerEntity> object);
    std::shared_ptr<ServerPhysicsObject> getPhysicsObject(std::shared_ptr<ServerEntity> object);
    void step();

    bool isColliding(glm::vec3 object1, glm::vec3 object2, glm::vec3 collider1, glm::vec3 collider2);
    void checkEntityChunk(std::shared_ptr<ServerPhysicsObject> entity, glm::vec3 prevpos);
    std::vector<std::shared_ptr<ServerPhysicsObject>> possibleEntities(glm::vec3 position);
    bool possibleCollision(glm::vec3 position, glm::vec3 collider, const Block& object2);
    bool possibleEntityCollision(glm::vec3 position, glm::vec3 collider, std::shared_ptr<ServerEntity> object2);
    std::vector<Block> possibleObstacles(glm::vec3 position);
    void calculateVelocity(std::shared_ptr<ServerPhysicsObject> obj);
};

