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
    
    
    GameState states;
    std::string next_scene_name;

    Helper helper;
    
    // SDL stuffs, unsure about whether to remove
    glm::ivec2 window_size;
    glm::ivec3 clear_color;
    SDL_Event event;
    SDL_Window* win;
    SDL_Renderer* ren;

    // possibly useful?
    bool endingFlag = false;
    GameState endingState = GameState::Ongoing;
    bool hasCollision = true;
    bool hasNearbyDialogue = true;

    // 全局 Lua 状态与组件数据库（HW7）
    lua_State* L = nullptr;
    std::unique_ptr<ComponentDB> componentDB;
    // Managers
    ActorManager* actorManager; 
    TextManager* textManager; 
    Input input;
    AudioManager* audioManager;
    ImageManager* imageManager;
    CameraManager* cameraManager;

    //std::unique_ptr<std::vector<std::string>> intro_image;
    //std::unique_ptr<std::vector<std::string>> intro_text;
    //std::vector<ImageRenderConfig> images_to_render;
    //std::vector<TextRenderConfig> text_to_render;

    //glm::vec2 spatial_hash_cell_size;
    //std::unordered_map<glm::ivec2, std::vector<Actor*>, Ivec2Hash> spatial_hash;

    //glm::vec2 spatial_hash_cell_size_trigger;
    //std::unordered_map<glm::ivec2, std::vector<Actor*>, Ivec2HashTrigger> spatial_hash_trigger;

    //glm::vec2 spatial_hash_cell_size_window;
    //std::unordered_map<glm::ivec2, std::vector<Actor*>, Ivec2HashWindow> spatial_hash_window;

    // Constructor and public methods
    GameEngine();
    ~GameEngine();
    
    void initializeGame(bool isInitialLoad = true);
    void update();
    void updateIntroAnimation();
    void updateGameEnd(bool win);
    void updateOngoing();
    void updateActorPositions(const glm::vec2& playerSpeed);
    void updateActorRenderDirection(const glm::vec2& playerSpeed, Actor* actor_ptr);
    glm::vec2 updateGameState();
    bool collisionDetected(const glm::vec2& pos, Actor* actor_ptr);
    void updateDialoguesCollision(std::vector<GameIncident>& allIncidents,
        std::vector<std::string>* dialogue_queue);
    void updateGameIncidents(std::vector<GameIncident>& incidents);
    void checkGameIncidents(Actor* actor, std::vector<GameIncident>& allIncidents, ContactType contactType);
    void updateDialoguesTrigger(std::vector<GameIncident>& allIncidents, std::vector<std::string>* dialogue_queue);
    void updateHurtAndAttackView();
    void postUpdate();
    void gameLoop();
    void frameRender(bool isInitialRender = false);
    void addActorToSpatialHash(Actor* actor, const glm::vec2& worldPos);
    void removeActorFromSpatialHash(Actor* actor, const glm::vec2& worldPos);
    void moveActorToNewSpatialHash(Actor* actor, const glm::vec2& newWorldPos);
    void initializeSpatialHash();

    void initializeSpatialHashTrigger();
    void addActorToSpatialHashTrigger(Actor* actor, const glm::vec2& worldPos);
    void removeActorFromSpatialHashTrigger(Actor* actor, const glm::vec2& worldPos);
    void moveActorToNewSpatialHashTrigger(Actor* actor, const glm::vec2& newWorldPos);

    void initializeSpatialHashWindow();
    void addActorToSpatialHashWindow(Actor* actor, const glm::vec2& worldPos);
    void removeActorFromSpatialHashWindow(Actor* actor, const glm::vec2& worldPos);
    void moveActorToNewSpatialHashWindow(Actor* actor, const glm::vec2& newWorldPos);
};

#endif // GAME_ENGINE_H
