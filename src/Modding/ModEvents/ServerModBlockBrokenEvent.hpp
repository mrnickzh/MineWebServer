#pragma once
#include "Modding/ServerModEvent.hpp"

class ServerModBlockBrokenEvent : public ServerModEvent {
public:
    ~ServerModBlockBrokenEvent() override = default;

    std::string eventid = "blockBrokenEvent";
    int x;
    int y;
    int z;
};
