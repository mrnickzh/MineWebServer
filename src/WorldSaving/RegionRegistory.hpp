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
        ss << "/regions/" << (int)currentRegion.x << "-" << (int)currentRegion.y << "-" << (int)currentRegion.z << ".bin";
        std::ofstream file(ss.str(), std::ios::binary);

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();

        loadedRegions.erase(currentRegion);
    }

    bool load(Vec3<float> pos) {
        Vec3<float> currentRegion = Vec3<float>(floor(pos.x / 8.0f), floor(pos.y / 8.0f), floor(pos.z / 8.0f));

        if (loadedRegions.contains(currentRegion)) { return true; }
        loadedRegions.insert(currentRegion);

        std::ostringstream ss;
        ss << "/regions/" << (int)currentRegion.x << "-" << (int)currentRegion.y << "-" << (int)currentRegion.z << ".bin";
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

        float rx = bb.readFloat();
        float ry = bb.readFloat();
        float rz = bb.readFloat();

        int format = bb.readInt();
        RegionFormat* rf = findFormat(format);
        rf->load(bb, currentRegion);

        if(format != FORMAT_VERSION) { // format migration
            save(currentRegion);
        }
        return true;
    }

    void exportAll() {
        std::set<Vec3<float>> savedRegions;
        for (auto& chunk : Server::getInstance().chunks) {
            Vec3<float> currentRegion = Vec3<float>(floor(chunk.first.x / 8.0f), floor(chunk.first.y / 8.0f), floor(chunk.first.z / 8.0f));
            if (savedRegions.contains(currentRegion)) { continue; }
            // std::cout << currentRegion.x << " " << currentRegion.y << " " << currentRegion.z << std::endl;
            save(Vec3<float>((currentRegion.x * 8.0f), (currentRegion.y * 8.0f), (currentRegion.z * 8.0f)));
            savedRegions.insert(currentRegion);
        }

        // for (std::filesystem::directory_entry entry : std::filesystem::directory_iterator("/regions")) {
        //     std::cout << entry.path().string() << std::endl;
        //     std::cout << entry.file_size() << std::endl;
        // }

        std::vector<uint8_t> worldbuffer;
        for (auto& currentRegion : savedRegions) {
            std::ostringstream ss;
            ss << "/regions/" << (int)currentRegion.x << "-" << (int)currentRegion.y << "-" << (int)currentRegion.z << ".bin";
            std::string path = ss.str();
            // std::cout << ss.str() << std::endl;

            std::ifstream file(path, std::ifstream::ate | std::ios::binary);
            std::streamsize size = file.tellg();
            // std::cout << "size: " << size << std::endl;
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

        std::ofstream file("/world.mww", std::ios::binary);
        // std::cout << worldbuffer.size() << std::endl;
        // std::cout << outbuffer.size() << std::endl;
        file.write(reinterpret_cast<const char*>(outbuffer.data()), outbuffer.size());
        file.close();
    }

    void importAll() {
        std::ostringstream ss;
        ss << "/world.mmw";
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

            int rx = bb.readFloat();
            int ry = bb.readFloat();
            int rz = bb.readFloat();
            // std::cout << rx << " " << ry << " " << rz << std::endl;

            rf->load(bb, Vec3<float>(rx, ry, rz));

            if(format != FORMAT_VERSION) { // format migration
                save(Vec3<float>(rx, ry, rz));
            }
        }
    }
};
