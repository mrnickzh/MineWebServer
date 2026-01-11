#pragma once
#include <vector>

#include "Vec.hpp"

class HeightMap {
public:
    std::map<Vec2<float>, std::vector<std::pair<float, Vec3<float>>>> heightMaps;

    static HeightMap& getInstance() {
        static HeightMap instance;
        return instance;
    }

    void addMap(Vec2<float> xz, float height, Vec3<float> blockpos) {
        if (heightMaps.find(xz) == heightMaps.end()) {
            heightMaps[xz] = std::vector<std::pair<float, Vec3<float>>>();
        }
        heightMaps[xz].push_back(std::make_pair(height, blockpos));
    }
};
