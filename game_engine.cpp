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

        //SDL
        window_size = parser.getResolution();
        clear_color = parser.getClearColor();
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();
        win = helper.SDL_CreateWindow("", 100, 100, window_size.x, window_size.y, SDL_WINDOW_SHOWN);
        ren = Helper::SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
        // LUA initialize
        // 1. 初始化全局 Lua（整个引擎共用一个 lua_State）
        L = luaL_newstate();
        luaL_openlibs(L);
        // 3. initialize managers:
        world = PhysicsManager::Instance().GetOrCreateWorld();
        componentDB = std::make_shared<ComponentDB>(L, world);
        actorManager = std::make_shared<ActorManager>(L); // 创建 Actor 管理器实例，并传入 Lua 状态
        textManager = std::make_shared<TextManager>(ren); // 创建 Text 管理器实例，传入 SDL_Renderer 用于文本渲染
        input.Init(); // Initialize input states
        audioManager = std::make_shared<AudioManager>();
        audioManager->Init(); // Initialize audio subsystem
        imageManager = std::make_shared<ImageManager>(ren); // 创建 Image 管理器实例，传入 SDL_Renderer 用于图像加载和渲染
        cameraManager = std::make_shared<CameraManager>();
        //PhysicsManager* physics = &PhysicsManager::Instance();

        // 注册 Lua API（Debug / Application / Actor）
        Debug().RegisterLuaAPI(L);
        ApplicationAPI().RegisterLuaAPI(L);
        ActorAPI(actorManager, componentDB).RegisterLuaAPI(L);
        InputAPI().RegisterLuaAPI(L); // 统一格式
        TextAPI(textManager).RegisterLuaAPI(L);
        AudioAPI().RegisterLuaAPI(L); // 统一格式
        ImageAPI(imageManager).RegisterLuaAPI(L);
        CameraAPI(cameraManager).RegisterLuaAPI(L);
        SceneAPI(actorManager, &next_scene_name, &current_scene_name).RegisterLuaAPI(L);
        Vector2API().RegisterLuaAPI(L);
        RigidbodyAPI().RegisterLuaAPI(L);
        PhysicsAPI().RegisterLuaAPI(L);
        CollisionAPI().RegisterLuaAPI(L);
        RayCastingAPI().RegisterLuaAPI(L);
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
    // An Update function that actually only handles the ACTOR MANAGER's update

    // 通用的 Lua 组件驱动：先 OnStart，再 OnUpdate，再 OnLateUpdate
    if (!actorManager) return;

    // 1) 先处理所有待启动的组件 OnStart（每个组件只会执行一次）
    actorManager->ProcessOnStartAllActor();

    // error: don't sort by id
    // actor's id should be unique and increasing, but if have bugs check here first
    actorManager->ProcessOnUpdateAllActor();

    // 4) 再对每个组件调用 OnLateUpdate（如果存在）
    actorManager->ProcessOnLateUpdateAllActor();

    // 5) 在一帧所有生命周期函数执行完之后，处理延迟销毁
    actorManager->UpdateDestruction();
}

void GameEngine::gameLoop() {
    // Game Loop:   (if scene change is triggered, change scene)
                    // (poll input events and update input states)
                    // (call Lua lifecycle functions: OnStart / OnUpdate / OnLateUpdate)
                    // (clean input state)
                    // (render a frame)
    while (!input.GetQuit()) {
        // Scene.Load is deferred: apply the newest request at next-frame start.
        processPendingSceneLoad();

        // 事件处理（只依赖 Helper::SDL_PollEvent 和 Input 管理键盘 / 退出）
        while (Helper::SDL_PollEvent(&event)) {
            input.ProcessEvent(event);
        }
        //（OnStart / OnUpdate / OnLateUpdate）
        update();

        // clean up the input manager after update() using the states.
        input.LateUpdate();

        // advance physics engine by one step
        PhysicsManager::Instance().Step();

        // render the frame base on the render requirements in text and image render stacks.
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
