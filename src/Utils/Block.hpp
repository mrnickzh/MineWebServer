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
    int lightlevel = 0;

    Block(int id, glm::vec3 position, glm::vec3 rotation, bool cancollide, glm::vec3 collider) : id(id), position(position), rotation(rotation), cancollide(cancollide), collider(collider) {};
    Block() : id(0), position(glm::vec3(0.0f, 0.0f, 0.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)), cancollide(false), collider(glm::vec3(0.5f, 0.5f, 0.5f)) {};
};

