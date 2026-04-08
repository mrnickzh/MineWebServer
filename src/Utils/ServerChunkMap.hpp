#pragma once
#include <map>
#include <memory>
#include <set>
#include <array>

#include "Block.hpp"
#include "PerlinNoise.hpp"
#include "vec3Comparator.hpp"

class ServerChunkMap {
public:
    // std::map<glm::vec3, std::shared_ptr<Block>> blocks;
    std::array<std::shared_ptr<Block>, 512> blocks;

    bool checkValidPos(glm::vec3 pos);
    void addBlock(glm::vec3 blockPos, std::shared_ptr<Block> block);
    std::shared_ptr<Block> getBlock(glm::vec3 blockPos);
    void generate(glm::vec3 chunkPos);
    void generateOres(glm::vec3 chunkPos, int oreBlockId, int clusterCount, int clusterSize, int minY, int maxY);
    std::set<glm::vec3, vec3Comparator> checkLights(glm::vec3 chunkPos, Block prevblock);
    std::set<glm::vec3, vec3Comparator> checkAmbient(glm::vec3 chunkPos);
    std::set<glm::vec3, vec3Comparator> checkHeight(glm::vec3 chunkPos, glm::vec3 blockPos);
    void resetAmbient();
    void resetLights();
};
