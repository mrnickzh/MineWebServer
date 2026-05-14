#include "ServerBlockRegistry.hpp"

void ServerBlockRegistry::registerBlock(int id, Block block, std::string modName, std::string blockId) {
    blocks.insert({id, std::pair{std::pair{modName, blockId}, block}});
}

std::pair<std::pair<std::string, std::string>, Block> ServerBlockRegistry::getBlock(int id) {
    auto it = blocks.find(id);
    if (it == blocks.end()) {
        printf("Block id %d not found\n", id);
    }
    return std::pair{it->second.first, it->second.second};
}

