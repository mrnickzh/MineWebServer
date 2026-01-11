#pragma once
#include <functional>
#include <map>
#include <vector>
#include <set>
#include <random>
#include <thread>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/HeightMap.hpp"
#include "Utils/Vec.hpp"
#include "Utils/ServerChunkMap.hpp"
#include "Utils/ServerEntity.hpp"


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

        std::uniform_int_distribution<uint64_t> dist(0, INT64_MAX);

        seedTerrain = dist(random);
        seedStructures = dist(random);
        seedCaves = dist(random);
        seedOres = dist(random);

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

    std::map<Vec3<float>, std::shared_ptr<ServerChunkMap>> chunks;
    std::set<ServerEntity> entities;

    float worldMaxY = 256.0f;
    float worldMinY = -256.0f;

    SeedMap* seedMap;

    Server() {
        seedMap = new SeedMap(32335);
    }

    static Server& getInstance() {
        static Server instance;
        return instance;
    }

#ifndef BUILD_TYPE_DEDICATED
    void saveWorld();
    void loadWorld();
#endif

    void setCallback(std::function<void(ClientSession*, std::vector<uint8_t>)> callback);
    void processPacket(ClientSession* session, std::vector<uint8_t> data);
    void sendPacket(ClientSession* session, ServerPacket* packet);
};
