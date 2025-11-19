#include "Server.hpp"

#include <iostream>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacketHelper.hpp"
#include "Protocol/Packets/GenerateChunkServer.hpp"
#include "Protocol/Packets/HandShakePacketServer.hpp"

void Server::setCallback(std::function<void(ClientSession, std::vector<uint8_t>)> callback) {
    this->callback = std::move(callback);
    ServerPacketHelper::registerPacket(0, []() { return new HandShakePacketServer(); });
    ServerPacketHelper::registerPacket(1, []() { return new AddMapObjectServer(); });
    ServerPacketHelper::registerPacket(2, []() { return new GenerateChunkServer(); });
}

void Server::processPacket(ClientSession session, std::vector<uint8_t> data) {
    ServerPacketHelper::decodePacket(session, data);
}

void Server::sendPacket(ClientSession session, ServerPacket* packet) {
    std::vector<uint8_t> data = ServerPacketHelper::encodePacket(packet);
    callback(session, data);
}