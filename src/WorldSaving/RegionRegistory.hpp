//
// Created by onlym on 12/9/2025.
//

#pragma once

#include <vector>
#include <WorldSaving/RegionFormat/RegionFormat.hpp>
#include <WorldSaving/RegionFormat/impl/v1/RegionFormat_V1.hpp>
#include <fstream>
#include <filesystem>

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
    static RegionRegistory& getInstance() {
        static RegionRegistory instance;
        return instance;
    }

    explicit RegionRegistory() {
        formats.push_back(new RegionFormat_V1());
    }

    void save(Vec3<float> pos) {
        ByteBuf bb(16384);
        bb.writeInt(FORMAT_VERSION);
        findFormat(FORMAT_VERSION)->save(bb, pos);

        std::vector<uint8_t> data = bb.toByteArray();
        std::ostringstream ss;
        ss << "chunks/" << (int)pos.x << "-" << (int)pos.y << "-" << (int)pos.z << ".bin";
        std::ofstream file(ss.str(), std::ios::binary);

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
    }

    bool load(Vec3<float> pos) {
        std::ostringstream ss;
        ss << "chunks/" << (int)pos.x << "-" << (int)pos.y << "-" << (int)pos.z << ".bin";
        std::string path = ss.str();

        if (!std::filesystem::exists(path))
            return false;

        std::ifstream file(path, std::ios::binary | std::ios::ate);

        std::streamsize size = file.tellg();
        file.seekg(0);

        std::vector<uint8_t> buffer(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        file.close();

        ByteBuf bb(16384);
        bb.fromByteArray(buffer);

        int format = bb.readInt();
        RegionFormat* rf = findFormat(format);
        rf->load(bb, pos);

        if(format != FORMAT_VERSION) { // format migration
            save(pos);
        }
        return true;
    }
};
