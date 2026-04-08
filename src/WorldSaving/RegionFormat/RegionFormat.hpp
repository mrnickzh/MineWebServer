#pragma once

#include <Utils/ByteBuf.hpp>

class RegionFormat {
public:
    const int version;

    explicit RegionFormat(int version) : version(version) {}
    virtual ~RegionFormat() = default;

    virtual void load(ByteBuf& buffer, glm::vec3 pos) = 0;
    virtual void save(ByteBuf& buffer, glm::vec3 pos) = 0;
};
