#include "ServerMod.hpp"

#include "Server.hpp"

#ifdef BUILD_TYPE_DEDICATED
std::string moddir = "mods/";
#else
std::string moddir = "/mods/";
#endif

ServerMod::ServerMod(std::string modName) {
    this->modName = modName;
}

void ServerMod::loadAssets() {
    nlohmann::json assets;
    std::ifstream manifestfile(moddir + modName + "/manifest.json");

    if (!manifestfile) {
        std::cout << "mod " + modName + " assets manifest not found" << std::endl;
    } else manifestfile >> assets;

    for (auto& element : assets["blocks"].items()) {
        nlohmann::json mblock = element.value();
        std::string id = mblock["id"];
        bool cancollide = mblock["cancollide"];
        int lightlevel = mblock["lightlevel"];
        int realId = Server::getInstance().maxblockid++;
        Block block = Block(realId, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), cancollide, glm::vec3(0.5f, 0.5f, 0.5f));
        if (lightlevel > 0 ) {
            block.lightlevel = lightlevel;
        }
        Server::getInstance().serverBlockRegistry->registerBlock(realId, block, modName, id);
        modBlocks[id] = realId;
    }

    // for (auto& element : assets["entities"].items()) {
    //     nlohmann::json entity = element.value();
    //     int id = entity["id"];
    // }
}

void ServerMod::loadMainLua() {
    mainLua.open_libraries(sol::lib::base);

    std::ifstream file(moddir + modName + "/server/main.lua");
    std::stringstream script;
    script << file.rdbuf();

    mainLua.script(script.str());
}

void ServerMod::doEvent(std::string event) {
    std::cout << event << std::endl;
    sol::protected_function func = mainLua[event];
    sol::protected_function_result result = func();
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "Error: " << err.what() << std::endl;
    }
}


