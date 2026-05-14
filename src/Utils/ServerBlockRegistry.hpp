#pragma once

#include <map>
#include <string>

#include "Utils/Block.hpp"

class ServerBlockRegistry {
public:
    std::map<int, std::pair<std::pair<std::string, std::string>, Block>> blocks;

    ServerBlockRegistry() = default;
    void registerBlock(int id, Block block, std::string modName, std::string blockId);
    std::pair<std::pair<std::string, std::string>, Block> getBlock(int id);
};

