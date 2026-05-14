#pragma once

#include <WorldSaving/RegionRegistory.hpp>

#include "LightMapServer.hpp"
#include "Server.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerChunkMap.hpp"

class RegisterModServer : public ServerPacket {
public:
    std::string modName;

    void receive(ByteBuf &buffer) override {

    }
    void send(ByteBuf &buffer) override {
        buffer.writeString(modName);
        ServerMod* mod = Server::getInstance().serverModManager->mods[modName];
        buffer.writeInt(mod->modBlocks.size());
        for (auto b : mod->modBlocks) {
            buffer.writeString(b.first);
            buffer.writeInt(b.second);
        }

    }

    void process(ClientSession* session) override {
    }
};
