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

    std::thread lightupdater([&]() {
        while (true) {
            // std::cout << lightUpdateQueue.size() << " queue" << std::endl;
            std::set<Vec3<float>> affectedChunks;
            std::set<Vec3<float>> updatedChunks;
            std::deque<std::pair<Vec3<float>, Block>> tempLightQueue;
            for (auto p : Server::getInstance().lightUpdateFallbackQueue) {
                std::lock_guard<std::mutex> guard(lightUpdateFallbackQueueMutex);
                tempLightQueue.push_back(p);
                Server::getInstance().lightUpdateFallbackQueue.pop_front();
            }
            while (!tempLightQueue.empty()) {
                std::lock_guard<std::mutex> guard(lightUpdateQueueMutex);
                lightUpdateQueue.push_back(tempLightQueue.front());
                tempLightQueue.pop_front();
            }
            while (!lightUpdateQueue.empty()) {
                std::lock_guard<std::mutex> guard(lightUpdateQueueMutex);
                std::pair<Vec3<float>, Block> lightChunk = lightUpdateQueue.front();
                {
                    std::set<Vec3<float>> L_affectedChunks;
                    std::set<Vec3<float>> A_affectedChunks;

                    A_affectedChunks = Server::getInstance().chunks[lightChunk.first]->checkHeight(lightChunk.first, lightChunk.second.position);
                    L_affectedChunks = Server::getInstance().chunks[lightChunk.first]->checkLights(lightChunk.first, lightChunk.second);

                    for (auto c : L_affectedChunks) {
                        affectedChunks.insert(c);
                        Server::getInstance().chunks[c]->checkLights(c, Block(0, Vec3<float>(0.0f, 0.0f, 0.0f)));
                    }
                    for (auto c : A_affectedChunks) {
                        affectedChunks.insert(c);
                        std::set<Vec3<float>> chunks = Server::getInstance().chunks[c]->checkAmbient(c);
                        for (auto& a : chunks) {
                            affectedChunks.insert(a);
                        }
                    }
                }
                lightUpdateQueue.pop_front();
            }
            // std::cout << affectedChunks.size() << " size" << std::endl;
            for (auto c : affectedChunks) {
                Server::getInstance().chunks[c]->resetAmbient();
            }
            for (auto c : affectedChunks) {
                std::set<Vec3<float>> chunks = Server::getInstance().chunks[c]->checkAmbient(c);
                for (auto& a : chunks) {
                    updatedChunks.insert(a);
                }
            }
            for (auto c : updatedChunks) {
                LightMapServer lightpacket;
                lightpacket.chunkpos = c;
                for (auto& s : Server::getInstance().clients) {
                    Server::getInstance().sendPacket(s.first, &lightpacket);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    lightupdater.detach();

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
                                    std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
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
    // std::lock_guard<std::mutex> guard(serverPacketQueueMutex);
    if (serverPacketQueueMutex.try_lock()) {
        for (auto p : serverFallbackPacketQueue) {
            serverPacketQueue.push_back(p);
            serverFallbackPacketQueue.pop_front();
        }
        serverPacketQueue.push_back(std::pair<ClientSession*, std::vector<uint8_t>>(session, data));
        serverPacketQueueMutex.unlock();
    }
    else {
        serverFallbackPacketQueue.push_back(std::pair<ClientSession*, std::vector<uint8_t>>(session, data));
    }
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