#include <filesystem>
#include <iostream>
#include "Core/Engine.h"

// Usage: g2d [scene.lua]   (defaults to the examples showcase)
int main(int argc, char *argv[])
{
    std::string scene = argc > 1 ? argv[1] : "examples/showcase.lua";
    // A path that exists from the current folder is used as given; anything
    // else is looked up relative to the folder holding the executable.
    if (std::filesystem::exists(scene))
    {
        scene = std::filesystem::absolute(scene).string();
    }
    Engine engine;
    EngineConfig config;
    config.title = "G2D";
    if (!engine.Init(config))
    {
        return 1;
    }
    const bool ran = engine.Run(scene);
    engine.Shutdown();
    if (!ran)
    {
        std::cerr << "Usage: " << argv[0] << " [scene.lua]" << std::endl;
    }
    return ran ? 0 : 1;
}
