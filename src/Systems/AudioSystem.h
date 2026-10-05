#ifndef AUDIOSYSTEM_H
#define AUDIOSYSTEM_H
#include <SDL2/SDL_mixer.h>
#include "../ECS/ECS.h"
#include "../AssetManager.h"
#include "../Components/AudioComponent.h"
class AudioSystem : public System
{
public:
    explicit AudioSystem(const AssetManager &assetManager) : assetManager(assetManager)
    {
        RequireComponent<AudioComponent>();
    }
    void Update()
    {
        for (auto entity : GetEntities())
        {
            auto &audio = entity.GetComponent<AudioComponent>();
            if (audio.hasStarted)
            {
                continue;
            }
            // Marked started even if playback fails (e.g. no audio device), so a
            // missing device doesn't mean retrying every frame.
            audio.hasStarted = true;
            Mix_Chunk *sound = assetManager.GetSound(audio.soundId);
            if (!sound)
            {
                continue;
            }
            audio.channel = Mix_PlayChannel(-1, sound, audio.isLoop ? -1 : 0);
            if (audio.channel >= 0)
            {
                Mix_Volume(audio.channel, audio.volume);
            }
        }
    }
    void OnEntityRemoved(Entity entity) override
    {
        const auto &audio = entity.GetComponent<AudioComponent>();
        // Only halt the channel if it is still playing this entity's sound; a
        // finished one-shot's channel may have been reused by another sound.
        if (audio.channel >= 0 && Mix_Playing(audio.channel) &&
            Mix_GetChunk(audio.channel) == assetManager.GetSound(audio.soundId))
        {
            Mix_HaltChannel(audio.channel);
        }
    }

private:
    const AssetManager &assetManager;
};
#endif
