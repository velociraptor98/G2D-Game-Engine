#include "./TestFramework.h"
#include "./SDLTestHelpers.h"
#include <SDL2/SDL_mixer.h>
#include "ECS/ECS.h"
#include "Assets/AssetManager.h"
#include "Components/AudioComponent.h"
#include "Systems/AudioSystem.h"

namespace
{
    struct AudioWorld
    {
        DummyAudio audio;
        AssetManager assets;
        Registry registry;
        AudioSystem *system = &registry.AddSystem<AudioSystem>(assets);

        AudioWorld() { assets.AddSound("rotor", "./assets/sounds/helicopter.wav"); }
        ~AudioWorld() { assets.ClearAssets(); }
        Entity Emitter(bool isLoop)
        {
            Entity entity = registry.CreateEntity();
            entity.AddComponent<AudioComponent>("rotor", isLoop, 32);
            registry.Update();
            return entity;
        }
    };
}

TEST(AssetManagerLoadsSoundsAndRejectsMissingOnes)
{
    AudioWorld world;
    REQUIRE(world.audio.isOpen);
    CHECK(world.assets.GetSound("rotor") != nullptr);
    std::cerr << "  (a sound load error is expected below)" << std::endl;
    world.assets.AddSound("missing", "./does/not/exist.wav");
    CHECK(world.assets.GetSound("missing") == nullptr);
}

TEST(AudioSystemStartsASoundOnceAtItsVolume)
{
    AudioWorld world;
    REQUIRE(world.audio.isOpen);
    Entity entity = world.Emitter(true);
    world.system->Update();
    const int channel = entity.GetComponent<AudioComponent>().channel;
    REQUIRE(channel >= 0);
    CHECK(Mix_Playing(channel) != 0);
    CHECK_EQ(Mix_Volume(channel, -1), 32);

    world.system->Update();
    CHECK_EQ(entity.GetComponent<AudioComponent>().channel, channel);
    int playing = 0;
    for (int i = 0; i < Mix_AllocateChannels(-1); ++i)
    {
        playing += Mix_Playing(i) ? 1 : 0;
    }
    CHECK_EQ(playing, 1);
}

TEST(KillingTheEntityStopsItsLoopingSound)
{
    AudioWorld world;
    REQUIRE(world.audio.isOpen);
    Entity entity = world.Emitter(true);
    world.system->Update();
    const int channel = entity.GetComponent<AudioComponent>().channel;
    REQUIRE(channel >= 0);
    entity.Kill();
    world.registry.Update();
    CHECK_EQ(Mix_Playing(channel), 0);
}

TEST(RemovingTheAudioComponentStopsTheSound)
{
    AudioWorld world;
    REQUIRE(world.audio.isOpen);
    Entity entity = world.Emitter(true);
    world.system->Update();
    const int channel = entity.GetComponent<AudioComponent>().channel;
    REQUIRE(channel >= 0);
    entity.RemoveComponent<AudioComponent>();
    world.registry.Update();
    CHECK_EQ(Mix_Playing(channel), 0);
}

TEST(UnknownSoundIsSkippedWithoutRetrying)
{
    AudioWorld world;
    REQUIRE(world.audio.isOpen);
    Entity entity = world.registry.CreateEntity();
    entity.AddComponent<AudioComponent>("nope", true);
    world.registry.Update();
    world.system->Update();
    CHECK(entity.GetComponent<AudioComponent>().hasStarted);
    CHECK_EQ(entity.GetComponent<AudioComponent>().channel, -1);
}
