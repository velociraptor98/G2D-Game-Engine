#ifndef KEYBOARDCONTROLLEDCOMPONENT_H
#define KEYBOARDCONTROLLEDCOMPONENT_H
#include "../../lib/glm/glm.hpp"
struct KeyboardControlledComponent
{
    float speed;
    glm::vec2 facing;
    KeyboardControlledComponent(float speed = 0.0f, glm::vec2 facing = glm::vec2(1.0f, 0.0f))
        : speed(speed), facing(facing)
    {
    }
};
#endif
