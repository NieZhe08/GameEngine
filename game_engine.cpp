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
#include "SDL2/SDL_ttf.h"
#include "image_db.h"
#include "text_db.h"
#include "audio_db.h"
#include "AudioHelper.h"
#include <queue>
#include <deque>

class GameEngine {
public:
    Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> actorList;
    //Point camera; // Camera following the main actor
    glm::ivec2 mapSize; // x_resolution and y_resolution of the map
    glm::ivec2 viewSize;
    // VIEWSIZE: here viewsize refers to viewSize (row, col)
    //std::string _game_start_message;
    //int health;
    //int score;
    //std::string input_query_message = std::string("Your options are \"n\", \"e\", \"s\", \"w\", \"quit\"\n");// TODO
    
    std::unordered_map<std::uint64_t, std::vector<int>> mapHash;
    //std::stringstream render_ss;
    //std::stringstream dialogue_ss;
    GameState states;
    //std::vector<std::string> scored_actors; //TODO

    //std::string game_start_message;
    //std::string game_over_good_message;
    //std::string game_over_bad_message;

    // SDL rendering stuff
    Helper helper;
    std::string  game_title = "";
    glm::ivec2 window_size;
    glm::ivec3 clear_color;
    SDL_Event event;
    SDL_Window* win;
    SDL_Renderer* ren;
    ImageDB* imageDB;
    TextDB* textDB;
    std::unique_ptr<std::vector<std::string>> intro_image;
    std::unique_ptr<std::vector<std::string>> intro_text;
    std::vector<ImageRenderConfig> images_to_render; // Queue of images to render each frame
    std::vector<TextRenderConfig> text_to_render; // Queue of text to render each frame

    AudioDB audioDB;
    Mix_Chunk* intro_bgm_chunk; // Intro Animation BGM chunk
    Mix_Chunk* scene_bgm_chunk; // Game Scene BGM chunk
    Mix_Chunk* gamewin_bgm_chunk; // Game Win BGM chunk
    Mix_Chunk* gamelose_bgm_chunk; // Game Lose BGM chunk
    
    std::string next_scene_name;
    
    // Intro Animation Stage variables
    size_t image_idx = 0;
    size_t text_idx = 0;
    AudioState intro_bgm_states = AudioState::Not_Started;

    // Game Scene Wise Variables
    // Call when states == GameState::Ongoing, update when GameState::NextScene
    AudioState scene_bgm_states = AudioState::Not_Started;
    glm::vec2 camera = glm::vec2(0,0); // Camera following the main actor
    // HUD and decisive game variables
    int health = 3;
    int score = 0;
    std::string hp_image = "";
    float hp_image_width = 0.0f;
    float hp_image_height = 0.0f;
    glm::vec2 camera_lift = glm::vec2(0,0); // Additional camera lift applied on top of main actor centering, can be used for effects
    int coolDownTriggerFrame = -180;
    bool endingFlag = false; // Flag to indicate if the game is in the ending sequence after winning or losing
    GameState endingState = GameState::Ongoing; // Store whether the ending sequence is for winning or losing

    //Ending Game Stage variables
    AudioState gamewin_bgm_states = AudioState::Not_Started;
    AudioState gamelose_bgm_states = AudioState::Not_Started;
    std::string gamewin_image = "";
    std::string gamelose_image = "";
    bool has_gameend_stage = false; // Flag to indicate if there is a separate game end stage (with its own image and BGM) after winning or losing
    bool gameEndFirstFrame = false;

    GameEngine() {
        //next_scene_name = "";
        initializeGame();
    }

    void initializeGame(bool isInitialLoad = true) {
            
        //game_start_message = parser.getGameStartMessage();
        //game_over_good_message = parser.getGameOverGoodMessage();
        //game_over_bad_message = parser.getGameOverBadMessage();
        ////viewSize = parser.getResolution();
        JsonParser parser;
        if (isInitialLoad){
            next_scene_name = parser.getInitialScene();

            //SDL
            game_title = parser.getGameTitle();
            window_size = parser.getResolution();
            clear_color = parser.getClearColor();
            //std::cout<<"window_size: "<<window_size.x<<" "<<window_size.y<<"\n";

            SDL_Init(SDL_INIT_VIDEO);
            TTF_Init();
            win = helper.SDL_CreateWindow(game_title.c_str(), 100, 100, window_size.x, window_size.y, SDL_WINDOW_SHOWN);
            ren = Helper::SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            imageDB = new ImageDB(ren);
            imageDB->readIntroImage();
            intro_image = imageDB->getIntroImageVector();

            textDB = new TextDB(ren, (intro_image && !intro_image->empty())); // Only enable text rendering if intro images are present
            textDB->readIntroText();
            intro_text = textDB->getIntroTextVector();

            intro_bgm_chunk = audioDB.readBGM("intro_bgm");
            intro_bgm_states  = audioDB.hasIntroBGM();

            states = GameState::IntroAnimation;
            if (!intro_image || intro_image->empty()){
            states = GameState::Ongoing; // Skip intro animation if no intro image
            }

            // game ending 
            gamewin_image = imageDB->getGameWinImage();
            gamelose_image = imageDB->getGameLoseImage();
            has_gameend_stage = !gamewin_image.empty() || !gamelose_image.empty();
            gamewin_bgm_chunk = audioDB.readBGM("game_over_good_audio");
            gamelose_bgm_chunk = audioDB.readBGM("game_over_bad_audio");
            gamewin_bgm_states = audioDB.hasGameWinBGM();
            gamelose_bgm_states = audioDB.hasGameLoseBGM();

        } else {
            states = GameState::Ongoing; // Load next scene directly without intro animation
        }
        
        scene_bgm_chunk = audioDB.readBGM("gameplay_audio");
        scene_bgm_states = audioDB.hasGameplayBGM();

        camera_lift = parser.getCameraOffset() * 100.0f; // Apply camera offset from config, scaled by 100 to convert from tile units to pixel units
        camera = glm::vec2(window_size.x /2.0f, window_size.y /2.0f) + 
                camera_lift; // Initialize camera to center of the window

        // Clear mapHash before loading new scene to avoid stale actor indices
        mapHash.clear();

        //std::cout<<"Loading scene: "<<next_scene_name<<"\n";
        // load scene module
        SceneDB sceneDB(next_scene_name, mapHash, imageDB);
        actorList = sceneDB.getSceneActors();
        if (sceneDB.hasMainActor()){
            hp_image = parser.getHPimage();
            SDL_Texture* hp_tex = imageDB->loadImage(hp_image);
            Helper::SDL_QueryTexture(hp_tex,  &hp_image_width, &hp_image_height);
            textDB->loadFont();
        } 
        // After moving actorList, resolve mainActor to point inside actorList
        int mainIndex = sceneDB.getMainActorIndex();
        if (mainIndex >= 0 && actorList && !actorList->empty() && mainIndex < static_cast<int>(actorList->size())){
            mainActor = &(*actorList)[mainIndex];
        } else {
            mainActor = nullptr;
        }
        mapSize = sceneDB.getMapSize();

        //if (isInitialLoad){
        //    health = 3;
        //    score = 0;
        //}
        
        //scored_actors = std::vector<std::string>();
        next_scene_name = "";
        endingFlag = false;
        endingState = GameState::Ongoing;
        // Initialize game state, load map, actors, etc.
        //frameRender(isInitialLoad);
    }

    void update(){
        switch (states) {
            case GameState::IntroAnimation:
                updateIntroAnimation();
                break;
            case GameState::Ongoing:
                updateOngoing();
                break;
            case GameState::NextScene:
                //initializeGame(false); // Load next scene without resetting game state
                break;
            case GameState::Won:
                updateGameEnd(true);
                break;
            case GameState::Lost:
                updateGameEnd(false);
                break;
            case GameState::Quit:
                // Handle end game logic, show messages, etc.
                break;
        }
    }

    void updateIntroAnimation() {
        //if (states != GameState::IntroAnimation) return;
        if (intro_bgm_states == AudioState::Not_Started){
            AudioHelper::Mix_PlayChannel(0, intro_bgm_chunk, -1); // Play intro BGM in a loop
            intro_bgm_states = AudioState::Playing;
        } 

        /* below is a FRAME-WISE update for Intro Animation Stage*/
        images_to_render.clear();
        text_to_render.clear();
        while (Helper::SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                //states = GameState::Lost;
                endingFlag = true;
                endingState = GameState::Quit;
                break; // exit the Intro Animation Stage
            }
            if (intro_image && !intro_image->empty()) {
                if (event.type == SDL_KEYDOWN) {
                    SDL_Keycode key_press = event.key.keysym.scancode;
                    if (key_press == SDL_SCANCODE_SPACE || key_press == SDL_SCANCODE_RETURN) {
                        image_idx++;
                        text_idx++;
                    }
                } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    image_idx++;
                    text_idx++;
                }
            }
        }
        if (intro_image && !intro_image->empty() && (image_idx < intro_image->size() || (intro_text && !intro_text->empty() && text_idx < intro_text->size()))) {
            size_t img_idx = (image_idx >= intro_image->size()) ? intro_image->size()-1 : image_idx;
            if (img_idx < intro_image->size()) {
                images_to_render.emplace_back((*intro_image)[img_idx], SDL_FRect{0, 0, static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
            }
            if (intro_text && !intro_text->empty()) {
                size_t txt_idx = (text_idx >= intro_text->size()) ? intro_text->size()-1 : text_idx;
                if (txt_idx < intro_text->size()) {
                    text_to_render.emplace_back((*intro_text)[txt_idx], 25, window_size.y - 50);
                }
            }
        } else {// exit the Intro Animation Stage when all intro images and texts have been shown
            states = GameState::Ongoing;
            if (intro_bgm_states == AudioState::Playing){ 
                AudioHelper::Mix_HaltChannel(0);
            }
        }
    }

    void updateGameEnd(bool win) {
        //if (states != GameState::IntroAnimation) return;
        if (scene_bgm_states == AudioState::Playing){ 
                AudioHelper::Mix_HaltChannel(0);
                scene_bgm_states = AudioState::Stopped;
        }

        AudioState& bgm_state = win ? gamewin_bgm_states : gamelose_bgm_states;
        Mix_Chunk* bgm_chunk = win ? gamewin_bgm_chunk : gamelose_bgm_chunk;
        std::string end_image = win ? gamewin_image : gamelose_image;
        if (bgm_state == AudioState::Not_Started){
            AudioHelper::Mix_PlayChannel(0, bgm_chunk, 0); // Play intro BGM in a loop
            bgm_state = AudioState::Playing;
        } 

        /* below is a FRAME-WISE update for Intro Animation Stage*/
        images_to_render.clear();
        text_to_render.clear();
        images_to_render.emplace_back(end_image, SDL_FRect{0, 0, static_cast<float>(window_size.x), static_cast<float>(window_size.y)});

        while (Helper::SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                //states = GameState::Lost;
                endingFlag = true;
                endingState = GameState::Quit;
                break; // exit the GameEnd Stage
            }
        }
        
    }

    void updateOngoing(){
        if (states != GameState::Ongoing) return;

        if (scene_bgm_states == AudioState::Not_Started){
            AudioHelper::Mix_PlayChannel(0, scene_bgm_chunk, -1); // Play gameplay BGM in a loop
            scene_bgm_states = AudioState::Playing;
        }

        images_to_render.clear();
        text_to_render.clear();

        PlayerAction action = PlayerAction::Invalid;
        while (Helper::SDL_PollEvent(&event)) {
            action = updateGameState(event);
        }
        updateActorPositions(action);
        if (mainActor){
            glm::vec offset = glm::vec2((mainActor->transform_position.x) * 100 , 
                            (mainActor->transform_position.y) * 100 );
            camera = -offset  + glm::vec2(window_size.x /2.0f, window_size.y /2.0f) - camera_lift;
        }
        //std::cout<<"Camera Position: ("<<camera.x<<", "<<camera.y<<")\n";
        std::vector<GameIncident> incidents;
        updateDialogues(incidents);
        updateGameIncidents(incidents);
        if (health <= 0){
            //states = GameState::Lost;
            //endingFlag = true;
            //endingState = GameState::Lost;
            states = GameState::Lost;
            gameEndFirstFrame = true;
        }
    }

    void updateActorPositions(PlayerAction playerAction) {
        // Update NPC positions based on their velocities
        if (!actorList) return;
        bool hasMainPlayer = false;
        bool nonPlayerUpdate = (Helper::GetFrameNumber() &&Helper::GetFrameNumber() % 60 == 0); 
        for (Actor& actor : *actorList){
            glm::ivec2 nextPosition;
            bool hasMoved = false;
            if (&actor == mainActor) {
                if (hasMainPlayer){
                    //std::cout<<"Multiple main actors\n";
                }
                hasMainPlayer = true;
                auto result = updatePlayerPosition(playerAction);
                nextPosition = result.first;
                hasMoved = result.second;
                if (hasMoved) {
                    glm::ivec2 delta = nextPosition - actor.transform_position;
                    //std::cout<<"Frame Number:"<<Helper::GetFrameNumber() << ", Player Moved:("<<delta.x<<", "<<delta.y<<")\n";
                }
            } else {
                if (!nonPlayerUpdate) continue; // Only update non-player actors every 60 frames to slow down their movement
                if (actor.velocity != glm::ivec2(0,0)){
                    hasMoved = true;
                }
                nextPosition = actor.transform_position + actor.velocity;
            }
            if (!hasMoved) continue; // skip the following steps if the actor has not moved
            // collision detection only worry about whether collision happens between actor itself and others
            if (!collisionDetected(nextPosition, & actor)){// need to update mapHash
                // remove the actor's index from its old cell
                auto vectorIt = mapHash.find(hashPosition(actor.transform_position));
                if (vectorIt != mapHash.end()) {
                    //int idx = static_cast<int>(&actor - &(*actorList)[0]);// TODO
                    int idx = actor.id;
                    auto removeIt = std::remove(vectorIt->second.begin(), vectorIt->second.end(), idx);
                    vectorIt->second.erase(removeIt, vectorIt->second.end());
                }
                // push actor index into new cell
                //int new_idx = static_cast<int>(&actor - &(*actorList)[0]);
                int new_idx = actor.id;
                mapHash[hashPosition(nextPosition)].push_back(new_idx);
                actor.transform_position = nextPosition;
            } else {
                actor.velocity = -actor.velocity; // Reverse direction on collision
            }
        }
    }


    PlayerAction updateGameState(SDL_Event evt) { // update main
        if (evt.type == SDL_QUIT) {
            //states = GameState::Lost;
            endingFlag = true;
            endingState = GameState::Quit;
            return PlayerAction::Invalid;
        } else if (evt.type == SDL_KEYDOWN ) { // Only consider non-repeated keydown events for player actions
            SDL_Keycode key_press = evt.key.keysym.scancode;
            PlayerAction action = PlayerAction::Invalid;
            switch (key_press) {
                case SDL_SCANCODE_UP:
                    action = PlayerAction::MoveUp;
                    break;
                case SDL_SCANCODE_DOWN:
                    action = PlayerAction::MoveDown;
                    break;
                case SDL_SCANCODE_LEFT:
                    action = PlayerAction::MoveLeft;
                    break;
                case SDL_SCANCODE_RIGHT:
                    action = PlayerAction::MoveRight;
                    break;
                default:
                    break;
            }
            //Update game state based on player action
            //std::cout<<"Player Action: "<<(action != PlayerAction::Invalid ? std::to_string(static_cast<int>(action)) : "Invalid")<<"\n";
            //updateActorPositions(action);
            return action;
        }
        return PlayerAction::Invalid;
    }

    bool collisionDetected(glm::ivec2 position, Actor* actor_ptr) {
        //assert (true);// do check of index
        /*
        if (position.x<0 || position.x>=mapSize.x || position.y<0 || position.y>=mapSize.y){
            return true;
        }
        */
        std::uint64_t key = hashPosition(position);
        auto it = mapHash.find(key);
        if (it != mapHash.end()){
            for (int idx : it->second){
                Actor* actor = &(*actorList)[idx];
                //std::cout<<"Actor name:"<<actor->actor_name<<",blocking:"<<actor->blocking<<"\n";
                if (actor->blocking && actor != actor_ptr){
                    
                    return true;
                }
            }
        }
        return false;
    }

    std::pair<glm::ivec2, bool> updatePlayerPosition(PlayerAction action) {
        // Update player position based on action
        if (!mainActor) return std::make_pair(glm::ivec2(0,0), false);
        glm::ivec2 nextPosition = mainActor->transform_position;
        bool hasMoved = true;
        switch (action) {
            case PlayerAction::MoveUp:
                nextPosition.y -= 1;
                break;
            case PlayerAction::MoveDown:
                nextPosition.y += 1;
                break;
            case PlayerAction::MoveLeft:
                nextPosition.x -= 1;
                break;
            case PlayerAction::MoveRight:
                nextPosition.x += 1;
                break;
            default:
                hasMoved = false;
                break;
        }
        return std::make_pair(nextPosition, hasMoved);
        /*
        //std::cout<<"collision"<<(collisionDetected(nextPosition) ? " detected\n" : " not detected\n");
        if (!collisionDetected(nextPosition, mainActor->actor_name)){
            mainActor->position = nextPosition;
            return nextPosition;
        }
        return mainActor->position;
        */
        
    }

    std::vector<GameIncident> updateDialogues(std::vector<GameIncident>& allIncidents) {
        if (!mainActor) return allIncidents;
        // Update dialogues based on proximity to other actors
        // Use min-heap (std::greater) so that smaller actor IDs are processed first
        //std::priority_queue<int, std::vector<int>, std::greater<int>> actorsToBeDealted;
        //if (!actorList || !mainActor) return allIncidents;
        //for (int i = -10; i<20; i++){
        //    for (int j = -10; j<20; j++){
        //        glm::ivec2 check_position = mainActor->transform_position + glm::ivec2(i, j);
        //        auto it = mapHash.find(hashPosition(check_position));
        //        if (it != mapHash.end() && !it->second.empty()){
        //            for (int actor_idx : it->second){
        //                actorsToBeDealted.push(actor_idx);
        //            }
        //        }
        //    }
        //}
        //while (!actorsToBeDealted.empty()){
        //    int actor_idx = actorsToBeDealted.top();
        //    actorsToBeDealted.pop();
        //    Actor& actor = (*actorList)[actor_idx];
        //        if (actor.transform_position.x == mainActor->transform_position.x 
        //            && actor.transform_position.y == mainActor->transform_position.y){
        //            if (&actor == mainActor) continue;
        //            if (actor.contact_dialogue.empty()) continue;
        //            checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
        //            //dialogue_ss<<actor.contact_dialogue<<"\n";
        //            text_to_render.emplace_back( actor.contact_dialogue, 25, window_size.y / 2.0f - 50 - 50* (text_to_render.size()-1 ));
        //        } else {
        //            if (actor.nearby_dialogue.empty()) continue;
        //            checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
        //            //dialogue_ss<<actor.nearby_dialogue<<"\n";
        //            text_to_render.emplace_back( actor.contact_dialogue, 25, window_size.y / 2.0f - 50 - 50* (text_to_render.size()-1 ));
        //        }
        //}
        
        std::vector<std::string> dialogue_queue; // To store dialogues in the order they are processed
        for (Actor& actor : *actorList){
            if (actor.transform_position.x == mainActor->transform_position.x && actor.transform_position.y == mainActor->transform_position.y){
                if (&actor == mainActor) continue;
                if (actor.contact_dialogue.empty()) continue;
                checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
                //dialogue_ss<<actor.contact_dialogue<<"\n";
                if (actor.contact_dialogue != "" && actor.contact_incident != GameIncident::NextScene){
                    //std::cout<<actor.nearby_dialogue<<"\n";
                    dialogue_queue.push_back(actor.contact_dialogue);
                }
                
            } else if (glm::abs(actor.transform_position.x - mainActor->transform_position.x) <=1 && glm::abs(actor.transform_position.y - mainActor->transform_position.y) <=1){
                if (actor.nearby_dialogue.empty()) continue;
                checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
                //dialogue_ss<<actor.nearby_dialogue<<"\n";
                if (actor.nearby_dialogue != ""&& actor.nearby_incident != GameIncident::NextScene) {
                    //std::cout<<actor.nearby_dialogue<<"\n";
                    dialogue_queue.push_back(actor.nearby_dialogue);
                }
                
            }
        }
        for (size_t i=0; i<dialogue_queue.size(); i++){
            text_to_render.emplace_back( dialogue_queue[i], 25, window_size.y - 50 - 50* (dialogue_queue.size()-1 -i));
        }
        
        return allIncidents;
    }

    void updateGameIncidents(std::vector<GameIncident>& incidents) {
        if (!mainActor) return;
        for (GameIncident& incident : incidents){
            switch (incident){
                case GameIncident::HealthDown:
                    if (Helper::GetFrameNumber() - coolDownTriggerFrame >= 180){ // Cool down period of 3 seconds at 60 FPS 
                        health -= 1;
                        coolDownTriggerFrame = Helper::GetFrameNumber();
                    }
                    break;
                case GameIncident::ScoreUp:
                    score += 1;
                    break;
                case GameIncident::YouWin:
                    states = GameState::Won;
                    gameEndFirstFrame = true;
                    break;
                case GameIncident::GameOver:
                    if (Helper::GetFrameNumber() - coolDownTriggerFrame >= 180){ // Cool down period of 3 seconds at 60 FPS to prevent rapid score increase
                        gameEndFirstFrame = true;
                        states = GameState::Lost;
                        coolDownTriggerFrame = Helper::GetFrameNumber();
                    }
                    break;
                case GameIncident::NextScene:
                    states = GameState::NextScene;
                    break;
                default:
                    break;
            }
        }
    }

    //TO DO check when read in
    void checkGameIncidents(Actor* actor, std::vector<GameIncident>& allIncidents, ContactType contactType) {
        if (!mainActor) return;
        //if (Helper::GetFrameNumber() - coolDownTriggerFrame < 180){ // Cool down period of 3 seconds at 60 FPS to prevent rapid re-triggering of incidents
        //    return;
        //}
        switch (contactType){
            case ContactType::Nearby:
                if (actor->nearby_incident != GameIncident::None
                && (actor->nearby_incident != GameIncident::ScoreUp || !actor->triggered_scoreUp)){
                    allIncidents.push_back(actor->nearby_incident);
                    if (actor->nearby_incident == GameIncident::ScoreUp){
                        actor->triggered_scoreUp = true;
                    } else if (actor->nearby_incident == GameIncident::NextScene){
                        next_scene_name = actor->nearby_scene;
                    } 
                    //dialogue_ss<<actor->nearby_dialogue<<"\n";
                }
                break;
            case ContactType::Overlap:
                if (actor->contact_incident != GameIncident::None
                && (actor->contact_incident != GameIncident::ScoreUp || !actor->triggered_scoreUp)){
                    allIncidents.push_back(actor->contact_incident); 
                    if (actor->contact_incident == GameIncident::ScoreUp){
                        actor->triggered_scoreUp = true;
                    }
                    if (actor->contact_incident == GameIncident::NextScene){
                        next_scene_name = actor->contact_scene;
                    }
                    //dialogue_ss<<actor->contact_dialogue<<"\n";
                }
                break;
        }
        return;
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

    void gameLoop() {
        //initializeGame();
        while (states != GameState::Quit) {
            if (states == GameState::NextScene){
                initializeGame(false); // re-initialize game with next scene
            }
            update();
            if (states == GameState::NextScene){
                continue; // skip rendering for the current frame if we are transitioning to the next scene
            }
            if (gameEndFirstFrame && (states == GameState::Won || states == GameState::Lost)){
                updateGameEnd(states == GameState::Won);
                gameEndFirstFrame = false;
            }
            frameRender(false);
            if (endingFlag){
                states = endingState;
            }
            //std::cout<<"state"<<(static_cast<int>(states))<<"\n";
            endingFlag = false;
        }
        //finalRender();
        imageDB->clearCache();
    }

    void frameRender(bool isInitialRender = false) {// render main
        (void)isInitialRender;
        //std::cout << "clear color is "<< clear_color.x<< clear_color.y<< clear_color.z<< std::endl;
        SDL_SetRenderDrawColor(ren, clear_color.x, clear_color.y, clear_color.z, 255);
        SDL_RenderClear(ren);

        //HUD
        if (states == GameState::Ongoing || states == GameState::NextScene){
            if (hp_image != "" ){
                for (int i = 0; i < health; i++) {
                    SDL_FRect dst = {5.0f + i * (hp_image_width + 5.0f), 25.0f, hp_image_width, hp_image_height};
                    images_to_render.emplace_back(hp_image, dst);
                }
                text_to_render.push_back(TextRenderConfig("Score : " + std::to_string(score), 5, 5));
                }   
            
        }

        if (states == GameState::Ongoing){
            // Render game scene based on actor positions and map
            
            if (actorList) {
                std::priority_queue<const Actor*, std::vector<const Actor*>, ActorRenderComparator> renderQueue;
                for (const Actor& actor : *actorList) {
                    if (actor.has_view_image) {// TODO more effecient way to do 
                        renderQueue.push(&actor);
                        //std::cout<<"rendering actor "<<actor.actor_name<<" at position "<<actor.transform_position.x<<","<<actor.transform_position.y<<"\n";
                    }   
                }
                while (!renderQueue.empty()){
                    const Actor* renderActor = renderQueue.top();
                    renderQueue.pop();
                    imageDB->renderImageEx(
                        renderActor, camera
                    );
                }
            }
        }
        if (imageDB && !images_to_render.empty()) {
            for (const ImageRenderConfig& config : images_to_render) {
                if (!config.image_path.empty()) {
                    imageDB->renderImage(config.image_path, config.dst);
                }
                //std::cout<<"rendering image "<<config.image_path<<" at position "<<config.dst.x<<","<<config.dst.y<<"\n";
            }
        }
        if (textDB && !text_to_render.empty()) {
            for (const TextRenderConfig& config : text_to_render) {
                if (!config.text.empty()) {
                    textDB->drawText(config.text, config.x, config.y);
                }
            }
        }
        
        Helper::SDL_RenderPresent(ren);
        //mapRender();
        //dialogueRender();
        //generalRender();
        //renderDialogue();
        //if (states == GameState::Ongoing){
        //    inquiryRender();
        //}
        //std::cout<<render_ss.str();
        //render_ss.str(std::string());   
        //render_ss.clear();
        //if (states == GameState::NextScene){
        //    initializeGame(false); // re-initialize game with next scene
        //}
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

};