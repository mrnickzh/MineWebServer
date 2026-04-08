#pragma once

#define GLM_FORCE_PURE
#include "../../lib/glm/glm.hpp"
#include "../../lib/glm/gtc/matrix_transform.hpp"
#include "../../lib/glm/gtc/type_ptr.hpp"

class Block {
public:
    int id;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::ivec2 lightLevels[6] = {glm::vec2{0, -5}, glm::vec2{0, -5}, glm::vec2{0, -5}, glm::vec2{0, -5}, glm::vec2{0, -5}, glm::vec2{0, -5}};
    bool cancollide;
    glm::vec3 collider;

    Block(int id, glm::vec3 position, glm::vec3 rotation, bool cancollide, glm::vec3 collider) : id(id), position(position), rotation(rotation), cancollide(cancollide), collider(collider) {};
};

