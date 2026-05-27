#pragma once
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <vector>
#include <set>
#include <random>
#include <thread>

#include "Physics/ServerPhysicsEngine.hpp"
#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/HeightMap.hpp"
#include "Utils/ServerChunkMap.hpp"
#include "Utils/ServerEntity.hpp"
#include "Utils/vec3Comparator.hpp"

#define GLM_FORCE_PURE
#include "../lib/glm/glm.hpp"
#include "../lib/glm/gtc/matrix_transform.hpp"
#include "../lib/glm/gtc/type_ptr.hpp"
#include "Modding/ServerModManager.hpp"
#include "Utils/ServerBlockRegistry.hpp"

struct SeedMap {
public:
    PerlinNoise* perlinNoiseTerrain;
    PerlinNoise* perlinNoiseStructures;
    PerlinNoise* perlinNoiseCaves;
public:
    uint32_t initialSeed;
private:
    uint64_t seedTerrain;
    uint64_t seedStructures;
    uint64_t seedCaves;
public:
    uint64_t seedOres;
public:
    SeedMap(uint32_t initialSeed) : initialSeed(initialSeed){
        std::mt19937 random(initialSeed);

        seedTerrain = random();
        seedStructures = random();
        seedCaves = random();
        seedOres = random();

        printf("seed terrain %llu \n", (unsigned long long) seedTerrain);
        printf("seed caves %llu \n", (unsigned long long) seedCaves);
        printf("seed ores %llu \n", (unsigned long long) seedOres);

        perlinNoiseTerrain = new PerlinNoise(seedTerrain);
        perlinNoiseStructures = new PerlinNoise(seedStructures);
        perlinNoiseCaves = new PerlinNoise(seedCaves);
    }
};

class Server {
public:
    std::function<void(ClientSession*, std::vector<uint8_t>)> callback;
    std::map<ClientSession*, void*> clients;

    std::map<glm::vec3, std::shared_ptr<ServerChunkMap>, vec3Comparator> chunks;
    std::map<std::string, std::shared_ptr<ServerEntity>> entities;

    std::mutex chunksMutex;

    std::unique_ptr<ServerPhysicsEngine> serverPhysicsEngine;
    std::mutex serverEntityMutex;

    std::mutex lightUpdateQueueMutex;
    std::mutex lightUpdateFallbackQueueMutex;
    std::deque<std::pair<glm::vec3, Block>> lightUpdateQueue;
    std::deque<std::pair<glm::vec3, Block>> lightUpdateFallbackQueue;

#ifndef BUILD_TYPE_DEDICATED
    std::mutex serverPacketQueueMutex;
    std::deque<std::pair<ClientSession*, std::vector<uint8_t>>> serverPacketQueue;
    std::deque<std::pair<ClientSession*, std::vector<uint8_t>>> serverFallbackPacketQueue;
#endif

    ServerBlockRegistry* serverBlockRegistry = new ServerBlockRegistry();
    ServerModManager* serverModManager = new ServerModManager();
    int maxblockid = 0;

    SeedMap* seedMap;

    Server() {
        seedMap = new SeedMap(32335);

        Block air = Block(0, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), false, glm::vec3(0.5f, 0.5f, 0.5f));
        serverBlockRegistry->registerBlock(0, air, "base", std::to_string(0));

        {
            nlohmann::json assets;
#ifndef BUILD_TYPE_DEDICATED
            std::ifstream manifestfile("/assets/manifest.json");
#else
            std::ifstream manifestfile("manifest.json");
#endif

            if (!manifestfile) {
                std::cout << "no assets manifest found" << std::endl;
            } else manifestfile >> assets;

            for (auto& element : assets["blocks"].items()) {
                nlohmann::json mblock = element.value();
                int id = mblock["id"];
                bool cancollide = mblock["cancollide"];
                int lightlevel = mblock["lightlevel"];
                Block block = Block(id, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), cancollide, glm::vec3(0.5f, 0.5f, 0.5f));
                if (lightlevel > 0) {
                    block.lightlevel = lightlevel;
                }
                serverBlockRegistry->registerBlock(id, block, "base", std::to_string(id));
                maxblockid = id + 1;
            }

            // for (auto& element : assets["entities"].items()) {
            //     nlohmann::json entity = element.value();
            //     int id = entity["id"];
            //     std::string texture = entity["texture"];
            //     Main::textureManager->addTexture(texture, id);
            // }
        }
    }

    static Server& getInstance() {
        static Server instance;
        return instance;
    }

#ifndef BUILD_TYPE_DEDICATED
    void saveWorld();
    void loadWorld();
#endif

    void start();
    void setCallback(std::function<void(ClientSession*, std::vector<uint8_t>)> callback);
    void processPacket(ClientSession* session, std::vector<uint8_t> data);
    void sendPacket(ClientSession* session, ServerPacket* packet);
};
