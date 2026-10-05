#include "./Engine.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include "../Events/KeyPressedEvent.h"
#include "../Scripting/LuaBindings.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/AudioSystem.h"
#include "../Systems/CameraMovementSystem.h"
#include "../Systems/CollisionResponseSystem.h"
#include "../Systems/CollisionSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/KeyboardControlSystem.h"
#include "../Systems/LifetimeSystem.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/ProjectileEmitSystem.h"
#include "../Systems/RenderColliderSystem.h"
#include "../Systems/RenderHealthBarSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/RenderTextSystem.h"
#include "../Systems/ScriptSystem.h"
#include "../Systems/WorldBoundsSystem.h"

Engine::Engine()
    : registry(std::make_unique<Registry>()), assetManager(std::make_unique<AssetManager>()),
      eventBus(std::make_unique<EventBus>())
{
}

Engine::~Engine()
{
    Shutdown();
}

bool Engine::Init(const EngineConfig &engineConfig)
{
    config = engineConfig;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    initialised = true;
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        std::cerr << "Error initializing SDL_image: " << IMG_GetError() << std::endl;
        return false;
    }
    if (TTF_Init() != 0)
    {
        std::cerr << "Error initializing SDL_ttf: " << TTF_GetError() << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0)
    {
        std::cerr << "Audio unavailable, continuing without sound: " << Mix_GetError() << std::endl;
    }
    window = SDL_CreateWindow(config.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              config.windowWidth, config.windowHeight, 0);
    if (!window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    rootPath = config.rootPath;
    if (rootPath.empty())
    {
        if (char *basePath = SDL_GetBasePath())
        {
            rootPath = basePath;
            SDL_free(basePath);
        }
    }
    sceneLoader.SetPathResolver([this](const std::string &path) { return ResolvePath(path); });

    registry->AddSystem<KeyboardControlSystem>();
    registry->AddSystem<ScriptSystem>();
    registry->AddSystem<MovementSystem>();
    registry->AddSystem<WorldBoundsSystem>();
    registry->AddSystem<CollisionSystem>();
    registry->AddSystem<CollisionResponseSystem>().SubscribeToEvents(*eventBus);
    registry->AddSystem<DamageSystem>().SubscribeToEvents(*eventBus);
    registry->AddSystem<ProjectileEmitSystem>(*registry).SubscribeToEvents(*eventBus);
    registry->AddSystem<LifetimeSystem>();
    registry->AddSystem<AudioSystem>(*assetManager);
    registry->AddSystem<AnimationSystem>();
    registry->AddSystem<CameraMovementSystem>();
    registry->AddSystem<RenderSystem>();
    registry->AddSystem<RenderHealthBarSystem>();
    registry->AddSystem<RenderTextSystem>();
    registry->AddSystem<RenderColliderSystem>();
    return true;
}

std::string Engine::ResolvePath(const std::string &path) const
{
    const std::filesystem::path given(path);
    if (given.is_absolute() || rootPath.empty())
    {
        return path;
    }
    return (std::filesystem::path(rootPath) / given).lexically_normal().string();
}

bool Engine::Start(const std::string &scenePath)
{
    if (!LoadScene(scenePath))
    {
        return false;
    }
    isRunning = true;
    ticksLastFrame = SDL_GetTicks();
    return true;
}

bool Engine::Run(const std::string &scenePath)
{
    if (!Start(scenePath))
    {
        return false;
    }
    while (isRunning)
    {
        RunFrame();
    }
    return true;
}

bool Engine::LoadScene(const std::string &path)
{
    UnloadScene();
    lua = luaL_newstate();
    luaL_openlibs(lua);
    // Lets scenes `require` shared Lua modules by root-relative name.
    lua_getglobal(lua, "package");
    const std::string luaPath = ResolvePath("?.lua") + ";" + ResolvePath("?/init.lua");
    lua_pushstring(lua, luaPath.c_str());
    lua_setfield(lua, -2, "path");
    lua_pop(lua, 1);
    RegisterLuaBindings(lua, *registry);
    RegisterEngineBindings();
    registry->GetSystem<ScriptSystem>().SetLuaState(lua);

    currentScene = path;
    if (!sceneLoader.LoadFile(path, lua, *registry, *assetManager, renderer, sceneInfo))
    {
        UnloadScene();
        return false;
    }
    if (sceneInfo.worldWidth <= 0 || sceneInfo.worldHeight <= 0)
    {
        sceneInfo.worldWidth = config.windowWidth;
        sceneInfo.worldHeight = config.windowHeight;
    }
    ReadSceneCallbacks();
    registry->Update();
    CallOnStart();
    return true;
}

void Engine::UnloadScene()
{
    // Entities go first: removing them releases their Lua references and stops
    // their sounds, which needs the old Lua state and assets still in place.
    registry->KillAllEntities();
    registry->Update();
    if (lua)
    {
        ReleaseRef(onUpdateRef);
        ReleaseRef(onKeyRef);
        registry->GetSystem<ScriptSystem>().SetLuaState(nullptr);
        lua_close(lua);
        lua = nullptr;
    }
    assetManager->ClearAssets();
    sceneInfo = SceneInfo();
    camera = {0, 0, config.windowWidth, config.windowHeight};
    debugOverlay = false;
}

void Engine::ReadSceneCallbacks()
{
    lua_getglobal(lua, "Scene");
    const char *names[] = {"on_update", "on_key"};
    int *refs[] = {&onUpdateRef, &onKeyRef};
    for (int i = 0; i < 2; ++i)
    {
        lua_getfield(lua, -1, names[i]);
        if (lua_isfunction(lua, -1))
        {
            *refs[i] = luaL_ref(lua, LUA_REGISTRYINDEX);
        }
        else
        {
            if (!lua_isnil(lua, -1))
            {
                std::cerr << "Scene warning: Scene." << names[i] << " should be a function" << std::endl;
            }
            lua_pop(lua, 1);
        }
    }
    lua_pop(lua, 1);
}

void Engine::ReleaseRef(int &ref)
{
    if (lua && ref != LUA_NOREF)
    {
        luaL_unref(lua, LUA_REGISTRYINDEX, ref);
    }
    ref = LUA_NOREF;
}

void Engine::CallOnStart()
{
    lua_getglobal(lua, "Scene");
    lua_getfield(lua, -1, "on_start");
    if (lua_isfunction(lua, -1))
    {
        if (lua_pcall(lua, 0, 0, 0) != LUA_OK)
        {
            std::cerr << "Scene.on_start error: " << lua_tostring(lua, -1) << std::endl;
            lua_pop(lua, 1);
        }
    }
    else
    {
        lua_pop(lua, 1);
    }
    lua_pop(lua, 1);
}

void Engine::CallOnUpdate(float deltaTime, Uint32 ticks)
{
    if (onUpdateRef == LUA_NOREF)
    {
        return;
    }
    lua_rawgeti(lua, LUA_REGISTRYINDEX, onUpdateRef);
    lua_pushnumber(lua, deltaTime);
    lua_pushinteger(lua, ticks);
    if (lua_pcall(lua, 2, 0, 0) != LUA_OK)
    {
        // Disabled after the first error so it reports once, not every frame.
        std::cerr << "Scene.on_update error: " << lua_tostring(lua, -1) << std::endl;
        lua_pop(lua, 1);
        ReleaseRef(onUpdateRef);
    }
}

bool Engine::CallOnKey(const char *keyName)
{
    if (onKeyRef == LUA_NOREF)
    {
        return false;
    }
    lua_rawgeti(lua, LUA_REGISTRYINDEX, onKeyRef);
    lua_pushstring(lua, keyName);
    if (lua_pcall(lua, 1, 1, 0) != LUA_OK)
    {
        std::cerr << "Scene.on_key error: " << lua_tostring(lua, -1) << std::endl;
        lua_pop(lua, 1);
        ReleaseRef(onKeyRef);
        return false;
    }
    const bool handled = lua_toboolean(lua, -1);
    lua_pop(lua, 1);
    return handled;
}

void Engine::ProcessInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            Quit();
        }
        else if (event.type == SDL_KEYDOWN && !event.key.repeat)
        {
            const SDL_Keycode key = event.key.keysym.sym;
            eventBus->Emit<KeyPressedEvent>(key);
            if (!CallOnKey(SDL_GetKeyName(key)) && key == SDLK_ESCAPE)
            {
                RequestBack();
            }
        }
    }
}

void Engine::Update(float deltaTime)
{
    const Uint32 ticks = SDL_GetTicks();
    registry->Update();
    registry->GetSystem<KeyboardControlSystem>().Update(SDL_GetKeyboardState(nullptr));
    registry->GetSystem<ScriptSystem>().Update(deltaTime, ticks);
    CallOnUpdate(deltaTime, ticks);
    registry->GetSystem<MovementSystem>().Update(deltaTime);
    registry->GetSystem<WorldBoundsSystem>().Update(sceneInfo.worldWidth, sceneInfo.worldHeight);
    registry->GetSystem<CollisionSystem>().Update(*eventBus);
    registry->GetSystem<ProjectileEmitSystem>().Update(ticks);
    registry->GetSystem<LifetimeSystem>().Update(ticks);
    registry->GetSystem<AudioSystem>().Update();
    registry->GetSystem<AnimationSystem>().Update(ticks);
    registry->GetSystem<CameraMovementSystem>().Update(camera, sceneInfo.worldWidth, sceneInfo.worldHeight);
    for (auto &hook : updateHooks)
    {
        hook(deltaTime);
    }
}

void Engine::Render()
{
    const SDL_Color &background = sceneInfo.backgroundColor;
    SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, 255);
    SDL_RenderClear(renderer);
    registry->GetSystem<RenderSystem>().Render(renderer, *assetManager, camera);
    registry->GetSystem<RenderHealthBarSystem>().Render(renderer, camera);
    registry->GetSystem<RenderTextSystem>().Render(renderer, *assetManager, camera);
    for (auto &hook : renderHooks)
    {
        hook(renderer);
    }
    if (debugOverlay)
    {
        registry->GetSystem<RenderColliderSystem>().Render(renderer, camera);
    }
    SDL_RenderPresent(renderer);
}

void Engine::RunFrame()
{
    const Uint32 frameTarget = config.targetFps > 0 ? 1000 / config.targetFps : 0;
    const int waitTime = static_cast<int>(frameTarget) - static_cast<int>(SDL_GetTicks() - ticksLastFrame);
    if (waitTime > 0 && waitTime <= static_cast<int>(frameTarget))
    {
        SDL_Delay(waitTime);
    }
    // Clamped so a stall (window drag, breakpoint) doesn't teleport everything.
    const float deltaTime = std::min((SDL_GetTicks() - ticksLastFrame) / 1000.0f, 0.05f);
    ticksLastFrame = SDL_GetTicks();

    ProcessInput();
    Update(deltaTime);
    Render();
    ApplyPendingChange();
}

void Engine::RequestScene(const std::string &path)
{
    pendingChange = PendingChange::Load;
    pendingScene = path;
}

void Engine::RequestReload()
{
    pendingChange = PendingChange::Reload;
}

void Engine::RequestBack()
{
    pendingChange = PendingChange::Back;
}

void Engine::ApplyPendingChange()
{
    const PendingChange change = pendingChange;
    pendingChange = PendingChange::None;
    if (change == PendingChange::None || !isRunning)
    {
        return;
    }
    const std::string previous = currentScene;
    bool loaded = false;
    switch (change)
    {
    case PendingChange::Load:
        loaded = LoadScene(pendingScene);
        if (loaded)
        {
            history.push_back(previous);
        }
        break;
    case PendingChange::Reload:
        loaded = LoadScene(previous);
        break;
    case PendingChange::Back:
        if (history.empty())
        {
            Quit();
            return;
        }
        loaded = LoadScene(history.back());
        history.pop_back();
        break;
    case PendingChange::None:
        return;
    }
    // A scene that fails to load falls back to restarting the one that was
    // running, so one broken scene doesn't end the session.
    if (!loaded && (change == PendingChange::Reload || !LoadScene(previous)))
    {
        std::cerr << "Could not load a scene to continue with; quitting" << std::endl;
        Quit();
    }
    ticksLastFrame = SDL_GetTicks();
}

void Engine::AddUpdateHook(std::function<void(float)> hook)
{
    updateHooks.push_back(std::move(hook));
}

void Engine::AddRenderHook(std::function<void(SDL_Renderer *)> hook)
{
    renderHooks.push_back(std::move(hook));
}

void Engine::Shutdown()
{
    if (!initialised)
    {
        return;
    }
    UnloadScene();
    Mix_CloseAudio();
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    initialised = false;
    isRunning = false;
}

namespace
{
    Engine &EngineOf(lua_State *lua)
    {
        return *static_cast<Engine *>(lua_touserdata(lua, lua_upvalueindex(1)));
    }

    int LoadSceneBinding(lua_State *lua)
    {
        EngineOf(lua).RequestScene(luaL_checkstring(lua, 1));
        return 0;
    }

    int ReloadSceneBinding(lua_State *lua)
    {
        EngineOf(lua).RequestReload();
        return 0;
    }

    int BackBinding(lua_State *lua)
    {
        EngineOf(lua).RequestBack();
        return 0;
    }

    int QuitBinding(lua_State *lua)
    {
        EngineOf(lua).Quit();
        return 0;
    }

    int SetDebugOverlayBinding(lua_State *lua)
    {
        EngineOf(lua).SetDebugOverlay(lua_toboolean(lua, 1));
        return 0;
    }

    int ToggleDebugOverlayBinding(lua_State *lua)
    {
        Engine &engine = EngineOf(lua);
        engine.SetDebugOverlay(!engine.IsDebugOverlay());
        return 0;
    }

    int GetWorldSizeBinding(lua_State *lua)
    {
        const SceneInfo &info = EngineOf(lua).GetSceneInfo();
        lua_pushinteger(lua, info.worldWidth);
        lua_pushinteger(lua, info.worldHeight);
        return 2;
    }

    int SpawnBinding(lua_State *lua)
    {
        luaL_checktype(lua, 1, LUA_TTABLE);
        Engine &engine = EngineOf(lua);
        const Entity entity = engine.GetSceneLoader().CreateEntity(lua, 1, "spawn", engine.GetRegistry());
        lua_pushinteger(lua, EncodeEntity(entity));
        return 1;
    }
}

void Engine::RegisterEngineBindings()
{
    const luaL_Reg functions[] = {
        {"load_scene", LoadSceneBinding},
        {"reload_scene", ReloadSceneBinding},
        {"back", BackBinding},
        {"quit", QuitBinding},
        {"set_debug_overlay", SetDebugOverlayBinding},
        {"toggle_debug_overlay", ToggleDebugOverlayBinding},
        {"get_world_size", GetWorldSizeBinding},
        {"spawn", SpawnBinding},
    };
    for (const auto &function : functions)
    {
        lua_pushlightuserdata(lua, this);
        lua_pushcclosure(lua, function.func, 1);
        lua_setglobal(lua, function.name);
    }
}
