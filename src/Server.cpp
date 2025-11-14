#include "Server.hpp"

#include <iostream>
#include "Protocol/ServerPacketHelper.hpp"
#include "Protocol/Packets/HandShakePacketServer.hpp"

void Server::setCallback(std::function<void(std::vector<uint8_t>)> callback) {
    this->callback = std::move(callback);
    ServerPacketHelper::registerPacket(0, []() { return new HandShakePacketServer(); });
    ServerPacketHelper::registerPacket(1, []() { return new AddMapObjectServer(); });
}

void Server::processPacket(std::vector<uint8_t> data) {
    ServerPacketHelper::decodePacket(data);
}

void Server::sendPacket(Packet* packet) {
    std::vector<uint8_t> data = ServerPacketHelper::encodePacket(packet);
    callback(data);
}
