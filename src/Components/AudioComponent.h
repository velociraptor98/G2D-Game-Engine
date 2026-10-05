#ifndef AUDIOCOMPONENT_H
#define AUDIOCOMPONENT_H
#include <SDL2/SDL_mixer.h>
#include <string>
#include <utility>
// Plays once when the entity appears (or forever if looping) and stops when the
// entity is killed or loses this component.
struct AudioComponent
{
    std::string soundId;
    bool isLoop;
    int volume;
    bool hasStarted;
    int channel;
    AudioComponent(std::string soundId = "", bool isLoop = false, int volume = MIX_MAX_VOLUME)
        : soundId(std::move(soundId)), isLoop(isLoop), volume(volume), hasStarted(false), channel(-1)
    {
    }
};
#endif
