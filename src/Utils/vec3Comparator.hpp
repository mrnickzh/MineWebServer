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

struct vec3Equals {
    bool operator()(const glm::vec3& a, const glm::vec3& b) const {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

struct vec3PairEquals {
    bool operator()(const std::pair<glm::vec3, glm::vec3>& a, const std::pair<glm::vec3, glm::vec3>& b) const {
        return a.first.x == b.first.x && a.first.y == b.first.y && a.first.z == b.first.z && a.second.x == b.second.x && a.second.y == b.second.y && a.second.z == b.second.z;
    }
};

template <typename T>
struct vec3Hash {
    std::size_t operator()(const glm::vec3& a) const {
        std::size_t res = 17;
        res = res * 31 + std::hash<T>()( a.x );
        res = res * 31 + std::hash<T>()( a.y );
        res = res * 31 + std::hash<T>()( a.z );
        return res;
    }
};

template <typename T>
struct vec3PairHash {
    std::size_t operator()(const std::pair<glm::vec3, glm::vec3>& a) const {
        std::size_t res = 17;
        res = res * 31 + std::hash<T>()( a.first.x );
        res = res * 31 + std::hash<T>()( a.first.y );
        res = res * 31 + std::hash<T>()( a.first.z );
        res = res * 31 + std::hash<T>()( a.second.x );
        res = res * 31 + std::hash<T>()( a.second.y );
        res = res * 31 + std::hash<T>()( a.second.z );
        return res;
    }
};
