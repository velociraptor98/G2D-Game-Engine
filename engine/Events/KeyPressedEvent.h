#ifndef KEYPRESSEDEVENT_H
#define KEYPRESSEDEVENT_H
#include <SDL2/SDL.h>
struct KeyPressedEvent
{
    SDL_Keycode symbol;
    explicit KeyPressedEvent(SDL_Keycode symbol) : symbol(symbol) {}
};
#endif
