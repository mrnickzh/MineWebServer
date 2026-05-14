#pragma once

#include <WorldSaving/RegionRegistory.hpp>

#include "LightMapServer.hpp"
#include "Server.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerChunkMap.hpp"

class TransferModServer : public ServerPacket {
    public:
        std::string modName;

    void receive(ByteBuf &buffer) override {

    }
    void send(ByteBuf &buffer) override {
        buffer.writeString(modName);

        std::ifstream file("mods/" + modName + ".zip", std::ifstream::ate | std::ios::binary);

        std::streamsize size = file.tellg();
        file.seekg(0);

        buffer.writeInt(size);

        std::cout << "mod size: " << size << "\n";

        std::vector<uint8_t> filebuf(size);
        file.read(reinterpret_cast<char*>(filebuf.data()), size);
        file.close();

        ByteBuf bb(67108864);
        bb.fromByteArray(filebuf);

        buffer.concat(bb);
        std::cout << buffer.toByteArray().size() << "\n";
    }

    void process(ClientSession* session) override {
    }
};
