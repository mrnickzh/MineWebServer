#pragma once

#include <WorldSaving/RegionFormat/RegionFormat.hpp>

class RegionFormat_V1 : public RegionFormat {
public:
    RegionFormat_V1() : RegionFormat(1){}

    void load(ByteBuf &buffer, glm::vec3 pos) override;
    void save(ByteBuf &buffer, glm::vec3 pos) override;
};
