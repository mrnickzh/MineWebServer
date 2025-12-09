//
// Created by onlym on 12/9/2025.
//

#pragma once

#include <WorldSaving/RegionFormat/RegionFormat.hpp>

class RegionFormat_V1 : public RegionFormat {
public:
    RegionFormat_V1() : RegionFormat(1){}

    void load(ByteBuf &buffer, Vec3<float> pos) override;
    void save(ByteBuf &buffer, Vec3<float> pos) override;
};
