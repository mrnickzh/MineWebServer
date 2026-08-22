#pragma once
#include <map>
#include <string>

#include "ServerMod.hpp"

class ServerModManager {
public:
    std::unordered_map<std::string, ServerMod*> mods;

    ServerModManager();
    void initLoad();
    void loadMod(std::string& modname);
};

