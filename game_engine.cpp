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
#include "audio_db.h"
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

    GameEngine::GameEngine() {
        // //next_scene_name = "";
        initializeGame(true);
    }

    GameEngine::~GameEngine() {
        // Destroy managers that may hold LuaRef first.
        delete actorManager;
        actorManager = nullptr;

        componentDB.reset();

        delete textManager;
        textManager = nullptr;
        delete audioManager;
        audioManager = nullptr;
        delete imageManager;
        imageManager = nullptr;
        delete cameraManager;
        cameraManager = nullptr;

        // Close Lua state after all LuaRef owners are gone.
        if (L) {
            lua_close(L);
            L = nullptr;
        }

        // Clean up SDL resources
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
            componentDB = std::make_unique<ComponentDB>(L);
            actorManager = new ActorManager(L); // 创建 Actor 管理器实例，并传入 Lua 状态
            textManager = new TextManager(ren); // 创建 Text 管理器实例，传入 SDL_Renderer 用于文本渲染
            input.Init(); // Initialize input states
            audioManager = new AudioManager();
            audioManager->Init(); // Initialize audio subsystem
            imageManager = new ImageManager(ren); // 创建 Image 管理器实例，传入 SDL_Renderer 用于图像加载和渲染
            cameraManager = new CameraManager();

            // 注册 Lua API（Debug / Application / Actor）
            Debug().RegisterLuaAPI(L);
            ApplicationAPI().RegisterLuaAPI(L);
            ActorAPI(actorManager, componentDB.get()).RegisterLuaAPI(L);
            InputAPI().RegisterLuaAPI(L); // 统一格式
            TextAPI(textManager).RegisterLuaAPI(L);
            AudioAPI().RegisterLuaAPI(L); // 统一格式
            ImageAPI(imageManager).RegisterLuaAPI(L);
            CameraAPI(cameraManager).RegisterLuaAPI(L);

            states = GameState::Ongoing; 
        } else {
            states = GameState::Ongoing; // Load next scene directly without intro animation
        }
        
        // load scene module
        SceneDB sceneDB(next_scene_name, L, componentDB.get(), *actorManager);
       
        next_scene_name = "";
        endingFlag = false;
        endingState = GameState::Ongoing;
        // Initialize game state, load map, actors, etc.
        //frameRender(isInitialLoad);
    }

    void GameEngine::update(){
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

    void GameEngine::updateIntroAnimation() {
        // //if (states != GameState::IntroAnimation) return;
        // //if (intro_bgm_states == AudioState::Not_Started){
        // //    AudioHelper::Mix_PlayChannel(0, intro_bgm_chunk, -1); // Play intro BGM in a loop
        // //    intro_bgm_states = AudioState::Playing;
        // //} 
        // intro_bgm_info.play(&audioManager);

        // /* below is a FRAME-WISE update for Intro Animation Stage*/
        // images_to_render.clear();
        // text_to_render.clear();
        // // Update Based on user input
        // if (input.GetQuit()) {
        //     //states = GameState::Lost;
        //     endingFlag = true;
        //     endingState = GameState::Quit; // exit the Intro Animation Stage
        // }
        // if (intro_image && !intro_image->empty()) {
        //     if (input.GetKeyDown(SDL_SCANCODE_SPACE) || input.GetKeyDown(SDL_SCANCODE_RETURN)) {
        //         image_idx++;
        //         text_idx++;
        //     }
        // }
        
        // if (intro_image && !intro_image->empty() && (image_idx < intro_image->size() || (intro_text && !intro_text->empty() && text_idx < intro_text->size()))) {
        //     size_t img_idx = (image_idx >= intro_image->size()) ? intro_image->size()-1 : image_idx;
        //     if (img_idx < intro_image->size()) {
        //         images_to_render.emplace_back((*intro_image)[img_idx], SDL_FRect{0, 0, static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
        //     }
        //     if (intro_text && !intro_text->empty()) {
        //         size_t txt_idx = (text_idx >= intro_text->size()) ? intro_text->size()-1 : text_idx;
        //         if (txt_idx < intro_text->size()) {
        //             text_to_render.emplace_back((*intro_text)[txt_idx], 25, window_size.y - 50);
        //         }
        //     }
        // } else {// exit the Intro Animation Stage when all intro images and texts have been shown
        //     states = GameState::Ongoing;
        //     //if (intro_bgm_states == AudioState::Playing){ 
        //     //    AudioHelper::Mix_HaltChannel(0);
        //     //}
        //     intro_bgm_info.halt(&audioManager);
        // }
    }

    void GameEngine::updateGameEnd(bool win) {
        // //if (states != GameState::IntroAnimation) return;
        // //if (scene_bgm_states == AudioState::Playing){ 
        // //        AudioHelper::Mix_HaltChannel(0);
        // //        scene_bgm_states = AudioState::Stopped;
        // //}
        // scene_bgm_info.halt(&audioManager);

        // //AudioState& bgm_state = win ? gamewin_bgm_states : gamelose_bgm_states;
        // //Mix_Chunk* bgm_chunk = win ? gamewin_bgm_chunk : gamelose_bgm_chunk;
        // std::string end_image = win ? gamewin_image : gamelose_image;
        // //if (bgm_state == AudioState::Not_Started){
        // //    AudioHelper::Mix_PlayChannel(0, bgm_chunk, 0); // Play intro BGM in a loop
        // //    bgm_state = AudioState::Playing;
        // //} 
        // if (win){
        //     if (gamewin_bgm_info.audio_state == AudioState::Not_Started)
        //         gamewin_bgm_info.play(&audioManager);
        // } else {
        //     if (gamelose_bgm_info.audio_state == AudioState::Not_Started)
        //         gamelose_bgm_info.play(&audioManager);
        // }

        // /* below is a FRAME-WISE update for Intro Animation Stage*/
        // images_to_render.clear();
        // text_to_render.clear();
        // images_to_render.emplace_back(end_image, SDL_FRect{0, 0, static_cast<float>(window_size.x), static_cast<float>(window_size.y)});

        // if (input.GetQuit()) {
        //     //states = GameState::Lost;
        //     endingFlag = true;
        //     endingState = GameState::Quit; // exit the Intro Animation Stage
        // }
        
    }

    void GameEngine::updateOngoing(){
        // if (states != GameState::Ongoing) return;
        // static const glm::vec2 zero_vec(0.0f, 0.0f);

        // //if (scene_bgm_states == AudioState::Not_Started){
        // //    AudioHelper::Mix_PlayChannel(0, scene_bgm_chunk, -1); // Play gameplay BGM in a loop
        // //    scene_bgm_states = AudioState::Playing;
        // //}

        // images_to_render.clear();
        // text_to_render.clear();

        // glm::vec playerSpeed = updateGameState();
        // mainActor->velocity = playerSpeed; // Update main actor's velocity based on player input
        // updateActorPositions(playerSpeed);

        // scene_bgm_info.play(&audioManager);
        // if (Helper::GetFrameNumber() % 20 == 0){ // Play step sound effect every 20 frames if the main actor is moving
        //     if (mainActor && mainActor->velocity != zero_vec){
        //         step_sfx.play(&audioManager);
        //     }
        // }

        // if (mainActor){
        //     glm::vec offset = glm::vec2((mainActor->transform_position.x) * 100 * zoom_factor, 
        //                     (mainActor->transform_position.y) * 100 * zoom_factor);
        //     glm::vec2 instant_camera = -offset  + glm::vec2(window_size.x /2.0f, window_size.y /2.0f) - camera_lift * zoom_factor; // Update camera to follow the main actor, apply zoom factor to camera lift as well
        //     camera = glm::mix(camera, instant_camera, cam_ease_factor); // Smoothly ease camera towards the target position for a more polished feel
        // }
        // //std::cout<<"Camera Position: ("<<camera.x<<", "<<camera.y<<")\n";
        // std::vector<std::string> dialogue_queue;
        // std::vector<GameIncident> incidents;
        // updateDialoguesCollision(incidents, &dialogue_queue);
        // updateDialoguesTrigger(incidents, &dialogue_queue);
        // updateHurtAndAttackView();
        // // render diaglogues in dialogue_queue 
        // for (size_t i=0; i<dialogue_queue.size(); i++){
        //     text_to_render.emplace_back( dialogue_queue[i], 25, window_size.y - 50 - 50* (dialogue_queue.size()-1 -i));
        // }
        // updateGameIncidents(incidents);
        // if (health <= 0){
        //     //states = GameState::Lost;
        //     //endingFlag = true;
        //     //endingState = GameState::Lost;
        //     states = GameState::Lost;
        //     gameEndFirstFrame = true;
        // }
    }

    void GameEngine::updateActorPositions(const glm::vec2& playerSpeed) {
        // // Update NPC positions based on their velocities
        // if (!actorList) return;
        // static const glm::vec2 zero_vec(0.0f, 0.0f);
        // //bool nonPlayerUpdate = (Helper::GetFrameNumber() &&Helper::GetFrameNumber() % 60 == 0); 
        // for (Actor& actor : *actorList){
        //     glm::vec2 nextPosition;
        //     glm::vec2 vel;
        //     bool hasMoved = false;
        //     if (&actor == mainActor) {
        //         vel = playerSpeed;
        //         nextPosition = actor.transform_position + playerSpeed;
        //         if (playerSpeed != zero_vec){
        //             hasMoved = true;
        //         }
        //     } else {
        //         //if (!nonPlayerUpdate) continue; // Only update non-player actors every 60 frames to slow down their movement
        //         // Now we update non-player actors every frame
        //         if (actor.velocity != zero_vec){
        //             hasMoved = true;
        //         }
        //         vel = actor.velocity;
        //         nextPosition = actor.transform_position + actor.velocity;
        //     }
        //     // collision detection only worry about whether collision happens between actor itself and others
        //     updateActorRenderDirection(vel, &actor);
        //     //if (!hasMoved) continue; // skip the following steps if the actor has not moved
        //     if (!collisionDetected(nextPosition, &actor)){// need to update mapHash
        //         // remove the actor's index from its old cell
        //         moveActorToNewSpatialHash(&actor, nextPosition);
        //         moveActorToNewSpatialHashTrigger(&actor, nextPosition);
        //         moveActorToNewSpatialHashWindow(&actor, nextPosition);
        //         actor.transform_position = nextPosition;
        //         actor.velocity = vel; // Update velocity to the new velocity after collision check, so that the actor will stop immediately when colliding with others instead of going through them for one more frame
        //         //std::cout<<"Actor "<<actor.actor_name<<" moves to ("<<actor.transform_position.x<<", "<<actor.transform_position.y<<")"<<std::endl;
        //     } else {
        //         actor.velocity = -actor.velocity; // Reverse direction on collision
        //         // would be a problem for main actor
        //     }
        //     //actor.transform_position = nextPosition; // Directly update position without collision for test
        // }
    }

    void GameEngine::updateActorRenderDirection (const glm::vec2& playerSpeed, Actor* actor_ptr){
        // if (!actor_ptr) return;
        // if (actor_ptr->has_view_image_back){
        //     if (playerSpeed.y > 0) {
        //         actor_ptr->view_dir_down = true;
        //     } else if (playerSpeed.y < 0) {
        //         actor_ptr->view_dir_down = false;
        //     }
        // }
        // if (x_scale_actor_flipping_on_movement){
        //     if (playerSpeed.x > 0) {
        //         actor_ptr->flip_x = false;
        //     } else if (playerSpeed.x < 0) {
        //         actor_ptr->flip_x = true;
        //     }
        // }
    }

    
    glm::vec2 GameEngine::updateGameState() { // update main
        // static const glm::vec2 zero_vec(0.0f, 0.0f);
        // if (input.GetQuit()) {
        //     //states = GameState::Lost;
        //     endingFlag = true;
        //     endingState = GameState::Quit;
        //     return zero_vec;
        // } else { // Only consider non-repeated keydown events for player actions
        //     //SDL_Keycode key_press = evt.key.keysym.scancode;
        //     //PlayerAction action = PlayerAction::Invalid;
        //     glm::vec2 playerSpeed = zero_vec;
        //     if (input.GetKey(SDL_SCANCODE_UP) || input.GetKey(SDL_SCANCODE_W)) {
        //         playerSpeed += glm::vec2(0.0f, -player_movement_speed);
        //     } 
        //     if (input.GetKey(SDL_SCANCODE_DOWN)|| input.GetKey(SDL_SCANCODE_S)) {
        //         playerSpeed += glm::vec2(0.0f, player_movement_speed);
        //     } 
        //     if (input.GetKey(SDL_SCANCODE_LEFT) || input.GetKey(SDL_SCANCODE_A)) {
        //         playerSpeed += glm::vec2(-player_movement_speed, 0.0f);
        //     } 
        //     if (input.GetKey(SDL_SCANCODE_RIGHT) || input.GetKey(SDL_SCANCODE_D)) {
        //         playerSpeed += glm::vec2(player_movement_speed, 0.0f);
        //     }
        //     if (playerSpeed != zero_vec){
        //         playerSpeed = glm::normalize(playerSpeed) * player_movement_speed; // Normalize diagonal movement to maintain consistent speed
        //     }
        //     return playerSpeed;
        // }
        return glm::vec2(0.0f, 0.0f);
    }

    bool GameEngine::collisionDetected(const glm::vec2& pos, Actor* actor_ptr) {
        // if (!hasCollision) return false;
        // if (actor_ptr->has_box_collider == false) return false; // If the actor itself doesn't have a box collider, skip collision detection
        // bool have_collision = false;
        // if (collision_sets[actor_ptr->id].empty() == false) have_collision = true; // If this actor has already been detected to collide with others in this frame, skip further collision detection to optimize performance
        
        //     //if (&other_actor == actor_ptr) continue; // Skip self
        //     //if (other_actor.has_box_collider == false) continue; // If the other actor doesn't have a box collider, skip collision detection
        //     //if (checkAABB(pos, actor_ptr->box_collider, 
        //     //    other_actor.transform_position, other_actor.box_collider)
        //     //    ){ // Visualize collision boxes for debugging
        //     //    // Add the collided actor to the collision set of the current actor
        //     //    collision_sets[actor_ptr->id].insert(&other_actor);
        //     //    collision_sets[other_actor.id].insert(actor_ptr);
        //     //    have_collision = true;
        //     //}
        // glm::ivec2 center_cell = worldToCell(pos, spatial_hash_cell_size);
        // //if (Helper::GetFrameNumber() % 60 == 0 || true){
        // //    std::cout<<"Checking collision for Actor "<<actor_ptr->actor_name<<" at cell ("<<center_cell.x<<", "<<center_cell.y<<") in spatial hash cell ("<<center_cell.x<<", "<<center_cell.y<<") with "<<collision_sets[actor_ptr->id].size()<<" collisions already detected this frame.\n";
        // //}
        // for (int i = -1; i <= 1; i++) {
        //     for (int j = -1; j <= 1; j++) {
        //         glm::ivec2 check_cell = center_cell + glm::ivec2(i, j);
        //         auto it = spatial_hash.find(check_cell);
        //         if (it != spatial_hash.end()) {
        //             for (Actor* other_actor_ptr : it->second) {
        //                 //std::cout<<"spatial hash cell contents"<<it->second.size()<<"\n";
        //                 if (!other_actor_ptr) continue;
        //                 if (other_actor_ptr == actor_ptr) continue; // Skip self
        //                 if (other_actor_ptr->has_box_collider == false) continue; // If the other actor doesn't have a box collider, skip collision detection
        //                 if (checkAABB(pos, actor_ptr->box_collider, 
        //                     other_actor_ptr->transform_position, other_actor_ptr->box_collider)
        //                     ){ // Visualize collision boxes for debugging
        //                     // Add the collided actor to the collision set of the current actor
        //                         collision_sets[actor_ptr->id].insert(other_actor_ptr);
        //                         collision_sets[other_actor_ptr->id].insert(actor_ptr);
        //                         have_collision = true;
        //                     }
        //             }
        //         }
        //     }
        // }
        // //std::cout<<"has collision: "<<have_collision<<"\n";
        return false;
    }

    
    void GameEngine::updateDialoguesCollision(std::vector<GameIncident>& allIncidents,
        std::vector<std::string>* dialogue_queue) {
        // if (!mainActor) return;
        // // Update dialogues based on proximity to other actors
        // // Use min-heap (std::greater) so that smaller actor IDs are processed first
        // //std::priority_queue<int, std::vector<int>, std::greater<int>> actorsToBeDealted;
        // //if (!actorList || !mainActor) return allIncidents;
        // //for (int i = -10; i<20; i++){
        // //    for (int j = -10; j<20; j++){
        // //        glm::ivec2 check_position = mainActor->transform_position + glm::ivec2(i, j);
        // //        auto it = mapHash.find(hashPosition(check_position));
        // //        if (it != mapHash.end() && !it->second.empty()){
        // //            for (int actor_idx : it->second){
        // //                actorsToBeDealted.push(actor_idx);
        // //            }
        // //        }
        // //    }
        // //}
        // //while (!actorsToBeDealted.empty()){
        // //    int actor_idx = actorsToBeDealted.top();
        // //    actorsToBeDealted.pop();
        // //    Actor& actor = (*actorList)[actor_idx];
        // //        if (actor.transform_position.x == mainActor->transform_position.x 
        // //            && actor.transform_position.y == mainActor->transform_position.y){
        // //            if (&actor == mainActor) continue;
        // //            if (actor.contact_dialogue.empty()) continue;
        // //            checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
        // //            //dialogue_ss<<actor.contact_dialogue<<"\n";
        // //            text_to_render.emplace_back( actor.contact_dialogue, 25, window_size.y / 2.0f - 50 - 50* (text_to_render.size()-1 ));
        // //        } else {
        // //            if (actor.nearby_dialogue.empty()) continue;
        // //            checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
        // //            //dialogue_ss<<actor.nearby_dialogue<<"\n";
        // //            text_to_render.emplace_back( actor.contact_dialogue, 25, window_size.y / 2.0f - 50 - 50* (text_to_render.size()-1 ));
        // //        }
        // //}
        
        // //std::vector<std::string> dialogue_queue; // To store dialogues in the order they are processed
        // //for (Actor& actor : *actorList){
        // //    if (actor.transform_position.x == mainActor->transform_position.x && actor.transform_position.y == mainActor->transform_position.y){
        // //        if (&actor == mainActor) continue;
        // //        if (actor.contact_dialogue.empty()) continue;
        // //        checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
        // //        //dialogue_ss<<actor.contact_dialogue<<"\n";
        // //        if (actor.contact_dialogue != "" && actor.contact_incident != GameIncident::NextScene){
        // //            //std::cout<<actor.nearby_dialogue<<"\n";
        // //            dialogue_queue.push_back(actor.contact_dialogue);
        // //        }
        // //        
        // //    } else if (glm::abs(actor.transform_position.x - mainActor->transform_position.x) <=1 && glm::abs(actor.transform_position.y - mainActor->transform_position.y) <=1){
        // //        if (actor.nearby_dialogue.empty()) continue;
        // //        checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
        // //        //dialogue_ss<<actor.nearby_dialogue<<"\n";
        // //        if (actor.nearby_dialogue != ""&& actor.nearby_incident != GameIncident::NextScene) {
        // //            //std::cout<<actor.nearby_dialogue<<"\n";
        // //            dialogue_queue.push_back(actor.nearby_dialogue);
        // //        }
        // //        
        // //    }
        // //}
        // for (Actor* actor: collision_sets[mainActor->id]){
        //     if (actor == mainActor) continue;
        //     if (actor->contact_dialogue.empty()) continue;
        //     checkGameIncidents(actor, allIncidents, ContactType::Overlap);
        //     if (actor->contact_dialogue != "" && actor->contact_incident != GameIncident::NextScene){
        //         //std::cout<<actor.nearby_dialogue<<"\n";
        //         //dialogue_queue.push_back(actor->contact_dialogue);
        //     }
        //     if (actor->contact_incident == GameIncident::HealthDown){
        //         attack_on_this_frame_set.insert(actor);
        //     }
        // }
        // // SPEC SAYS NO CONTACT DIALOGUE!
        // // I BELIEVE THIS WILL BE BACK IN FUTURE
        // //for (size_t i=0; i<dialogue_queue.size(); i++){
        // //    text_to_render.emplace_back( dialogue_queue[i], 25, window_size.y - 50 - 50* (dialogue_queue.size()-1 -i));
        // //}
    }

    void GameEngine::updateGameIncidents(std::vector<GameIncident>& incidents) {
        // if (!mainActor) return;
        // for (GameIncident& incident : incidents){
        //     switch (incident){
        //         case GameIncident::HealthDown:
        //             if (Helper::GetFrameNumber() - coolDownTriggerFrame >= 180){ // Cool down period of 3 seconds at 60 FPS 
        //                 health -= 1;
        //                 coolDownTriggerFrame = Helper::GetFrameNumber();
        //                 damage_sfx.play(&audioManager);
        //             }
        //             break;
        //         case GameIncident::ScoreUp:
        //             score += 1;
        //             score_sfx.play(&audioManager);
        //             break;
        //         case GameIncident::YouWin:
        //             states = GameState::Won;
        //             gameEndFirstFrame = true;
        //             break;
        //         case GameIncident::GameOver:
        //             if (Helper::GetFrameNumber() - coolDownTriggerFrame >= 180){ // Cool down period of 3 seconds at 60 FPS to prevent rapid score increase
        //                 gameEndFirstFrame = true;
        //                 states = GameState::Lost;
        //                 coolDownTriggerFrame = Helper::GetFrameNumber();
        //             }
        //             break;
        //         case GameIncident::NextScene:
        //             states = GameState::NextScene;
        //             break;
        //         default:
        //             break;
        //     }
        // }
    }

    //TO DO check when read in
    void GameEngine::checkGameIncidents(Actor* actor, std::vector<GameIncident>& allIncidents, ContactType contactType) {
        // if (!mainActor) return;
        // //if (Helper::GetFrameNumber() - coolDownTriggerFrame < 180){ // Cool down period of 3 seconds at 60 FPS to prevent rapid re-triggering of incidents
        // //    return;
        // //}
        // switch (contactType){
        //     case ContactType::Nearby:
        //         if (actor->nearby_incident != GameIncident::None
        //         && (actor->nearby_incident != GameIncident::ScoreUp || !actor->triggered_scoreUp)){
        //             allIncidents.push_back(actor->nearby_incident);
        //             if (actor->nearby_incident == GameIncident::ScoreUp){
        //                 actor->triggered_scoreUp = true;
        //             } else if (actor->nearby_incident == GameIncident::NextScene){
        //                 next_scene_name = actor->nearby_scene;
        //             } 

        //             // TODO deal with hurt renderer
        //             //dialogue_ss<<actor->nearby_dialogue<<"\n";
        //         }
        //         break;
        //     case ContactType::Overlap:
        //         if (actor->contact_incident != GameIncident::None
        //         && (actor->contact_incident != GameIncident::ScoreUp || !actor->triggered_scoreUp)){
        //             allIncidents.push_back(actor->contact_incident); 
        //             if (actor->contact_incident == GameIncident::ScoreUp){
        //                 actor->triggered_scoreUp = true;
        //             }
        //             if (actor->contact_incident == GameIncident::NextScene){
        //                 next_scene_name = actor->contact_scene;
        //             }
        //             //dialogue_ss<<actor->contact_dialogue<<"\n";
        //         }
        //         break;
        // }
        return;
    }

    void GameEngine::updateDialoguesTrigger(std::vector<GameIncident>& allIncidents, std::vector<std::string>* dialogue_queue){ 
        // //for (Actor& actor : *actorList){
        // //    if (actor.has_box_trigger == false) continue; // If the actor doesn't have a box trigger, skip
        // //    if (checkAABB(mainActor->transform_position, mainActor->box_trigger, 
        // //        actor.transform_position, actor.box_trigger))
        // //    { 
        // //        if (actor.nearby_dialogue.empty()) continue;
        // //        if (actor.dialogue_info.audio_state == AudioState::Not_Started) actor.dialogue_info.play(&audioManager);
        // //        checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
        // //        if (actor.nearby_dialogue != "" && actor.nearby_incident != GameIncident::NextScene){
        // //            dialogue_queue->push_back(actor.nearby_dialogue);
        // //        }
        // //        if (actor.nearby_incident == GameIncident::HealthDown){
        // //            attack_on_this_frame_set.insert(&actor);
        // //        }
        // //        
        // //   }
        // //}
        // if (!mainActor) return;
        // if (!hasNearbyDialogue) return;
        //     Actor& actor = *mainActor;
        //     if (actor.has_box_trigger == false) return;
        //     glm::ivec2 center_cell = worldToCell(actor.transform_position, spatial_hash_cell_size_trigger);
        //     for (int i = -2; i <= 2; i++) {
        //         for (int j = -2; j <= 2; j++) {
        //             glm::ivec2 check_cell = center_cell + glm::ivec2(i, j);
        //             auto it = spatial_hash_trigger.find(check_cell);
        //             if (it != spatial_hash_trigger.end()) {
        //                 for (Actor* other_actor_ptr : it->second) {
        //                     if (!other_actor_ptr) continue;
        //                     if (other_actor_ptr == &actor) continue; // Skip self
        //                     if (other_actor_ptr->has_box_trigger == false) continue; // If the other actor doesn't have a box collider, skip collision detection
        //                     
        //                     if (checkAABB(actor.transform_position, actor.box_trigger, 
        //                         other_actor_ptr->transform_position, other_actor_ptr->box_trigger)
        //                         ){ 
        //                             if (other_actor_ptr->nearby_dialogue.empty()) continue;
        //                             if (other_actor_ptr->dialogue_info.audio_state == AudioState::Not_Started) 
        //                                 other_actor_ptr->dialogue_info.play(&audioManager);
        //                             //std::cout<<"checking trigger between "<<actor.actor_name<<" and "<<other_actor_ptr->actor_name<<"\n";
        //                             //std::cout<<"Actor "<<other_actor_ptr->actor_name<<" trigger dialogue: "<<other_actor_ptr->nearby_dialogue<<"\n";
        //                             checkGameIncidents(other_actor_ptr, allIncidents, ContactType::Nearby);
        //                             if (other_actor_ptr->nearby_dialogue != "" && other_actor_ptr->nearby_incident != GameIncident::NextScene){
        //                                 dialogue_queue->push_back(other_actor_ptr->nearby_dialogue);
        //                             }
        //                             if (other_actor_ptr->nearby_incident == GameIncident::HealthDown){
        //                                 attack_on_this_frame_set.insert(other_actor_ptr);
        //                             }
        //                         }
        //                 }
        //             }
        //         }
        //     }

    }

    void GameEngine::updateHurtAndAttackView(){
        // if (!mainActor) return;

        // bool mainActorHurted = false;
        // if (mainActor->has_view_image_damage && mainActor->damage_view_duration_frames > 0){
        //     mainActor->damage_view_duration_frames -= 1;
        // }

        // for (auto it = rendering_attack_actors_set.begin(); it != rendering_attack_actors_set.end(); ){
        //     Actor* actor = *it;
        //     if (!actor){
        //         it = rendering_attack_actors_set.erase(it);
        //         continue;
        //     }
        //     actor->attack_view_duration_frames -= 1;
        //     if (actor->attack_view_duration_frames <= 0){
        //         it = rendering_attack_actors_set.erase(it);
        //     } else {
        //         ++it;
        //     }
        // }

        // for (Actor* actor: attack_on_this_frame_set){
        //     if (!actor) continue;
        //     if (actor->canSetAttackView()){
        //         actor->setAttackViewDuration();
        //         rendering_attack_actors_set.insert(actor);
        //     }
        //     mainActorHurted = true;
        // }
        // if (mainActorHurted && mainActor->canSetDamageView()){
        //     mainActor->setDamageViewDuration();
        // }
    }

    //void dialogueRender(){
    //    std::cout<<dialogue_ss.str();
    //    //render_ss<<dialogue_ss.str();
    //    dialogue_ss.str(std::string());
    //    dialogue_ss.clear();
    //}
//
    //void generalRender(){
    //    // Render General Printing Messages
    //    //render_ss<<"health : "<<health<<", score : "<<score<<"\n";
    //    std::cout<<"health : "<<health<<", score : "<<score<<"\n";
    //}
//
    //void inquiryRender(){
    //    std::cout<<"Please make a decision...\n";
    //    std::cout<<input_query_message;
    //}

    void GameEngine::postUpdate() {
        // // Clear collision sets after processing collisions for the current frame
        // for (auto& collision_set : collision_sets) {
        //     collision_set.clear();
        // }
        // // clear trigger sets
        // attack_on_this_frame_set.clear();
    }

    void GameEngine::gameLoop() {
        // 简化版主循环：基于 SDL 事件与 Lua 组件生命周期驱动
        while (!input.GetQuit()) {
            // 事件处理（只依赖 Helper::SDL_PollEvent 和 Input 管理键盘 / 退出）
            while (Helper::SDL_PollEvent(&event)) {
                input.ProcessEvent(event);
                if (input.GetQuit()) {
                    break;
                }
            }

            // 驱动 Lua 组件生命周期（OnStart / OnUpdate / OnLateUpdate）
            update();

            // 目前渲染对本测试套件不重要，可按需填充 frameRender
            // frameRender(false);

            // 输入状态后处理（更新 GetKeyDown / GetKeyUp 等状态）
            input.LateUpdate();

            this->frameRender(false);
        }

        //if (imageDB) {
        //    imageDB->clearCache();
        //}
    }

    void GameEngine::frameRender(bool isInitialRender) {// render main
        this->imageManager->renderSSImages();
        this->imageManager->renderUI();
        this->textManager->renderAllText();
        this->imageManager->renderPixels();
        Helper::SDL_RenderPresent(ren);
        
        // (void)isInitialRender;
        // //std::cout << "clear color is "<< clear_color.x<< clear_color.y<< clear_color.z<< std::endl;
        // SDL_SetRenderDrawColor(ren, clear_color.x, clear_color.y, clear_color.z, 255);
        // SDL_RenderClear(ren);

        // //HUD
        // if (states == GameState::Ongoing || states == GameState::NextScene){
        //     if (hp_image != "" ){
        //         for (int i = 0; i < health; i++) {
        //             SDL_FRect dst = {5.0f + i * (hp_image_width + 5.0f), 25.0f, hp_image_width, hp_image_height};
        //             images_to_render.emplace_back(hp_image, dst);
        //         }
        //         text_to_render.push_back(TextRenderConfig("score : " + std::to_string(score), 5, 5));
        //         }   
        //     
        // }

        // if (states == GameState::Ongoing){
        //     // Render game scene based on actor positions and map
        //     
        //     if (actorList) {
        //         std::priority_queue<Actor*, std::vector<Actor*>, ActorRenderComparator> renderQueue;
        //         
        //         // Calculate world coordinates of screen corners
        //         glm::vec2 world_top_left = -camera / (100.0f * zoom_factor);
        //         glm::vec2 world_bottom_right = (glm::vec2(window_size.x, window_size.y) - camera) / (100.0f * zoom_factor);

        //         // Convert to spatial hash cells using window-based cell size
        //         glm::ivec2 window_top_left = worldToCell(world_top_left, spatial_hash_cell_size_window);
        //         glm::ivec2 window_bottom_right = worldToCell(world_bottom_right, spatial_hash_cell_size_window);

        //         // Iterate through visible cells in the window-based spatial hash
        //         // Add small buffer (±1 cell) to catch actors partially on screen
        //         for (int i = window_top_left.x - 1; i <= window_bottom_right.x + 1; i++) {
        //             for (int j = window_top_left.y - 1; j <= window_bottom_right.y + 1; j++) {
        //                 auto it = spatial_hash_window.find(glm::ivec2(i, j));
        //                 if (it != spatial_hash_window.end()) {
        //                     for (Actor* actor_ptr : it->second) {
        //                         if (!actor_ptr) continue;
        //                         if (actor_ptr->has_view_image) {
        //                             renderQueue.push(actor_ptr);
        //                         }
        //                     }
        //                 }
        //             }
        //         }
        //         
        //         // Render all visible actors in sorted order
        //         while (!renderQueue.empty()){
        //             Actor* renderActor = renderQueue.top();
        //             renderQueue.pop();
        //             imageDB->renderImageEx(
        //                 renderActor, camera, zoom_factor, Helper::GetFrameNumber()
        //             );
        //             // Visualize box collider if actor has one (for debugging)
        //             //visualizeBox(ren, renderActor->transform_position, renderActor->box_collider, camera, zoom_factor);
        //             //visualizeBox(ren, renderActor->transform_position, renderActor->box_trigger, camera, zoom_factor, SDL_Color{0, 255, 0, 255});
        //         }
        //     }
        // }
        // if (imageDB && !images_to_render.empty()) {
        //     for (const ImageRenderConfig& config : images_to_render) {
        //         if (!config.image_path.empty()) {
        //             imageDB->renderImage(config.image_path, config.dst); 
        //         }
        //         //std::cout<<"rendering image "<<config.image_path<<" at position "<<config.dst.x<<","<<config.dst.y<<"\n";
        //     }
        // }
        // if (textDB && !text_to_render.empty()) {
        //     for (const TextRenderConfig& config : text_to_render) {
        //         if (!config.text.empty()) {
        //             textDB->drawText(config.text, config.x, config.y);
        //         }
        //     }
        // }

        
        // Helper::SDL_RenderPresent(ren);
        // //mapRender();
        // //dialogueRender();
        // //generalRender();
        // //renderDialogue();
        // //if (states == GameState::Ongoing){
        // //    inquiryRender();
        // //}
        // //std::cout<<render_ss.str();
        // //render_ss.str(std::string());   
        // //render_ss.clear();
        // //if (states == GameState::NextScene){
        // //    initializeGame(false); // re-initialize game with next scene
        // //}
    }

    //void mapRender(){
    //    if (!mainActor) return;
    //    for (int i=0; i<viewSize.y; i++){
    //        int row = mainActor->position.y - viewSize.y/2 + i;
    //        for (int j=0; j<viewSize.x; j++){
    //            int col = mainActor->position.x - viewSize.x/2 + j;
    //            char render_char = ' ';
    //            auto it = mapHash.find(hashPosition(glm::ivec2(col, row)));
    //            if (it!=mapHash.end() && !it->second.empty()){
    //                // choose the actor with the largest id among those in this cell
    //                int best_idx = it->second[0];
    //                for (int idx : it->second){
    //                    if ((*actorList)[idx].id > (*actorList)[best_idx].id) best_idx = idx;
    //                }
    //                char rc = (*actorList)[best_idx].view;
    //                std::cout<<rc;
    //                continue;
    //            }
    //            std::cout<<render_char;
    //        }
    //        std::cout<<"\n";
    //    }
    //}

    //void finalRender() {
    //    switch (states){
    //    case GameState::Won:
    //        std::cout<<game_over_good_message;
    //        break;
    //    case GameState::Lost:
    //        std::cout<<game_over_bad_message;
    //        break;
    //    default:
    //        break;
    //    }
    //}

    void GameEngine::addActorToSpatialHash(Actor* actor, const glm::vec2& worldPos) {
        // if (!actor || !actorList) return;
        // spatial_hash[worldToCell(worldPos, spatial_hash_cell_size)].push_back(actor);
    }

    void GameEngine::removeActorFromSpatialHash(Actor* actor, const glm::vec2& worldPos) {
        // if (!actor || !actorList) return;
        // glm::ivec2 cell = worldToCell(worldPos, spatial_hash_cell_size);
        // auto& cell_actors = spatial_hash[cell];
        // cell_actors.erase(std::remove(cell_actors.begin(), cell_actors.end(), actor), cell_actors.end());
    }

    void GameEngine::moveActorToNewSpatialHash(Actor* actor, const glm::vec2& newWorldPos){
        // if (!hasCollision) return; // If collision detection is turned off, no need to update spatial hash
        // if (!actor || !actorList) return;
        // const glm::vec2& oldWorldPos = actor->transform_position;
        // if (worldToCell(oldWorldPos, spatial_hash_cell_size) == worldToCell(newWorldPos, spatial_hash_cell_size)){
        //     return; // If the actor is still in the same cell, no need to update the spatial hash
        // }
        // removeActorFromSpatialHash(actor, oldWorldPos);
        // addActorToSpatialHash(actor, newWorldPos);
    }

    void GameEngine::initializeSpatialHash() {
        // spatial_hash.clear();
        // if (!actorList) return;
        // for (Actor& actor : *actorList){
        //     addActorToSpatialHash(&actor, actor.transform_position);
        // }
    }

    
    void GameEngine::initializeSpatialHashTrigger() {
        // spatial_hash_trigger.clear();
        // if (!actorList) return;
        // for (Actor& actor : *actorList){
        //     if (actor.has_box_trigger){
        //         addActorToSpatialHashTrigger(&actor, actor.transform_position);
        //     }
        // }
    }

    void GameEngine::addActorToSpatialHashTrigger(Actor* actor, const glm::vec2& worldPos) {
        // if (!actor || !actorList) return;
        // spatial_hash_trigger[worldToCell(worldPos, spatial_hash_cell_size_trigger)].push_back(actor);
    }

    void GameEngine::removeActorFromSpatialHashTrigger(Actor* actor, const glm::vec2& worldPos) {
        // if (!actor || !actorList) return;
        // glm::ivec2 cell = worldToCell(worldPos, spatial_hash_cell_size_trigger);
        // auto& cell_actors = spatial_hash_trigger[cell];
        // cell_actors.erase(std::remove(cell_actors.begin(), cell_actors.end(), actor), cell_actors.end());
    }
    
    void GameEngine::moveActorToNewSpatialHashTrigger(Actor* actor, const glm::vec2& newWorldPos){
        // if (!hasNearbyDialogue) return; // If nearby dialogue is turned off, no need to update spatial hash for triggers
        // if (!actor || !actorList) return;
        // const glm::vec2& oldWorldPos = actor->transform_position;
        // if (worldToCell(oldWorldPos, spatial_hash_cell_size_trigger) == worldToCell(newWorldPos, spatial_hash_cell_size_trigger)){
        //     return; // If the actor is still in the same cell, no need to update the spatial hash
        // }
        // removeActorFromSpatialHashTrigger(actor, oldWorldPos);
        // addActorToSpatialHashTrigger(actor, newWorldPos);
    }

    void GameEngine::initializeSpatialHashWindow() {
        // spatial_hash_window.clear();
        // if (!actorList) return;
        // for (Actor& actor : *actorList){
        //     if (actor.has_view_image){
        //         addActorToSpatialHashWindow(&actor, actor.transform_position);
        //     }
        // }
    }

    void GameEngine::addActorToSpatialHashWindow(Actor* actor, const glm::vec2& worldPos) {
        // if (!actor || !actorList) return;
        // spatial_hash_window[worldToCell(worldPos, spatial_hash_cell_size_window)].push_back(actor);
    }

    void GameEngine::removeActorFromSpatialHashWindow(Actor* actor, const glm::vec2& worldPos) {
        // if (!actor || !actorList) return;
        // glm::ivec2 cell = worldToCell(worldPos, spatial_hash_cell_size_window);
        // auto& cell_actors = spatial_hash_window[cell];
        // cell_actors.erase(std::remove(cell_actors.begin(), cell_actors.end(), actor), cell_actors.end());
    }
    
    void GameEngine::moveActorToNewSpatialHashWindow(Actor* actor, const glm::vec2& newWorldPos){
        // if (!actor || !actorList) return;
        // if (!actor->has_view_image) return; // Only track actors with view images
        // const glm::vec2& oldWorldPos = actor->transform_position;
        // if (worldToCell(oldWorldPos, spatial_hash_cell_size_window) == worldToCell(newWorldPos, spatial_hash_cell_size_window)){
        //     return; // If the actor is still in the same cell, no need to update the spatial hash
        // }
        // removeActorFromSpatialHashWindow(actor, oldWorldPos);
        // addActorToSpatialHashWindow(actor, newWorldPos);
    }
