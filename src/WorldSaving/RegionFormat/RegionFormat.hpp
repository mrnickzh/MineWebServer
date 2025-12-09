//
// Created by onlym on 12/9/2025.
//

#pragma once

#include <Utils/ByteBuf.hpp>
#include <Utils/Vec.hpp>

class RegionFormat {
public:
    const int version;

    explicit RegionFormat(int version) : version(version) {}
    virtual ~RegionFormat() = default;

    virtual void load(ByteBuf& buffer, Vec3<float> pos) = 0;
    virtual void save(ByteBuf& buffer, Vec3<float> pos) = 0;
};
