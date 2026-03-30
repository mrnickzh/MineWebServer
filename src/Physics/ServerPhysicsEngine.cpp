#include "ServerPhysicsEngine.hpp"

#include <algorithm>
#include <iostream>

#include "Server.hpp"
#include "../Utils/ServerGetAABB.hpp"
#include "Utils/ServerChunkMap.hpp"

ServerPhysicsEngine::ServerPhysicsEngine(std::map<Vec3<float>, std::shared_ptr<ServerChunkMap>>* worldmap) {
    chunkmap = worldmap;
}

bool ServerPhysicsEngine::isColliding(Vec3<float> object1, Vec3<float> object2, Vec3<float> collider1, Vec3<float> collider2) {
    ServerAABB obj1 = GetAABB::CP2AABB(collider1, object1);
    ServerAABB obj2 = GetAABB::CP2AABB(collider2, object2);

    bool xcheck1 = obj1.AA.x < obj2.BB.x;
    bool x1check1 = obj1.BB.x > obj2.AA.x;
    bool ycheck1 = obj1.AA.y < obj2.BB.y;
    bool y1check1 = obj1.BB.y > obj2.AA.y;
    bool zcheck1 = obj1.AA.z < obj2.BB.z;
    bool z1check1 = obj1.BB.z > obj2.AA.z;
    if (xcheck1 && ycheck1 && x1check1 && y1check1 && zcheck1 && z1check1) {
        // std::cout << "====================" << std::endl;
        // std::cout << object1.x << " " << object1.y << std::endl;
        // std::cout << collider1.x << " " << collider1.y << std::endl;
        // std::cout << object2.x << " " << object2.y << std::endl;
        // std::cout << collider2.x << " " << collider2.y << std::endl;
        return true;
    }
    return false;
}

bool ServerPhysicsEngine::possibleCollision(Vec3<float> position, Vec3<float> collider, const Block& object2) {
    if (!object2.cancollide) { return false; }
    return isColliding(position, object2.position, collider, object2.collider);
}

std::vector<Block> ServerPhysicsEngine::possibleObstacles(Vec3<float> position) {
    std::vector<Block> obstacles;
    Vec3<float> currentChunk = Vec3<float>(floor(position.x / 8.0f), floor(position.y / 8.0f), floor(position.z / 8.0f));
    Vec3<float> currentChunkBlock = Vec3<float>(floor(std::fmod(position.x, 8.0f)), floor(std::fmod(position.y, 8.0f)), floor(std::fmod(position.z, 8.0f)));
    currentChunkBlock.x += (currentChunk.x < 0.0f ? (currentChunkBlock.x == 0.0f ? 0.0f : 8.0f) : 0.0f);
    currentChunkBlock.y += (currentChunk.y < 0.0f ? (currentChunkBlock.y == 0.0f ? 0.0f : 8.0f) : 0.0f);
    currentChunkBlock.z += (currentChunk.z < 0.0f ? (currentChunkBlock.z == 0.0f ? 0.0f : 8.0f) : 0.0f);
    // std::cout << currentChunkBlock.x << " " << currentChunkBlock.y << " " << currentChunkBlock.z << " " << currentChunk.x << " " << currentChunk.y << " " << currentChunk.z << std::endl;
    // std::cout << "=====BEGIN=====" << std::endl;
    for (int i = (int)currentChunkBlock.x - 1; i <= (int)currentChunkBlock.x + 1; i++) {
        Vec3<float> collisionChunkX = currentChunk;
        Vec3<float> collisionChunkBlockX = currentChunkBlock;
        collisionChunkBlockX.x = (float)i;
        if (i < 0) { collisionChunkX.x -= 1.0f; collisionChunkBlockX.x += 8.0f; }
        if (i > 7) { collisionChunkX.x += 1.0f; collisionChunkBlockX.x -= 8.0f; }
        for (int j = (int)currentChunkBlock.y - 1; j <= (int)currentChunkBlock.y + 2; j++) {
            Vec3<float> collisionChunkY = collisionChunkX;
            Vec3<float> collisionChunkBlockY = collisionChunkBlockX;
            collisionChunkBlockY.y = (float)j;
            if (j < 0) { collisionChunkY.y -= 1.0f; collisionChunkBlockY.y += 8.0f; }
            if (j > 7) { collisionChunkY.y += 1.0f; collisionChunkBlockY.y -= 8.0f; }
            for (int k = (int)currentChunkBlock.z - 1; k <= (int)currentChunkBlock.z + 1; k++) {
                Vec3<float> collisionChunkZ = collisionChunkY;
                Vec3<float> collisionChunkBlockZ = collisionChunkBlockY;
                collisionChunkBlockZ.z = (float)k;
                if (k < 0) { collisionChunkZ.z -= 1.0f; collisionChunkBlockZ.z += 8.0f; }
                if (k > 7) { collisionChunkZ.z += 1.0f; collisionChunkBlockZ.z -= 8.0f; }
                if ((*chunkmap).find(collisionChunkZ) == (*chunkmap).end()) { return obstacles; }
                std::shared_ptr<Block> realblock = (*chunkmap)[collisionChunkZ]->getBlock(collisionChunkBlockZ);
                Vec3<float> abspos = Vec3<float>(realblock->position.x + (collisionChunkZ.x * 8.0f), realblock->position.y + (collisionChunkZ.y * 8.0f), realblock->position.z + (collisionChunkZ.z * 8.0f));
                obstacles.push_back(Block(realblock->id, abspos, realblock->rotation, realblock->cancollide, realblock->collider));
                // std::cout << collisionChunkZ.x << ", " << collisionChunkZ.y << ", " << collisionChunkZ.z << ", " << collisionChunkBlockZ.x << ", " << collisionChunkBlockZ.y << ", " << collisionChunkBlockZ.z << std::endl;
            }
        }
    }
    // std::cout << "=====END=====" << std::endl;
    return obstacles;
}

void ServerPhysicsEngine::calculateVelocity(std::shared_ptr<ServerPhysicsObject>& obj) {
    Vec3<float> vel = obj->velocity;
    Vec3<float> pos = obj->getPosition();

    // std::cout << vel.x << " " << vel.y << " " << vel.z << " vel" << std::endl;
    // std::cout << pos.x << " " << pos.y << " " << pos.z << " pos" << std::endl;

    bool YCollision = false;
    bool XCollision = false;
    bool ZCollision = false;

    std::vector<Block> obstacles = possibleObstacles(pos);
    if (obstacles.size() < 36) {
        return;
    }

    float inertiaAdjusted = 0.002f * obj->mass;
    float gravityAdjusted = 0.003f * obj->mass;

    // std::cout << vel.x << " " << (inertiaAdjusted * (std::abs(vel.x) / rate)) << " " << (std::abs(vel.x) / rate) << " x" << std::endl;
    // std::cout << vel.z << " " << (inertiaAdjusted * (std::abs(vel.z) / rate)) << " " << (std::abs(vel.z) / rate) << " z" << std::endl;

    for (auto obstacle : obstacles) {
        // printf("%f %f %f %d %d\n", obstacle->position.x, obstacle->position.y, obstacle->position.z, obstacle->id, (int)obstacle->cancollide);
        if (possibleCollision(Vec3<float>(pos.x + vel.x, pos.y, pos.z), obj->getCollider(), obstacle)) { XCollision = true; }
        if (possibleCollision(Vec3<float>(pos.x, pos.y, pos.z + vel.z), obj->getCollider(), obstacle)) { ZCollision = true; }
        if (possibleCollision(Vec3<float>(pos.x, pos.y + vel.y, pos.z), obj->getCollider(), obstacle)) { YCollision = true; }
        // std::cout << XCollision << " " << YCollision << " " << ZCollision << " col" << std::endl;
    }

    if (XCollision) { vel.x = 0.0f; }
    if (YCollision) { vel.y = 0.0f; }
    if (ZCollision) { vel.z = 0.0f; }

    for (auto obstacle : obstacles) {
        if (possibleCollision(Vec3<float>(pos.x + vel.x, pos.y, pos.z + vel.z), obj->getCollider(), obstacle)) { XCollision = true; ZCollision = true; }
        if (possibleCollision(Vec3<float>(pos.x + vel.x, pos.y + vel.y, pos.z), obj->getCollider(), obstacle)) { XCollision = true; YCollision = true; }
        if (possibleCollision(Vec3<float>(pos.x, pos.y + vel.y, pos.z + vel.z), obj->getCollider(), obstacle)) { YCollision = true; ZCollision = true; }
    }

    if (!XCollision) { pos.x += vel.x; }
    else { vel.x = 0.0f; }
    if (!ZCollision) { pos.z += vel.z; }
    else { vel.z = 0.0f; }
    if (!YCollision) { pos.y += vel.y; }
    else { vel.y = 0.0f; }

    float rate = (float)(std::sqrt(std::pow(std::abs(vel.x), 2) + std::pow(std::abs(vel.z), 2)));

    // std::cout << (inertiaAdjusted * (std::abs(vel.x) / rate)) << " x" << std::endl;
    // std::cout << (inertiaAdjusted * (std::abs(vel.z) / rate)) << " z" << std::endl;

    if (rate != 0.0f) {
        if (vel.x < 0) { vel.x += (inertiaAdjusted * (std::abs(vel.x) / rate)); }
        if (vel.x > 0) { vel.x -= (inertiaAdjusted * (std::abs(vel.x) / rate)); }
        if (vel.z < 0) { vel.z += (inertiaAdjusted * (std::abs(vel.z) / rate)); }
        if (vel.z > 0) { vel.z -= (inertiaAdjusted * (std::abs(vel.z) / rate)); }
        if (vel.x < inertiaAdjusted && vel.x > -inertiaAdjusted) { vel.x = 0.0f; }
        if (vel.z < inertiaAdjusted && vel.z > -inertiaAdjusted) { vel.z = 0.0f; }
    }

    vel.y -= gravityAdjusted;

    // std::cout << XCollision << " " << YCollision << " " << ZCollision << std::endl;
    // std::cout << vel.x << " " << vel.y << " " << vel.z << std::endl;

    obj->setPosition(pos);
    // std::cout << obj->getPosition().x << " " << obj->getPosition().y << " " << obj->getPosition().z << std::endl;
    obj->velocity = vel;
}

void ServerPhysicsEngine::registerObject(std::shared_ptr<ServerEntity> object, float mass) {
    std::shared_ptr<ServerPhysicsObject> physicsBlock = std::make_shared<ServerPhysicsObject>(object, mass);
    registeredObjects.push_back(physicsBlock);
}

void ServerPhysicsEngine::unregisterObject(std::shared_ptr<ServerEntity> object) {

    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) { registeredObjects.erase(it); }
}

void ServerPhysicsEngine::addVelocityRotation(std::shared_ptr<ServerEntity> object, Vec3<float> velocity) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) {
        Vec3<float> rotation = Vec3<float>(object->rotation.x * (M_PI / 180.0), object->rotation.y * (M_PI / 180.0), object->rotation.z * (M_PI / 180.0));
        Vec3<float> real_velocity = Vec3<float>(0.0f, 0.0f, 0.0f);
        real_velocity.x = (velocity.x * std::cos(rotation.y) * std::cos(rotation.z)) + (velocity.z * -std::sin(rotation.y) * std::cos(rotation.z));
        real_velocity.y = velocity.y * std::sin(rotation.z);
        real_velocity.z = (velocity.x * std::sin(rotation.y) * std::cos(rotation.z)) + (velocity.z * std::cos(rotation.y) * std::cos(rotation.z));
        it->get()->velocity = Vec3<float>(it->get()->velocity.x + real_velocity.x, it->get()->velocity.y + real_velocity.y, it->get()->velocity.z + real_velocity.z);
    }
}

void ServerPhysicsEngine::addVelocityClampedRotation(std::shared_ptr<ServerEntity> object, Vec3<float> velocity, Vec3<float> limit) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) {
        Vec3<float> rotation = Vec3<float>(object->rotation.x * (M_PI / 180.0), object->rotation.y * (M_PI / 180.0), object->rotation.z * (M_PI / 180.0));

        // std::cout << rotation.x << " " << rotation.y << " " << rotation.z << std::endl;
        // std::cout << rotation.y << " " << velocity.x * std::sin(rotation.y) << std::endl;
        // std::cout << rotation.y << " " << velocity.x << " " << std::sin(rotation.y) << std::endl;
        // std::cout << rotation.y << " " << dvelocity.x * std::cosl(rotation.y) * std::cosl(rotation.z) << std::endl;
        // std::cout << rotation.y << " " << dvelocity.z * std::sinl(rotation.y) * std::cosl(rotation.z) << std::endl;
        // std::cout << rotation.y << " " << dvelocity.z * std::cosl(rotation.y) * std::cosl(rotation.z) << std::endl;

        // std::cout << "=====BEGIN=====" << std::endl;
        // std::cout << velocity.x * std::cos(rotation.y) * std::cos(rotation.z) << " xx" << std::endl;
        // std::cout << velocity.z * -std::sin(rotation.y) * std::cos(rotation.z) << " xz" << std::endl;
        // std::cout << "=====" << std::endl;
        // std::cout << velocity.z * std::cos(rotation.y) * std::cos(rotation.z) << " zz" << std::endl;
        // std::cout << velocity.x * std::sin(rotation.y) * std::cos(rotation.z) << " zx" << std::endl;

        Vec3<float> real_velocity = Vec3<float>(0.0f, 0.0f, 0.0f);
        real_velocity.x = (velocity.x * std::cos(rotation.y) * std::cos(rotation.z)) + (velocity.z * -std::sin(rotation.y) * std::cos(rotation.z));
        real_velocity.y = velocity.y * std::sin(rotation.z);
        real_velocity.z = (velocity.x * std::sin(rotation.y) * std::cos(rotation.z)) + (velocity.z * std::cos(rotation.y) * std::cos(rotation.z));

        float rate = (float)(std::sqrt(std::pow(std::abs(real_velocity.x), 2) + std::pow(std::abs(real_velocity.z), 2)));
        float limitX = limit.x * (std::abs(real_velocity.x) / rate);
        float limitZ = limit.z * (std::abs(real_velocity.z) / rate);
        float limitY = limit.y;

        real_velocity.x = std::clamp(it->get()->velocity.x + real_velocity.x, -std::abs(limitX), std::abs(limitX));
        real_velocity.y = std::clamp(it->get()->velocity.y + real_velocity.y, -std::abs(limitY), std::abs(limitY));
        real_velocity.z = std::clamp(it->get()->velocity.z + real_velocity.z, -std::abs(limitZ), std::abs(limitZ));

        // std::cout << "=====" << std::endl;
        // std::cout << limitX << " " << limitZ << std::endl;
        // std::cout << (velocity.x * std::cos(rotation.y) * std::cos(rotation.z)) + (velocity.z * std::sin(rotation.y) * std::cos(rotation.z)) << " " << (velocity.x * std::sin(rotation.y) * std::cos(rotation.z)) + (velocity.z * std::cos(rotation.y) * std::cos(rotation.z)) << std::endl;
        // std::cout << real_velocity.x << " " << real_velocity.y << " " << real_velocity.z << std::endl;

        it->get()->velocity = Vec3<float>(real_velocity.x, real_velocity.y, real_velocity.z);
    }}

void ServerPhysicsEngine::setVelocity(std::shared_ptr<ServerEntity> object, Vec3<float> velocity) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) { it->get()->velocity = velocity; }
}

void ServerPhysicsEngine::addVelocity(std::shared_ptr<ServerEntity> object, Vec3<float> velocity) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) { it->get()->velocity = Vec3<float>(it->get()->velocity.x + velocity.x, it->get()->velocity.y + velocity.y, it->get()->velocity.z + velocity.z); }
}

void ServerPhysicsEngine::addVelocityClamped(std::shared_ptr<ServerEntity> object, Vec3<float> velocity, Vec3<float> limit) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) {
        it->get()->velocity = Vec3<float>(std::clamp(it->get()->velocity.x + velocity.x, -limit.x, limit.x), std::clamp(it->get()->velocity.y + velocity.y, -limit.y, limit.y), std::clamp(it->get()->velocity.z + velocity.z, -limit.z, limit.z));
    }
}

Vec3<float> ServerPhysicsEngine::getVelocity(std::shared_ptr<ServerEntity> object) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) { return it->get()->velocity; }
    return Vec3<float>(0.0f, 0.0f, 0.0f);
}

bool ServerPhysicsEngine::isOnFoot(std::shared_ptr<ServerEntity> object) {
    auto it = std::find_if(registeredObjects.begin(), registeredObjects.end(), [&object](const std::shared_ptr<ServerPhysicsObject>& obj) {return obj->object == object; });
    if (it != registeredObjects.end()) {
        std::vector<Block> obstacles = possibleObstacles(it->get()->getPosition());
        for (auto &obstacle : obstacles) {
            if (!obstacle.cancollide) { continue; }

            ServerAABB obj1 = GetAABB::CP2AABB(object->collider, object->position);
            ServerAABB obj2 = GetAABB::CP2AABB(obstacle.collider, obstacle.position);

            bool footcheckd = obj1.AA.y - 0.01f <= obj2.BB.y;
            bool footchecku = obj1.AA.y + 0.01f >= obj2.BB.y;
            bool xcheck1 = obj1.AA.x < obj2.BB.x;
            bool x1check1 = obj1.BB.x > obj2.AA.x;
            bool zcheck1 = obj1.AA.z < obj2.BB.z;
            bool z1check1 = obj1.BB.z > obj2.AA.z;

            // std::cout << footcheckd << " " << footchecku << std::endl;

            if (footcheckd && footchecku && xcheck1 && x1check1 && zcheck1 && z1check1) { return true; }
        }
    }
    return false;
}

ServerRaycastResult ServerPhysicsEngine::raycast(float length, Vec3<float> startpos, Vec3<float> rotation) {
    ServerRaycastResult result(false, length, Vec3<float>(0.0f, 0.0f, 0.0f), Vec3<float>(0.0f, 0.0f, 0.0f), nullptr, Vec3<float>(0.0f, 0.0f, 0.0f), Vec3<float>(0.0f, 0.0f, 0.0f), nullptr);

    rotation = Vec3<float>(rotation.x * (M_PI / 180.0), rotation.y * (M_PI / 180.0), rotation.z * (M_PI / 180.0));
    Vec3<float> raystep = Vec3<float>(0.2f, 0.2f, 0.0f);
    Vec3<float> direction = Vec3<float>(0.0f, 0.0f, 0.0f);
    int steps = 0;

    direction.x = (raystep.x * std::cos(rotation.y) * std::cos(rotation.z)) + (raystep.z * -std::sin(rotation.y) * std::cos(rotation.z));
    direction.y = raystep.y * std::sin(rotation.z);
    direction.z = (raystep.x * std::sin(rotation.y) * std::cos(rotation.z)) + (raystep.z * std::cos(rotation.y) * std::cos(rotation.z));

    Vec3<float> endpos = Vec3<float>(direction.x * std::round(length / 0.2f), direction.y * std::round(length / 0.2f), direction.z * std::round(length / 0.2f));

    while (steps < (int)std::round(length / 0.2f)) {
        Vec3<float> position = startpos + Vec3<float>(direction.x * (float)steps, direction.y * (float)steps, direction.z * (float)steps);

        Vec3<float> currentChunk = Vec3<float>(floor((position.x + 0.5f) / 8.0f), floor((position.y + 0.5f) / 8.0f), floor((position.z + 0.5f) / 8.0f));
        Vec3<float> currentChunkBlock = Vec3<float>((int)round(std::fmod(position.x, 8.0f)), (int)round(std::fmod(position.y, 8.0f)), (int)round(std::fmod(position.z, 8.0f)));
        // std::cout << currentChunk.x << ", " << currentChunk.y << ", " << currentChunk.z << std::endl;
        // std::cout << currentChunkBlock.x << ", " << currentChunkBlock.y << ", " << currentChunkBlock.z << std::endl;
        currentChunkBlock.x += (currentChunk.x < 0.0f ? (currentChunkBlock.x == 0.0f ? 0.0f : 8.0f) : 0.0f);
        currentChunkBlock.y += (currentChunk.y < 0.0f ? (currentChunkBlock.y == 0.0f ? 0.0f : 8.0f) : 0.0f);
        currentChunkBlock.z += (currentChunk.z < 0.0f ? (currentChunkBlock.z == 0.0f ? 0.0f : 8.0f) : 0.0f);

        currentChunkBlock.x -= (currentChunk.x > 0.0f ? (currentChunkBlock.x == 8.0f ? 8.0f : 0.0f) : 0.0f);
        currentChunkBlock.y -= (currentChunk.y > 0.0f ? (currentChunkBlock.y == 8.0f ? 8.0f : 0.0f) : 0.0f);
        currentChunkBlock.z -= (currentChunk.z > 0.0f ? (currentChunkBlock.z == 8.0f ? 8.0f : 0.0f) : 0.0f);
        // std::cout << currentChunk.x << ", " << currentChunk.y << ", " << currentChunk.z << std::endl;
        // std::cout << currentChunkBlock.x << ", " << currentChunkBlock.y << ", " << currentChunkBlock.z << std::endl;
        // std::cout << "==============" << std::endl;
        Vec3<float> collisionChunk = currentChunk;
        Vec3<float> collisionChunkBlock = currentChunkBlock;
        // if (currentChunk.x < 0) { collisionChunk.x -= 1.0f; collisionChunkBlock.x += 8.0f; }
        // if (currentChunk.x > 7) { collisionChunk.x += 1.0f; collisionChunkBlock.x -= 8.0f; }
        // if (currentChunk.y < 0) { collisionChunk.y -= 1.0f; collisionChunkBlock.y += 8.0f; }
        // if (currentChunk.y > 7) { collisionChunk.y += 1.0f; collisionChunkBlock.y -= 8.0f; }
        // if (currentChunk.z < 0) { collisionChunk.z -= 1.0f; collisionChunkBlock.z += 8.0f; }
        // if (currentChunk.z > 7) { collisionChunk.z += 1.0f; collisionChunkBlock.z -= 8.0f; }
        // std::cout << collisionChunk.x << ", " << collisionChunk.y << ", " << collisionChunk.z << std::endl;
        // std::cout << collisionChunkBlock.x << ", " << collisionChunkBlock.y << ", " << collisionChunkBlock.z << std::endl;
        // std::cout << position.x << " " << position.y << " " << position.z << std::endl;
        if ((*chunkmap).find(collisionChunk) != (*chunkmap).end()) {
            std::shared_ptr<Block> object2 = (*chunkmap)[collisionChunk]->getBlock(collisionChunkBlock);
            // std::cout << object2->position.x << " " << object2->position.y << " " << object2->position.z << " " << object2->cancollide << std::endl;
            if (object2->cancollide) {
                result.hit = true;
                result.distance = std::sqrt(std::pow(startpos.x - object2->position.x, 2) + std::pow(startpos.y - object2->position.y, 2) + std::pow(startpos.z - object2->position.z, 2));
                result.blockpos = collisionChunkBlock;
                result.chunkpos = collisionChunk;
                result.object = object2;
                break;
            }
            else {
                // std::cout << collisionChunk.x << ", " << collisionChunk.y << ", " << collisionChunk.z << std::endl;
                // std::cout << collisionChunkBlock.x << ", " << collisionChunkBlock.y << ", " << collisionChunkBlock.z << std::endl;
                result.prevblockpos = collisionChunkBlock;
                result.prevchunkpos = collisionChunk;
                result.prevobject = object2;
            }
        }

        if (std::sqrt(std::pow(position.x - startpos.x, 2) + std::pow(position.y - startpos.y, 2) + std::pow(position.z - startpos.z, 2)) > std::sqrt(std::pow(endpos.x, 2) + std::pow(endpos.y, 2) + std::pow(endpos.z, 2))) {
            break;
        }

        steps++;
    }
    return result;
}

void ServerPhysicsEngine::step() {
    for (std::shared_ptr<ServerPhysicsObject> object : registeredObjects) {
        calculateVelocity(object);
    }
}