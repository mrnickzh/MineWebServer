#pragma once

#include "Vec.hpp"

struct ServerAABB {
    Vec3<float> AA;
    Vec3<float> BB;
};

class GetAABB {
public:
    static ServerAABB CP2AABB(Vec3<float> collider, Vec3<float> position) {
        Vec3<float> aa = position - collider;
        Vec3<float> bb = position + collider;
        return ServerAABB(aa, bb);
    }
};