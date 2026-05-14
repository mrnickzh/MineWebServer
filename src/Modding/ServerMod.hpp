#pragma once

#include "../lib/json/json.hpp"
#include "../../lib/sol/sol.hpp"

#include <fstream>
#include <iostream>

class ServerMod {
public:
    std::string modName;
    sol::state mainLua;

    std::map<std::string, int> modBlocks;

    ServerMod(std::string modName);
    void loadAssets();
    void loadMainLua();
    void doEvent(std::string event);
};
