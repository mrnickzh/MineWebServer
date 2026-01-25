#pragma once

#include <vector>
#include <WorldSaving/RegionFormat/RegionFormat.hpp>
#include <WorldSaving/RegionFormat/impl/v1/RegionFormat_V1.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../Utils/ZLibUtils.hpp"

class RegionRegistory {
private:
    std::vector<RegionFormat*> formats;

    const int FORMAT_VERSION = 1;

    RegionFormat* findFormat(int version) {
        for (auto* rf : formats) {
            if (rf->version == version)
                return rf;
        }
        return nullptr;
    }
public:
    std::set<Vec3<float>> loadedRegions;

    static RegionRegistory& getInstance() {
        static RegionRegistory instance;
        return instance;
    }

    explicit RegionRegistory() {
        formats.push_back(new RegionFormat_V1());
    }

    std::vector<std::string> strSplit(std::string s, std::string del = " ")
    {
        int start = 0;
        int end = s.find(del);
        std::vector<std::string> list;
        while (end != -1) {
            list.push_back(s.substr(start, end - start));
            start = end + del.size();
            end = s.find(del, start);
        }
        list.push_back(s.substr(start, end - start));
        return list;
    };

    bool isLoaded(Vec3<float> pos) {
        Vec3<float> currentRegion = Vec3<float>(floor(pos.x / 8.0f), floor(pos.y / 8.0f), floor(pos.z / 8.0f));
        if (loadedRegions.contains(currentRegion)) { return true; }
        return false;
    }

    void save(Vec3<float> pos) {
        Vec3<float> currentRegion = Vec3<float>(floor(pos.x / 8.0f), floor(pos.y / 8.0f), floor(pos.z / 8.0f));

        ByteBuf bb(8388608);
        bb.writeInt(FORMAT_VERSION);
        bb.writeFloat(currentRegion.x);
        bb.writeFloat(currentRegion.y);
        bb.writeFloat(currentRegion.z);
        findFormat(FORMAT_VERSION)->save(bb, currentRegion);

        std::vector<uint8_t> data = ZLibUtils::compress_data(bb.toByteArray());
        std::ostringstream ss;
#ifdef BUILD_TYPE_DEDICATED
    #ifdef _MSC_VER
            ss << "regions\\" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
    #else
            ss << "regions/" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
    #endif
#endif

#ifndef BUILD_TYPE_DEDICATED
        ss << "/regions/" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
#endif
        std::ofstream file(ss.str(), std::ios::binary);

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
    }

    bool load(Vec3<float> pos) {
        Vec3<float> currentRegion = Vec3<float>(floor(pos.x / 8.0f), floor(pos.y / 8.0f), floor(pos.z / 8.0f));

        if (loadedRegions.contains(currentRegion)) { return true; }
        loadedRegions.insert(currentRegion);

        std::ostringstream ss;
#ifdef BUILD_TYPE_DEDICATED
    #ifdef _MSC_VER
            ss << "regions\\" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
    #else
            ss << "regions/" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
    #endif
#endif

#ifndef BUILD_TYPE_DEDICATED
        ss << "/regions/" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
#endif
        std::string path = ss.str();

        if (!std::filesystem::exists(path))
            return false;

        std::ifstream file(path, std::ifstream::ate | std::ios::binary);

        std::streamsize size = file.tellg();
        file.seekg(0);

        std::vector<uint8_t> buffer(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        file.close();

        ByteBuf bb(8388608);
        bb.fromByteArray(ZLibUtils::decompress_data(buffer));

        int format = bb.readInt();
        float rx = bb.readFloat();
        float ry = bb.readFloat();
        float rz = bb.readFloat();

        RegionFormat* rf = findFormat(format);
        rf->load(bb, currentRegion);

        if(format != FORMAT_VERSION) { // format migration
            save(currentRegion);
        }
        return true;
    }

    void exportAll() {
        std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
        std::set<Vec3<float>> savedRegions;
        auto serverChunks = Server::getInstance().chunks;

        for (auto& chunk : serverChunks) {
            Vec3<float> currentRegion = Vec3<float>(floor(chunk.first.x / 8.0f), floor(chunk.first.y / 8.0f), floor(chunk.first.z / 8.0f));
            if (savedRegions.contains(currentRegion)) { continue; }
            // std::cout << currentRegion.x << " " << currentRegion.y << " " << currentRegion.z << std::endl;
            save(Vec3<float>((currentRegion.x * 8.0f), (currentRegion.y * 8.0f), (currentRegion.z * 8.0f)));
            savedRegions.insert(currentRegion);
        }

        std::vector<uint8_t> worldbuffer;
        for (std::filesystem::directory_entry entry : std::filesystem::directory_iterator("regions")) {
            std::vector<std::string> regionCoords = strSplit(entry.path().stem().string(), "_");
            Vec3<float> currentRegion = Vec3<float>(stof(regionCoords[0]), stof(regionCoords[1]), stof(regionCoords[2]));

            std::ostringstream ss;
#ifdef BUILD_TYPE_DEDICATED
    #ifdef _MSC_VER
            ss << "regions\\" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
    #else
            ss << "regions/" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
#endif
#endif

#ifndef BUILD_TYPE_DEDICATED
            ss << "/regions/" << (int)currentRegion.x << "_" << (int)currentRegion.y << "_" << (int)currentRegion.z << ".bin";
#endif
            std::string path = ss.str();
            // std::cout << ss.str() << std::endl;

            std::ifstream file(path, std::ifstream::ate | std::ios::binary);
            std::streamsize size = file.tellg();
            std::cout << "size: " << size << std::endl;
            file.seekg(0);

            std::vector<uint8_t> buffer;
            std::vector<uint8_t> temp(size);
            file.read(reinterpret_cast<char*>(temp.data()), size);
            file.close();
            ByteBuf bb(sizeof(int32_t));
            bb.writeInt(size);
            std::vector<uint8_t> vbb = bb.toByteArray();
            buffer.insert(buffer.end(), vbb.begin(), vbb.end());
            buffer.insert(buffer.end(), temp.begin(), temp.end());
            // std::string strdata;
            // for (uint8_t c : buffer) {
            //     strdata += std::to_string((int)c) + " ";
            // }
            // std::cout << strdata << std::endl;
            worldbuffer.insert(worldbuffer.end(), buffer.begin(), buffer.end());
        }

        // std::string strdata;
        // for (uint8_t c : worldbuffer) {
        //     strdata += std::to_string((int)c) + " ";
        // }
        // std::cout << strdata << std::endl;

        std::vector<uint8_t> outbuffer = ZLibUtils::compress_data(worldbuffer);
#ifdef BUILD_TYPE_DEDICATED
        std::ofstream file("world.mww", std::ios::binary);
#else
        std::ofstream file("/world.mww", std::ios::binary);
#endif
        // std::cout << worldbuffer.size() << std::endl;
        // std::cout << outbuffer.size() << std::endl;
        file.write(reinterpret_cast<const char*>(outbuffer.data()), outbuffer.size());
        file.close();
    }

    void importAll() {
        std::lock_guard<std::mutex> guard(Server::getInstance().chunksMutex);
        std::ostringstream ss;
#ifdef BUILD_TYPE_DEDICATED
        ss << "world.mww";
#else
        ss << "/world.mww";
#endif

        std::string path = ss.str();

        std::ifstream file(path, std::ifstream::ate | std::ios::binary);

        std::streamsize size = file.tellg();
        file.seekg(0);

        std::vector<uint8_t> buffer(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        file.close();

        std::vector<uint8_t> worldbuffer = ZLibUtils::decompress_data(buffer);
        // std::cout << worldbuffer.size() << std::endl;
        long pos = 0;

        while (pos < worldbuffer.size()) {
            int regionsize = 0;
            std::memcpy(&regionsize, worldbuffer.data() + pos, sizeof(int32_t));
            // std::cout << "regionsize: " << regionsize << std::endl;

            std::vector<uint8_t> regionbuffer;
            regionbuffer.insert(regionbuffer.begin(), (worldbuffer.data() + pos + sizeof(int32_t)), (worldbuffer.data() + pos + sizeof(int32_t) + regionsize));
            // std::cout << regionbuffer.size() << " rbuf size" << std::endl;

            // std::string strdata;
            // for (uint8_t c : regionbuffer) {
            //     strdata += std::to_string((int)c) + " ";
            // }
            // std::cout << strdata << std::endl;

            pos += regionsize + sizeof(int32_t);

            ByteBuf bb(8388608);
            bb.fromByteArray(ZLibUtils::decompress_data(regionbuffer));

            int format = bb.readInt();
            // std::cout << format << std::endl;
            RegionFormat* rf = findFormat(format);

            float rx = bb.readFloat();
            float ry = bb.readFloat();
            float rz = bb.readFloat();
            // std::cout << rx << " " << ry << " " << rz << std::endl;

            rf->load(bb, Vec3<float>(rx, ry, rz));
            loadedRegions.insert(Vec3<float>(rx, ry, rz));

#ifdef BUILD_TYPE_DEDICATED
            save(Vec3<float>(rx * 8.0f, ry * 8.0f, rz * 8.0f));
            loadedRegions.erase(Vec3<float>(rx, ry, rz));

            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    for (int z = 0; z < 8; z++) {
                        Vec3<float> regionChunk = Vec3<float>((rx * 8.0f) + (float)x, (ry * 8.0f) + (float)y, (rz * 8.0f) + (float)z);
                        if (Server::getInstance().chunks.find(regionChunk) != Server::getInstance().chunks.end()) {
                            Server::getInstance().chunks.erase(regionChunk);
                        }
                    }
                }
            }
#endif
        }
    }
};
