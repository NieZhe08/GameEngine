#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "glm/glm.hpp"
#include "game_utils.h"
#include "input_manager.h"
#include "AudioManager.h"
#include "image_db.h"
#include "text_db.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <string>
#include "ActorManager.h"
#include "luaAPI.h"
#include "cameraManager.h"

class ComponentDB;

class GameEngine {
public:
    std::string current_scene_name;
    std::string next_scene_name;

    // Autograder API to track the calls of SDL functions 
    Helper helper;
    
    // SDL stuffs, unsure about whether to remove
    glm::ivec2 window_size;
    glm::ivec3 clear_color;
    SDL_Event event;
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;

    // 全局 Lua 状态与组件数据库（HW7）
    lua_State* L = nullptr;
    std::shared_ptr<ComponentDB> componentDB;
    // Managers
    std::shared_ptr<ActorManager> actorManager;
    std::shared_ptr<TextManager> textManager;
    Input input;
    std::shared_ptr<AudioManager> audioManager;
    std::shared_ptr<ImageManager> imageManager;
    std::shared_ptr<CameraManager> cameraManager;
    

    // Physic Engine
    //PhysicsManager* physicsManager = nullptr;
    //bool createPhysicsFlag = false;

    // Constructor and public methods
    GameEngine();
    ~GameEngine();
    
    void initializeGame(bool isInitialLoad = true);
    void processPendingSceneLoad();
    void update();
    void gameLoop();
    void frameRender(bool isInitialRender = false);
};

#endif // GAME_ENGINE_H
