#include "Server.hpp"

#include <iostream>
#include <mutex>

#include "Protocol/ClientSession.hpp"
#include "Protocol/ServerPacketHelper.hpp"
#include "Protocol/Packets/ChatMessageServer.hpp"
#include "Protocol/Packets/GenerateChunkServer.hpp"
#include "Protocol/Packets/HandShakePacketServer.hpp"
#include "Protocol/Packets/RegisterModServer.hpp"

void Server::setCallback(std::function<void(ClientSession*, std::vector<uint8_t>)> callback) {
    serverModManager->initLoad();

    this->callback = std::move(callback);
    ServerPacketHelper::registerPacket(0, []() { return new HandShakePacketServer(); });
    ServerPacketHelper::registerPacket(1, []() { return new EditChunkServer(); });
    ServerPacketHelper::registerPacket(2, []() { return new GenerateChunkServer(); });
    ServerPacketHelper::registerPacket(3, []() { return new EntityActionServer(); });
    ServerPacketHelper::registerPacket(4, []() { return new PlayerAuthInputServer(); });
    ServerPacketHelper::registerPacket(5, []() { return new LightMapServer(); });
    ServerPacketHelper::registerPacket(6, []() { return new ChatMessageServer(); });
    ServerPacketHelper::registerPacket(7, []() { return new NetworkSettingsPacketServer(); });
    ServerPacketHelper::registerPacket(8, []() { return new TransferModServer(); });
    ServerPacketHelper::registerPacket(9, []() { return new RegisterModServer(); });

    this->serverPhysicsEngine = std::make_unique<ServerPhysicsEngine>(&this->chunks);

    if (!std::filesystem::exists("regions")) {
        std::filesystem::create_directory("regions");
    }
}

void Server::start() {
    std::thread lightthread([&]() {
        while (true) {
            // std::cout << lightUpdateQueue.size() << " queue" << std::endl;
            std::set<glm::vec3, vec3Comparator> L_updatedChunks;
            std::set<glm::vec3, vec3Comparator> A_updatedChunks;
            std::set<glm::vec3, vec3Comparator> updatedChunks;
            std::deque<std::pair<glm::vec3, Block>> tempLightQueue;
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
                std::pair<glm::vec3, Block> lightChunk = lightUpdateQueue.front();

                std::set<glm::vec3, vec3Comparator> L_affectedChunks;
                if (lightChunk.second.id == 5) { L_affectedChunks = Server::getInstance().chunks[lightChunk.first]->simulateLightSource(lightChunk.first, lightChunk.second, false, REMOVE); }
                else if (Server::getInstance().chunks[lightChunk.first]->getBlock(lightChunk.second.position)->id == 5) { L_affectedChunks = Server::getInstance().chunks[lightChunk.first]->simulateLightSource(lightChunk.first, lightChunk.second, false, ADD); }
                else { L_affectedChunks = Server::getInstance().chunks[lightChunk.first]->simulateLightSource(lightChunk.first, lightChunk.second, false, NONE); }
                L_updatedChunks.insert(L_affectedChunks.begin(), L_affectedChunks.end());

                auto heightResult = Server::getInstance().chunks[lightChunk.first]->checkHeight(lightChunk.first, lightChunk.second.position);
                if (heightResult.second != std::make_pair(std::make_pair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, -1.0f, -1.0f)), std::make_pair(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, -1.0f, -1.0f)))) {
                    std::set<glm::vec3, vec3Comparator> removeResult = Server::getInstance().chunks[lightChunk.first]->simulateAmbientSource(lightChunk.first, heightResult.second.second, false, REMOVE);
                    std::set<glm::vec3, vec3Comparator> addResult = Server::getInstance().chunks[lightChunk.first]->simulateAmbientSource(lightChunk.first, heightResult.second.first, false, ADD);
                    A_updatedChunks.insert(removeResult.begin(), removeResult.end());
                    A_updatedChunks.insert(addResult.begin(), addResult.end());
                }
                else {
                    std::set<glm::vec3, vec3Comparator> noneResult = Server::getInstance().chunks[lightChunk.first]->simulateAmbientSource(lightChunk.first, std::make_pair(lightChunk.first, lightChunk.second.position), false, NONE);
                    A_updatedChunks.insert(noneResult.begin(), noneResult.end());
                }

                lightUpdateQueue.pop_front();
            }
            // std::cout << updatedChunks.size() << " size" << std::endl;
            for (auto c : L_updatedChunks) {
                Server::getInstance().chunks[c]->resetLights();
                Server::getInstance().chunks[c]->checkLights(c);
                updatedChunks.insert(c);
            }
            for (auto c : A_updatedChunks) {
                Server::getInstance().chunks[c]->resetAmbient();
                Server::getInstance().chunks[c]->checkAmbient(c);
                updatedChunks.insert(c);
            }
            for (auto c : updatedChunks) {
                // printf("%f %f %f chunk\n", c.x, c.y, c.z);
                LightMapServer lightpacket;
                lightpacket.chunkpos = c;
                for (auto& s : Server::getInstance().clients) {
                    Server::getInstance().sendPacket(s.first, &lightpacket);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    lightthread.detach();

    std::thread physicsthread([&]() {
        int tps = 60;
        int mps = 1000 / tps;
        long long ltt = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        long long lst = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        while (true) {
            long long now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            long long elapsed = now - ltt;

            if (elapsed >= mps) {
                int ticksToRun = (int) (elapsed / mps);
                ltt += ticksToRun * mps;

                for (int i = 0; i < ticksToRun; i++) {
                    Server::getInstance().serverPhysicsEngine->step();
                }
            }

            if (now >= lst + 5000) {
                std::cout << "Physics sync..." << std::endl;
                for (auto& chunk : Server::getInstance().serverPhysicsEngine->registeredObjects) {
                    for (auto& c : chunk.second) {
                        PlayerAuthInputServer movepacket;
                        movepacket.uuid = c->object->uuid;
                        movepacket.position = c->object->position;
                        movepacket.rotation = c->object->rotation;
                        movepacket.velocity = c->velocity;
                        for (auto& s : Server::getInstance().clients) {
                            Server::getInstance().sendPacket(s.first, &movepacket);
                        }
                    }
                }
                lst = now;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
         }
    });
    physicsthread.detach();

#ifndef BUILD_TYPE_DEDICATED
    std::thread packetthread([&]() {
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
    packetthread.detach();
#endif

#ifdef BUILD_TYPE_DEDICATED
    std::thread regionthread([&]() {
        int ch = 0;
        while (true) {
            std::set<glm::vec3, vec3Comparator> activeRegions;
            for (auto& entity : Server::getInstance().entities) {
                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        for (int z = -1; z <= 1; z++) {
                            activeRegions.insert(glm::vec3(floor(floor(entity.second->position.x / 8.0f) / 8.0f) + (float)x, floor(floor(entity.second->position.y / 8.0f) / 8.0f) + (float)y, floor(floor(entity.second->position.z / 8.0f) / 8.0f) + (float)z));
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
                    RegionRegistory::getInstance().save(glm::vec3(region.x * 8.0f, region.y * 8.0f, region.z * 8.0f));
                    RegionRegistory::getInstance().loadedRegions.erase(region);

                    for (int x = 0; x < 8; x++) {
                        for (int y = 0; y < 8; y++) {
                            for (int z = 0; z < 8; z++) {
                                glm::vec3 regionChunk = glm::vec3((region.x * 8.0f) + (float)x, (region.y * 8.0f) + (float)y, (region.z * 8.0f) + (float)z);
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
    regionthread.detach();
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

    if (session->connectionState != HANDSHAKE_EXCHANGE) {
        auto compressionType  = session->networkSettings.compressionType;

        if (data.size() < session->networkSettings.compressionThreshold || compressionType == CompressionType::DUMMY) {
            data.insert(data.begin(), 0xFF);
        } else {
            switch (compressionType) {
                case CompressionType::ZLIB:
                    data = ZLibUtils::compress_data(data, 5);
                    break;
            }
        }
    }

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