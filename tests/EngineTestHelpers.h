#ifndef ENGINETESTHELPERS_H
#define ENGINETESTHELPERS_H
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <lua.hpp>
#include <sstream>
#include <string>
#include "Core/Engine.h"

// Captures everything written to std::cerr while alive.
struct CapturedErrors
{
    std::ostringstream text;
    std::streambuf *previous = std::cerr.rdbuf(text.rdbuf());
    ~CapturedErrors() { std::cerr.rdbuf(previous); }
    std::string str() const { return text.str(); }
};

// A real Engine on SDL's dummy video and audio drivers: no window appears.
struct HeadlessEngine
{
    Engine engine;
    bool ready = false;
    HeadlessEngine()
    {
        setenv("SDL_VIDEODRIVER", "dummy", 1);
        setenv("SDL_AUDIODRIVER", "dummy", 1);
        EngineConfig config;
        config.rootPath = std::filesystem::current_path().string();
        config.targetFps = 0;
        ready = engine.Init(config);
    }
    ~HeadlessEngine() { engine.Shutdown(); }
    HeadlessEngine(const HeadlessEngine &) = delete;
    HeadlessEngine &operator=(const HeadlessEngine &) = delete;

    void PressKey(SDL_Keycode key)
    {
        SDL_Event event{};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = key;
        event.key.keysym.scancode = SDL_GetScancodeFromKey(key);
        SDL_PushEvent(&event);
    }
    lua_Integer Global(const char *name)
    {
        lua_getglobal(engine.GetLua(), name);
        const lua_Integer value = lua_tointeger(engine.GetLua(), -1);
        lua_pop(engine.GetLua(), 1);
        return value;
    }
    bool HasTag(const char *tag) { return engine.GetRegistry().GetEntityByTag(tag).has_value(); }
};
#endif
