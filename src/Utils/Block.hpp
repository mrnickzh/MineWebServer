#pragma once
#include "Vec.hpp"

class Block {
public:
    int id;
    Vec3<float> position;

    Block(int id, Vec3<float> position) : id(id), position(position) {};
};

