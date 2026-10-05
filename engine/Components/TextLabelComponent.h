#ifndef TEXTLABELCOMPONENT_H
#define TEXTLABELCOMPONENT_H
#include <SDL2/SDL.h>
#include <string>
#include <utility>
#include "../../lib/glm/glm.hpp"
struct TextLabelComponent
{
    glm::vec2 position;
    std::string text;
    std::string fontId;
    SDL_Color color;
    bool isFixed;
    // When set, position is the centre of the text rather than its top-left.
    bool isCentred;
    TextLabelComponent(glm::vec2 position = glm::vec2(0.0f, 0.0f), std::string text = "", std::string fontId = "",
                       SDL_Color color = SDL_Color{255, 255, 255, 255}, bool isFixed = true, bool isCentred = false)
        : position(position), text(std::move(text)), fontId(std::move(fontId)), color(color), isFixed(isFixed),
          isCentred(isCentred)
    {
    }
};
#endif
