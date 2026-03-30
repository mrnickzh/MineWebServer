#pragma once
#include "Vec.hpp"

class Block {
public:
    int id;
    Vec3<float> position;
    Vec3<float> rotation;
    Vec2<int> lightLevels[6] = {Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}, Vec2<int>{0, -5}};
    bool cancollide;
    Vec3<float> collider;

    Block(int id, Vec3<float> position, Vec3<float> rotation, bool cancollide, Vec3<float> collider) : id(id), position(position), rotation(rotation), cancollide(cancollide), collider(collider) {};
};

