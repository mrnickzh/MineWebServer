#pragma once
#include <map>
#include <vector>

#include "../Utils/ServerChunkMap.hpp"

#include "ServerPhysicsObject.hpp"

struct ServerRaycastResult {
    bool hit;
    float distance;
    Vec3<float> blockpos;
    Vec3<float> chunkpos;
    std::shared_ptr<Block> object;
    Vec3<float> prevblockpos;
    Vec3<float> prevchunkpos;
    std::shared_ptr<Block> prevobject;
};

class ServerPhysicsEngine {
public:
    std::map<Vec3<float>, std::vector<std::shared_ptr<ServerPhysicsObject>>> registeredObjects;
    std::map<Vec3<float>, std::shared_ptr<ServerChunkMap>>* chunkmap;

    ServerPhysicsEngine(std::map<Vec3<float>, std::shared_ptr<ServerChunkMap>>* worldmap);
    void registerObject(std::shared_ptr<ServerEntity> object, float mass);
    void unregisterObject(std::shared_ptr<ServerEntity> object);
    void addVelocityRotation(std::shared_ptr<ServerEntity> object, Vec3<float> velocity);
    void addVelocityClampedRotation(std::shared_ptr<ServerEntity> object, Vec3<float> velocity, Vec3<float> limit);
    void addVelocity(std::shared_ptr<ServerEntity> object, Vec3<float> velocity);
    void addVelocityClamped(std::shared_ptr<ServerEntity> object, Vec3<float> velocity, Vec3<float> limit);
    void setVelocity(std::shared_ptr<ServerEntity> object, Vec3<float> velocity);
    bool isOnFoot(std::shared_ptr<ServerEntity> object);
    ServerRaycastResult raycast(float length, Vec3<float> startpos, Vec3<float> rotation);
    Vec3<float> getVelocity(std::shared_ptr<ServerEntity> object);
    void step();

    bool isColliding(Vec3<float> object1, Vec3<float> object2, Vec3<float> collider1, Vec3<float> collider2);
    void checkEntityChunk(std::shared_ptr<ServerPhysicsObject> entity, Vec3<float> prevpos);
    std::vector<std::shared_ptr<ServerPhysicsObject>> possibleEntities(Vec3<float> position);
    bool possibleCollision(Vec3<float> position, Vec3<float> collider, const Block& object2);
    bool possibleEntityCollision(Vec3<float> position, Vec3<float> collider, std::shared_ptr<ServerEntity> object2);
    std::vector<Block> possibleObstacles(Vec3<float> position);
    void calculateVelocity(std::shared_ptr<ServerPhysicsObject> obj);
};

