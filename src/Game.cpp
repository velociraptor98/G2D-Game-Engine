#include "./Game.h"
#include "./Constants.h"
#include <iostream>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include "./GameRules.h"
#include "./Components/TextLabelComponent.h"
#include "./Systems/MovementSystem.h"
#include "./Systems/RenderSystem.h"
#include "./Systems/AnimationSystem.h"
#include "./Systems/CameraMovementSystem.h"
#include "./Systems/KeyboardControlSystem.h"
#include "./Systems/CollisionSystem.h"
#include "./Systems/RenderColliderSystem.h"
#include "./Systems/ProjectileEmitSystem.h"
#include "./Systems/ProjectileLifecycleSystem.h"
#include "./Systems/DamageSystem.h"
#include "./Systems/RenderTextSystem.h"
#include "./Systems/RenderHealthBarSystem.h"
#include "./Systems/AudioSystem.h"
#include "./Systems/ScriptSystem.h"
#include "./Scripting/LuaBindings.h"
#include "./Events/KeyPressedEvent.h"
Game::Game()
    : isRunning(false), window(nullptr), renderer(nullptr), ticksLastFrame(0),
      registry(std::make_unique<Registry>()), assetManager(std::make_unique<AssetManager>()),
      eventBus(std::make_unique<EventBus>()),
      lua(luaL_newstate(), &lua_close),
      camera{0, 0, static_cast<int>(WINDOW_WIDTH), static_cast<int>(WINDOW_HEIGHT)}, isDebug(false)
{
}
Game::~Game()
{

}
bool Game::IsRunning() const
{
    return (*this).isRunning;
}
void Game::init(int width,int height)
{
    if(SDL_Init(SDL_INIT_EVERYTHING)!=0)
    {
        std::cerr << "Error initializing SDL 2: "<<SDL_GetError()<<std::endl;
        return ;
    }
    if((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        std::cerr<<"Error initializing SDL_image: "<<IMG_GetError()<<std::endl;
        return;
    }
    if(TTF_Init()!=0)
    {
        std::cerr<<"Error initializing SDL_ttf: "<<TTF_GetError()<<std::endl;
        return;
    }
    if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,2048)!=0)
    {
        std::cerr<<"Audio unavailable, continuing without sound: "<<Mix_GetError()<<std::endl;
    }
    window  = SDL_CreateWindow(
        NULL,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_BORDERLESS
    ); 
    if(!window)
    {
        std::cerr<<"error in window creation: "<<SDL_GetError()<<std::endl;
        return;
    }
    renderer = SDL_CreateRenderer(window,-1,0);
    if(!renderer)
    {
        std::cerr<<"Failed to create renderer: "<<SDL_GetError()<<std::endl;
        return;
    }
    registry->AddSystem<MovementSystem>();
    registry->AddSystem<RenderSystem>();
    registry->AddSystem<AnimationSystem>();
    registry->AddSystem<CameraMovementSystem>();
    registry->AddSystem<KeyboardControlSystem>();
    registry->AddSystem<CollisionSystem>();
    registry->AddSystem<RenderColliderSystem>();
    registry->AddSystem<ProjectileEmitSystem>(*registry);
    registry->AddSystem<ProjectileLifecycleSystem>();
    registry->AddSystem<DamageSystem>();
    registry->AddSystem<RenderTextSystem>();
    registry->AddSystem<RenderHealthBarSystem>();
    registry->AddSystem<AudioSystem>(*assetManager);
    luaL_openlibs(lua.get());
    RegisterLuaBindings(lua.get(),*registry);
    registry->AddSystem<ScriptSystem>(lua.get());
    registry->GetSystem<MovementSystem>().SubscribeToEvents(*eventBus);
    registry->GetSystem<ProjectileEmitSystem>().SubscribeToEvents(*eventBus);
    registry->GetSystem<DamageSystem>().SubscribeToEvents(*eventBus);
    eventBus->Subscribe<KeyPressedEvent>([this](KeyPressedEvent &event) {
        if(event.symbol == SDLK_c)
        {
            isDebug = !isDebug;
        }
    });
    if(!LoadLevel(1))
    {
        return;
    }
    ticksLastFrame = SDL_GetTicks();
    isRunning = true;
    return;
}

bool Game::LoadLevel(int levelNumber)
{
    return LevelLoader::LoadLevel(levelNumber,lua.get(),*registry,*assetManager,renderer,level);
}
void Game::ProcessInput()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
            isRunning = false;
            break;
            case SDL_KEYDOWN:
            if(event.key.keysym.sym == SDLK_ESCAPE)
            {
                isRunning =false;
            }
            if(!event.key.repeat)
            {
                eventBus->Emit<KeyPressedEvent>(event.key.keysym.sym);
            }
            break;
            default:
            break;
        }
    }
}
void Game::Update()
{
    //quick way to make sure update doesn't exceed the target.
    int waitTime = (int)FRAME_TARGET - (int)(SDL_GetTicks()-ticksLastFrame);
    if(waitTime > 0 && waitTime <= (int)FRAME_TARGET)
    {
        SDL_Delay(waitTime);
    }
    float deltaTime = (SDL_GetTicks()-ticksLastFrame)/1000.0f;
    //Clamping the delta time 
    deltaTime = (deltaTime>0.05f)?0.05f : deltaTime;
    ticksLastFrame = SDL_GetTicks();
    registry->Update();
    registry->GetSystem<KeyboardControlSystem>().Update(SDL_GetKeyboardState(nullptr));
    registry->GetSystem<ScriptSystem>().Update(deltaTime,SDL_GetTicks());
    registry->GetSystem<MovementSystem>().Update(deltaTime,level.mapWidth,level.mapHeight);
    registry->GetSystem<CollisionSystem>().Update(*eventBus);
    registry->GetSystem<ProjectileEmitSystem>().Update(SDL_GetTicks());
    registry->GetSystem<ProjectileLifecycleSystem>().Update(SDL_GetTicks());
    if(auto status = registry->GetEntityByTag("status-label"))
    {
        status->GetComponent<TextLabelComponent>().text = MissionStatus(*registry);
    }
    registry->GetSystem<AudioSystem>().Update();
    registry->GetSystem<AnimationSystem>().Update(SDL_GetTicks());
    registry->GetSystem<CameraMovementSystem>().Update(camera,level.mapWidth,level.mapHeight);
}
void Game::Render()
{
    SDL_SetRenderDrawColor(renderer,21,21,21,255);
    SDL_RenderClear(renderer);
    registry->GetSystem<RenderSystem>().Render(renderer,*assetManager,camera);
    registry->GetSystem<RenderHealthBarSystem>().Render(renderer,camera);
    registry->GetSystem<RenderTextSystem>().Render(renderer,*assetManager,camera);
    if(isDebug)
    {
        registry->GetSystem<RenderColliderSystem>().Render(renderer,camera);
    }
    SDL_RenderPresent(renderer);
}
void Game::Destroy()
{
    assetManager->ClearAssets();
    Mix_CloseAudio();
    if(renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if(window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

