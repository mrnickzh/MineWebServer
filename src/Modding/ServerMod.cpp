#include "ServerMod.hpp"

#include "Server.hpp"
#include "ServerModBridge.hpp"
#include "ServerModEvent.hpp"
#include "ModEvents/ServerModBlockBrokenEvent.hpp"

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
    mainLua.set_function("GAME_registerEvent",
    [this](std::string eventId, sol::protected_function func) {
        this->registerEvent(eventId, func);
    });
    mainLua.set_function("GAME_setBlock", &ServerModBridge::setBlock);

    mainLua.new_usertype<ServerModEvent>("ServerModEvent",
        "cancel", &ServerModEvent::cancel
    );
    mainLua.new_usertype<ServerModBlockBrokenEvent>("ServerModBlockBrokenEvent",
    sol::base_classes, sol::bases<ServerModEvent>(),
        "cancel", &ServerModEvent::cancel,
        "x", &ServerModBlockBrokenEvent::x, "y", &ServerModBlockBrokenEvent::y, "z", &ServerModBlockBrokenEvent::z
    );

    std::ifstream file(moddir + modName + "/server/main.lua");
    std::stringstream script;
    script << file.rdbuf();

    mainLua.script(script.str());
}

void ServerMod::registerEvent(std::string eventId, sol::protected_function func) {
    modEvents[eventId] = func;
}


