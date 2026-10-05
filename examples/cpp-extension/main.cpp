// A game using the engine as a library and extending it from C++: a new
// component that scene files can use, a system that drives it, and hooks
// that run the system each frame and draw over the scene.
#include <SDL2/SDL.h>
#include "Core/Engine.h"
#include "Components/TransformComponent.h"

struct SpinComponent
{
    float degreesPerSecond;
    SpinComponent(float degreesPerSecond = 0.0f) : degreesPerSecond(degreesPerSecond) {}
};

class SpinSystem : public System
{
public:
    SpinSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<SpinComponent>();
    }
    void Update(float deltaTime)
    {
        for (auto entity : GetEntities())
        {
            entity.GetComponent<TransformComponent>().rotation +=
                entity.GetComponent<SpinComponent>().degreesPerSecond * deltaTime;
        }
    }
};

int main()
{
    Engine engine;
    EngineConfig config;
    config.title = "G2D - C++ extension example";
    if (!engine.Init(config))
    {
        return 1;
    }

    // Makes `spin = { degrees_per_second = ... }` available in scene files.
    engine.GetSceneLoader().RegisterComponent("spin", [](const TableReader &fields, Entity entity) {
        entity.AddComponent<SpinComponent>(fields.Float("degrees_per_second", 90.0f));
    });
    auto &spinSystem = engine.GetRegistry().AddSystem<SpinSystem>();
    engine.AddUpdateHook([&spinSystem](float deltaTime) { spinSystem.Update(deltaTime); });
    engine.AddRenderHook([](SDL_Renderer *renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 200, 80, 255);
        const SDL_Rect frame = {8, 8, 784, 584};
        SDL_RenderDrawRect(renderer, &frame);
    });

    const bool ran = engine.Run("examples/cpp-extension/scene.lua");
    engine.Shutdown();
    return ran ? 0 : 1;
}
