#pragma once
#include "Vec.hpp"

class Block {
public:
    int id;
    Vec3<float> position;
    Vec2<int> lightLevels[6] = {Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}};

    Block(int id, Vec3<float> position) : id(id), position(position) {};
};

