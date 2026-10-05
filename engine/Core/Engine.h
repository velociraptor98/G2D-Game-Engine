#ifndef ENGINE_H
#define ENGINE_H
#include <SDL2/SDL.h>
#include <functional>
#include <lua.hpp>
#include <memory>
#include <string>
#include <vector>
#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Assets/AssetManager.h"
#include "../Scene/SceneLoader.h"

struct EngineConfig
{
    std::string title = "G2D";
    int windowWidth = 800;
    int windowHeight = 600;
    int targetFps = 60;
    // Relative scene and asset paths resolve against this; empty means the
    // folder holding the executable.
    std::string rootPath;
};

// Owns the window, the main loop and everything a scene runs on. Built-in
// systems are registered by Init(); games add their own components through
// GetSceneLoader().RegisterComponent, their own systems through
// GetRegistry().AddSystem, and run them from update/render hooks.
//
// Each scene gets a fresh Lua state. Loading a scene first unloads the current
// one completely: entities, assets and the old Lua state.
class Engine
{
public:
    Engine();
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    bool Init(const EngineConfig &config = EngineConfig());
    // Loads the scene and runs frames until quit. False if it failed to load.
    bool Run(const std::string &scenePath);
    // Loads the scene and marks the engine running, for hosts that call
    // RunFrame() themselves.
    bool Start(const std::string &scenePath);
    void Shutdown();

    bool LoadScene(const std::string &path);
    // One iteration of the loop: input, update, render, then any scene change
    // that was requested during the frame.
    void RunFrame();
    bool IsRunning() const { return isRunning; }

    // Scene changes requested from inside a frame (e.g. by Lua) happen at its end.
    void RequestScene(const std::string &path);
    void RequestReload();
    // Returns to the scene that loaded this one, or quits if there is none.
    void RequestBack();
    void Quit() { isRunning = false; }
    void SetDebugOverlay(bool enabled) { debugOverlay = enabled; }
    bool IsDebugOverlay() const { return debugOverlay; }

    // Called every frame, after the built-in systems update / render.
    void AddUpdateHook(std::function<void(float deltaTime)> hook);
    void AddRenderHook(std::function<void(SDL_Renderer *renderer)> hook);

    Registry &GetRegistry() { return *registry; }
    AssetManager &GetAssets() { return *assetManager; }
    EventBus &GetEventBus() { return *eventBus; }
    SceneLoader &GetSceneLoader() { return sceneLoader; }
    lua_State *GetLua() const { return lua; }
    SDL_Renderer *GetRenderer() const { return renderer; }
    const SDL_Rect &GetCamera() const { return camera; }
    const SceneInfo &GetSceneInfo() const { return sceneInfo; }
    const std::string &GetCurrentScene() const { return currentScene; }
    std::string ResolvePath(const std::string &path) const;

private:
    enum class PendingChange
    {
        None,
        Load,
        Reload,
        Back
    };

    void UnloadScene();
    void ProcessInput();
    void Update(float deltaTime);
    void Render();
    void ApplyPendingChange();
    void RegisterEngineBindings();
    void ReadSceneCallbacks();
    void CallOnStart();
    void CallOnUpdate(float deltaTime, Uint32 ticks);
    bool CallOnKey(const char *keyName);
    void ReleaseRef(int &ref);

    EngineConfig config;
    bool initialised = false;
    bool isRunning = false;
    bool debugOverlay = false;
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    Uint32 ticksLastFrame = 0;
    std::string rootPath;

    std::unique_ptr<Registry> registry;
    std::unique_ptr<AssetManager> assetManager;
    std::unique_ptr<EventBus> eventBus;
    SceneLoader sceneLoader;
    lua_State *lua = nullptr;
    SDL_Rect camera = {0, 0, 0, 0};
    SceneInfo sceneInfo;

    std::string currentScene;
    std::vector<std::string> history;
    PendingChange pendingChange = PendingChange::None;
    std::string pendingScene;
    int onUpdateRef = LUA_NOREF;
    int onKeyRef = LUA_NOREF;

    std::vector<std::function<void(float)>> updateHooks;
    std::vector<std::function<void(SDL_Renderer *)>> renderHooks;
};
#endif
