#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H
#include "../../lib/glm/glm.hpp"
struct TransformComponent
{
    glm::vec2 position;
    glm::vec2 scale;
    double rotation;
    TransformComponent(glm::vec2 position = glm::vec2(0.0f, 0.0f), glm::vec2 scale = glm::vec2(1.0f, 1.0f), double rotation = 0.0)
        : position(position), scale(scale), rotation(rotation)
    {
    }
};
#endif
