#pragma once
#include <map>
#include <memory>
#include <set>
#include <array>

#include "Block.hpp"
#include "PerlinNoise.hpp"
#include "vec3Comparator.hpp"

typedef std::pair<glm::vec3, glm::vec3> AbsPos;

enum SetArrType {
    NONE,
    ADD,
    REMOVE
};

class ServerChunkMap {
public:
    // std::map<glm::vec3, std::shared_ptr<Block>> blocks;
    std::array<Block, 512> blocks{};

    std::map<AbsPos, int, vec3PairComparator> lightSources;
    std::map<AbsPos, int, vec3PairComparator> ambientSources;

    bool checkValidPos(glm::vec3 pos);
    void addBlock(glm::vec3 blockPos, Block block);
    Block* getBlock(glm::vec3 blockPos);
    void generate(glm::vec3 chunkPos);
    void generateOres(glm::vec3 chunkPos, int oreBlockId, int clusterCount, int clusterSize, int minY, int maxY);
    std::set<glm::vec3, vec3Comparator> simulateLightSource(glm::vec3 chunkPos, Block source, bool collision, int setarrays);
    void checkLights(glm::vec3 chunkPos);
    std::set<glm::vec3, vec3Comparator> simulateAmbientSource(glm::vec3 chunkPos, AbsPos source, bool collision, int setarrays);
    void checkAmbient(glm::vec3 chunkPos);
    std::pair<std::set<glm::vec3, vec3Comparator>, std::pair<AbsPos, AbsPos>> checkHeight(glm::vec3 chunkPos, glm::vec3 blockPos);
    void resetAmbient();
    void resetLights();
};
