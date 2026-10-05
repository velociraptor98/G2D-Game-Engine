#ifndef KEYBOARDCONTROLLEDCOMPONENT_H
#define KEYBOARDCONTROLLEDCOMPONENT_H
#include <SDL2/SDL.h>
#include <array>
#include <vector>
#include "../../lib/glm/glm.hpp"

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};

struct KeyboardControlledComponent
{
    static constexpr int NO_SPRITE_ROW = -1;

    float speed;
    // Indexed by Direction. Any of a direction's keys moves that way.
    std::array<std::vector<SDL_Scancode>, 4> keys;
    // Spritesheet row to show while moving in each direction (indexed by
    // Direction), or NO_SPRITE_ROW to leave the sprite alone.
    std::array<int, 4> spriteRows;
    // Last direction moved in, normalised. Emitters can aim along it.
    glm::vec2 facing;

    KeyboardControlledComponent(float speed = 0.0f, glm::vec2 facing = glm::vec2(1.0f, 0.0f))
        : speed(speed),
          keys{{{SDL_SCANCODE_UP, SDL_SCANCODE_W},
                {SDL_SCANCODE_DOWN, SDL_SCANCODE_S},
                {SDL_SCANCODE_LEFT, SDL_SCANCODE_A},
                {SDL_SCANCODE_RIGHT, SDL_SCANCODE_D}}},
          spriteRows{NO_SPRITE_ROW, NO_SPRITE_ROW, NO_SPRITE_ROW, NO_SPRITE_ROW}, facing(facing)
    {
    }
};
#endif
