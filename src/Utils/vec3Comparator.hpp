#pragma once

#define GLM_FORCE_PURE
#include "../../lib/glm/glm.hpp"
#include "../../lib/glm/gtc/matrix_transform.hpp"
#include "../../lib/glm/gtc/type_ptr.hpp"

struct vec3Comparator {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    }
};

struct vec2Comparator {
    bool operator()(const glm::vec2& a, const glm::vec2& b) const {
        return std::tie(a.x, a.y) < std::tie(b.x, b.y);
    }
};

struct vec3PairComparator {
    bool operator()(const std::pair<glm::vec3, glm::vec3>& a, const std::pair<glm::vec3, glm::vec3>& b) const {
        return std::tie(a.first.x, a.first.y, a.first.z, a.second.x, a.second.y, a.second.z) < std::tie(b.first.x, b.first.y, b.first.z, b.second.x, b.second.y, b.second.z);
    }
};
