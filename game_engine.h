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

class GameEngine {
public:
    Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> actorList;
    std::vector<std::unordered_set<Actor*>> collision_sets;
    std::unordered_set<Actor*> attack_on_this_frame_set;
    std::unordered_set<Actor*> rendering_attack_actors_set;
    glm::ivec2 mapSize;
    glm::ivec2 viewSize;
    Input input;
    
    GameState states;

    Helper helper;
    std::string game_title = "";
    glm::ivec2 window_size;
    glm::ivec3 clear_color;
    SDL_Event event;
    SDL_Window* win;
    SDL_Renderer* ren;

    float zoom_factor = 1.0f;
    std::string next_scene_name;

    ImageDB* imageDB;
    TextDB* textDB;

    AudioManager audioManager;
    AudioInfo intro_bgm_info;
    AudioInfo scene_bgm_info;
    AudioInfo gamewin_bgm_info;
    AudioInfo gamelose_bgm_info;
    AudioInfo score_sfx;
    AudioInfo damage_sfx;
    AudioInfo step_sfx;

    size_t image_idx = 0;
    size_t text_idx = 0;
    
    std::unique_ptr<std::vector<std::string>> intro_image;
    std::unique_ptr<std::vector<std::string>> intro_text;
    std::vector<ImageRenderConfig> images_to_render;
    std::vector<TextRenderConfig> text_to_render;

    glm::vec2 camera = glm::vec2(0.0f, 0.0f);
    
    int health = 3;
    int score = 0;
    std::string hp_image = "";
    float hp_image_width = 0.0f;
    float hp_image_height = 0.0f;
    glm::vec2 camera_lift = glm::vec2(0.0f, 0.0f);
    int coolDownTriggerFrame = -180;
    bool endingFlag = false;
    GameState endingState = GameState::Ongoing;
    float player_movement_speed = 0.02f;
    float cam_ease_factor = 1.0f;
    bool x_scale_actor_flipping_on_movement = false;

    std::string gamewin_image = "";
    std::string gamelose_image =  "";
    bool has_gameend_stage = false;
    bool gameEndFirstFrame = false;

    glm::vec2 spatial_hash_cell_size;
    std::unordered_map<glm::ivec2, std::vector<Actor*>, Ivec2Hash> spatial_hash;

    glm::vec2 spatial_hash_cell_size_trigger;
    std::unordered_map<glm::ivec2, std::vector<Actor*>, Ivec2HashTrigger> spatial_hash_trigger;

    // Constructor and public methods
    GameEngine();
    
    void initializeGame(bool isInitialLoad = true);
    void update();
    void updateIntroAnimation();
    void updateGameEnd(bool win);
    void updateOngoing();
    void updateActorPositions(glm::vec2 playerSpeed);
    void updateActorRenderDirection(glm::vec2 playerSpeed, Actor* actor_ptr);
    glm::vec2 updateGameState();
    bool collisionDetected(glm::vec2 pos, Actor* actor_ptr);
    std::vector<GameIncident> updateDialoguesCollision(std::vector<GameIncident>& allIncidents,
        std::vector<std::string>* dialogue_queue);
    void updateGameIncidents(std::vector<GameIncident>& incidents);
    void checkGameIncidents(Actor* actor, std::vector<GameIncident>& allIncidents, ContactType contactType);
    void updateDialoguesTrigger(std::vector<GameIncident>& allIncidents, std::vector<std::string>* dialogue_queue);
    void updateHurtAndAttackView();
    void postUpdate();
    void gameLoop();
    void frameRender(bool isInitialRender = false);
    void addActorToSpatialHash(Actor* actor, glm::vec2 worldPos);
    void removeActorFromSpatialHash(Actor* actor, glm::vec2 worldPos);
    void moveActorToNewSpatialHash(Actor* actor, glm::vec2 newWorldPos);
    void initializeSpatialHash();

    void initializeSpatialHashTrigger();
    void addActorToSpatialHashTrigger(Actor* actor, glm::vec2 worldPos);
    void removeActorFromSpatialHashTrigger(Actor* actor, glm::vec2 worldPos);
    void moveActorToNewSpatialHashTrigger(Actor* actor, glm::vec2 newWorldPos);
};

#endif // GAME_ENGINE_H
