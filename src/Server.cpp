#include "Server.hpp"

#include <iostream>
#include <mutex>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacketHelper.hpp"
#include "Protocol/Packets/GenerateChunkServer.hpp"
#include "Protocol/Packets/HandShakePacketServer.hpp"

void Server::setCallback(std::function<void(ClientSession*, std::vector<uint8_t>)> callback) {
    this->callback = std::move(callback);
    ServerPacketHelper::registerPacket(0, []() { return new HandShakePacketServer(); });
    ServerPacketHelper::registerPacket(1, []() { return new EditChunkServer(); });
    ServerPacketHelper::registerPacket(2, []() { return new GenerateChunkServer(); });
    ServerPacketHelper::registerPacket(3, []() { return new EntityActionServer(); });
    ServerPacketHelper::registerPacket(4, []() { return new PlayerAuthInputServer(); });
    ServerPacketHelper::registerPacket(5, []() { return new LightMapServer(); });

    if (!std::filesystem::exists("regions")) {
        std::filesystem::create_directory("regions");
    }

#ifndef BUILD_TYPE_DEDICATED
    std::thread packetprocessor([&]() {
        while (true) {
            // std::cout << serverPacketQueue.size() << " queue" << std::endl;
            while (!serverPacketQueue.empty()) {
                std::lock_guard<std::mutex> guard(serverPacketQueueMutex);
                std::pair<ClientSession*, std::vector<uint8_t>> packet = serverPacketQueue.front();
                ServerPacketHelper::decodePacket(packet.first, packet.second);
                serverPacketQueue.pop_front();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    packetprocessor.detach();
#endif

#ifdef BUILD_TYPE_DEDICATED
    std::thread watcher([&]() {
        int ch = 0;
        while (true) {
            std::set<Vec3<float>> activeRegions;
            for (auto& entity : Server::getInstance().entities) {
                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        for (int z = -1; z <= 1; z++) {
                            activeRegions.insert(Vec3<float>(floor(floor(entity.position.x / 8.0f) / 8.0f) + (float)x, floor(floor(entity.position.y / 8.0f) / 8.0f) + (float)y, floor(floor(entity.position.z / 8.0f) / 8.0f) + (float)z));
                        }
                    }
                }
            }
            auto loadedRegions = RegionRegistory::getInstance().loadedRegions;
            printf("Loaded %llu regions\n", loadedRegions.size());
            printf("Loaded %llu chunks\n", Server::getInstance().chunks.size());
            for (auto& region : loadedRegions) {
                if (activeRegions.find(region) == activeRegions.end()) {
                    printf("Unloading %f %f %f\n", region.x, region.y, region.z);
                    RegionRegistory::getInstance().save(Vec3<float>(region.x * 8.0f, region.y * 8.0f, region.z * 8.0f));
                    RegionRegistory::getInstance().loadedRegions.erase(region);

                    for (int x = 0; x < 8; x++) {
                        for (int y = 0; y < 8; y++) {
                            for (int z = 0; z < 8; z++) {
                                Vec3<float> regionChunk = Vec3<float>((region.x * 8.0f) + (float)x, (region.y * 8.0f) + (float)y, (region.z * 8.0f) + (float)z);
                                if (Server::getInstance().chunks.find(regionChunk) != Server::getInstance().chunks.end()) {
                                    Server::getInstance().chunks.erase(regionChunk);
                                }
                            }
                        }
                    }
                }
            }

            if (ch >= 20) {
                printf("Creaing backup...\n");
                RegionRegistory::getInstance().exportAll();
                ch = 0;
            }

            ch++;
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    });
    watcher.detach();
#endif
}

void Server::processPacket(ClientSession* session, std::vector<uint8_t> data) {
#ifndef BUILD_TYPE_DEDICATED
    std::lock_guard<std::mutex> guard(serverPacketQueueMutex);
    serverPacketQueue.push_back(std::pair<ClientSession*, std::vector<uint8_t>>(session, data));
#else
    ServerPacketHelper::decodePacket(session, data);
#endif
}

void Server::sendPacket(ClientSession* session, ServerPacket* packet) {
    std::vector<uint8_t> data = ServerPacketHelper::encodePacket(packet);
    callback(session, data);
}

#ifndef BUILD_TYPE_DEDICATED
void Server::saveWorld() {
    RegionRegistory::getInstance().exportAll();
}

void Server::loadWorld() {
    RegionRegistory::getInstance().importAll();
}
#endif