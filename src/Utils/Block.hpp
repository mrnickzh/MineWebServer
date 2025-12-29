#pragma once
#include "Vec.hpp"

class Block {
public:
    int id;
    Vec3<float> position;
    int lightLevels[6] = {0, 0, 0, 0, 0, 0};

    Block(int id, Vec3<float> position) : id(id), position(position) {};
};

