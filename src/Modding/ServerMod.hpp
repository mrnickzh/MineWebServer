#pragma once

#include "../lib/json/json.hpp"
#include "../../lib/sol/sol.hpp"

#include <typeindex>
#include <fstream>
#include <iostream>

#include "ServerModEvent.hpp"

class ServerMod {
public:
    std::string modName;
    sol::state mainLua;

    std::unordered_map<std::string, int> modBlocks;
    std::unordered_map<std::string, sol::protected_function> modEvents;

    ServerMod(std::string modName);
    void loadAssets();
    void loadMainLua();
    void registerEvent(std::string eventId, sol::protected_function func);

    template<typename ModEventType>
    void doEvent(ModEventType* event) {
        std::string eventId = event->eventid;
        std::cout << eventId << std::endl;

        if (modEvents.find(eventId) == modEvents.end()) { return; }
        sol::protected_function func = modEvents[eventId];

        sol::protected_function_result result = func(event);
        if (!result.valid()) {
            sol::error err = result;
            printf("Server mod '%s' error: %s", modName.c_str(), err.what());
        }

        delete event;
    }
};
