#ifndef BOXCOLLIDERCOMPONENT_H
#define BOXCOLLIDERCOMPONENT_H
#include "../../lib/glm/glm.hpp"
// Size and offset are in sprite pixels and get multiplied by the transform's scale.
struct BoxColliderComponent
{
    int width;
    int height;
    glm::vec2 offset;
    BoxColliderComponent(int width = 0, int height = 0, glm::vec2 offset = glm::vec2(0.0f, 0.0f))
        : width(width), height(height), offset(offset)
    {
    }
};
#endif
