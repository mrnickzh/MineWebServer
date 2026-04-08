#pragma once
#include <vector>

#define GLM_FORCE_PURE
#include "../../lib/glm/glm.hpp"
#include "../../lib/glm/gtc/matrix_transform.hpp"
#include "../../lib/glm/gtc/type_ptr.hpp"

#include "vec3Comparator.hpp"

class HeightMap {
public:
    std::map<glm::vec2, std::vector<std::pair<float, glm::vec3>>, vec2Comparator> heightMaps;

    static HeightMap& getInstance() {
        static HeightMap instance;
        return instance;
    }

    void addMap(glm::vec2 xz, float height, glm::vec3 blockpos) {
        if (heightMaps.find(xz) == heightMaps.end()) {
            heightMaps[xz] = std::vector<std::pair<float, glm::vec3>>();
        }
        heightMaps[xz].push_back(std::make_pair(height, blockpos));
    }
};
