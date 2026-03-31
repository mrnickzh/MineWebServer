#pragma once

#include "Server.hpp"
#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacket.hpp"
#include "Utils/ServerEntity.hpp"

class ChatMessageServer : public ServerPacket {
public:
    std::string message;

    void receive(ByteBuf &buffer) override {
        message = buffer.readString();
    }

    void send(ByteBuf &buffer) override {
        buffer.writeString(message);
    }

    void process(ClientSession* session) override {
        for (auto& s : Server::getInstance().clients) {
            ChatMessageServer packet;
            packet.message = "[" + session->username + "]: " + message;
            Server::getInstance().sendPacket(s.first, &packet);
        }
    }
};
