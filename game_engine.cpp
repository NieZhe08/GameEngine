// #include "MapHelper.h"
#include "glm/glm.hpp"
#include "json_paraser.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "game_utils.h"
#include "scene_db.h"
#include <queue>
#include "Helper.h"
#include "SDL2/SDL.h"
#include "SDL_ttf/SDL_ttf.h"
#include "image_db.h"
#include "text_db.h"
#include "AudioHelper.h"
#include <deque>
#include <queue>
#include "input_manager.h"
#include <unordered_set>
#include <functional>
#include "AudioManager.h"
#include "game_engine.h"
#include "component_db.h"
#include "luaAPI.h"

#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

#include "box2d/box2d.h"

GameEngine::GameEngine() {
    // //next_scene_name = "";
    initializeGame(true);
}

GameEngine::~GameEngine() {
    // Destroy managers that may hold LuaRef first.
    actorManager.reset();
    componentDB.reset();
    textManager.reset();
    audioManager.reset();
    imageManager.reset();
    cameraManager.reset();

    // Clear static LuaRef holders before closing Lua state.
    EventBus::ClearAll();

    // NOTE: lua_close currently aborts with munmap_chunk(): invalid pointer
    // in this codebase during close_state. Keep process-exit reclamation.
    if (L) lua_close(L);
    L = nullptr;

    // SDL teardown should be centralized at engine level.
    if (ren) {
        SDL_DestroyRenderer(ren);
        ren = nullptr;
    }
    if (win) {
        SDL_DestroyWindow(win);
        win = nullptr;
    }
    TTF_Quit();
    SDL_Quit();
}

void GameEngine::initializeGame(bool isInitialLoad) {
    JsonParser parser;
    if (isInitialLoad){
        next_scene_name = parser.getInitialScene();

        window_size = parser.getResolution();
        clear_color = parser.getClearColor();
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();
        win = helper.SDL_CreateWindow("", 100, 100, window_size.x, window_size.y, SDL_WINDOW_SHOWN);
        ren = Helper::SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
        L = luaL_newstate();
        luaL_openlibs(L);
        world = PhysicsManager::Instance().GetOrCreateWorld();
        actorManager = std::make_shared<ActorManager>(L);
        textManager = std::make_shared<TextManager>(ren);
        input.Init();
        audioManager = std::make_shared<AudioManager>();
        audioManager->Init();
        imageManager = std::make_shared<ImageManager>(ren);
        cameraManager = std::make_shared<CameraManager>();
        componentDB = std::make_shared<ComponentDB>(
            L,
            world,
            imageManager.get(),
            window_size.x,
            window_size.y
        );

        Debug().RegisterLuaAPI(L);
        ApplicationAPI().RegisterLuaAPI(L);
        ActorAPI(actorManager, componentDB).RegisterLuaAPI(L);
        InputAPI().RegisterLuaAPI(L);
        TextAPI(textManager).RegisterLuaAPI(L);
        AudioAPI().RegisterLuaAPI(L);
        ImageAPI(imageManager).RegisterLuaAPI(L);
        CameraAPI(cameraManager, imageManager).RegisterLuaAPI(L);
        SceneAPI(actorManager, &next_scene_name, &current_scene_name).RegisterLuaAPI(L);
        Vector2API().RegisterLuaAPI(L);
        RigidbodyAPI().RegisterLuaAPI(L);
        ParticleSystemAPI(imageManager.get()).RegisterLuaAPI(L);
        PhysicsAPI().RegisterLuaAPI(L);
        CollisionAPI().RegisterLuaAPI(L);
        RayCastingAPI().RegisterLuaAPI(L);
        EventLuaAPI().RegisterLuaAPI(L);
    } 

    // load scene module
    current_scene_name = next_scene_name;
    SceneDB sceneDB(current_scene_name, L, componentDB, actorManager);
    
    next_scene_name = "";
    // Initialize game state, load map, actors, etc.
    //frameRender(isInitialLoad);
}

void GameEngine::processPendingSceneLoad() {
    if (next_scene_name.empty()) return;
    if (!actorManager || !componentDB) return;

    // Keep actors marked DontDestroy and unload the rest.
    actorManager->ClearSceneActors();

    current_scene_name = next_scene_name;
    SceneDB sceneDB(current_scene_name, L, componentDB, actorManager);
    next_scene_name.clear();
}

void GameEngine::update(){
    if (!actorManager) return;

    actorManager->ProcessOnStartAllActor();

    actorManager->ProcessOnUpdateAllActor();

    actorManager->ProcessOnLateUpdateAllActor();

    actorManager->UpdateDestruction();
}

void GameEngine::gameLoop() {
    // Game Loop:   (if scene change is triggered, change scene)
                    // (poll input events and update input states)
                    // (call Lua lifecycle functions: OnStart / OnUpdate / OnLateUpdate)
                    // (clean input state)
                    // (render a frame)
    while (!input.GetQuit()) {
        processPendingSceneLoad();

        while (Helper::SDL_PollEvent(&event)) {
            input.ProcessEvent(event);
        }
        update();

        input.LateUpdate();

        EventBus::FlushPending();

        PhysicsManager::Instance().Step();

        frameRender(false);
    }

}

void GameEngine::frameRender(bool isInitialRender) {// render main
    (void)isInitialRender;

    SDL_SetRenderDrawColor(
        ren,
        static_cast<Uint8>(std::clamp(clear_color.x, 0, 255)),
        static_cast<Uint8>(std::clamp(clear_color.y, 0, 255)),
        static_cast<Uint8>(std::clamp(clear_color.z, 0, 255)),
        255
    );
    SDL_RenderClear(ren);

    if (this->imageManager && this->cameraManager) {
        this->imageManager->setCameraState(
            glm::vec2(this->cameraManager->getPositionX(), this->cameraManager->getPositionY()),
            this->cameraManager->getZoom()
        );
    }

    this->imageManager->renderSSImages();
    this->imageManager->renderUI();
    this->textManager->renderAllText();
    this->imageManager->renderPixels();
    Helper::SDL_RenderPresent(ren);
    
}
