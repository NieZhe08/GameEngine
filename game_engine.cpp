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

class GameEngine {
public:
    Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> actorList;
    //Point camera; // Camera following the main actor
    glm::ivec2 mapSize; // x_resolution and y_resolution of the map
    glm::ivec2 viewSize;
    // VIEWSIZE: here viewsize refers to viewSize (row, col)
    //std::string _game_start_message;
    int health;
    int score;
    std::string input_query_message = std::string("Your options are \"n\", \"e\", \"s\", \"w\", \"quit\"\n");// TODO
    
    std::unordered_map<std::uint64_t, std::vector<int>> mapHash;
    std::stringstream render_ss;
    std::stringstream dialogue_ss;
    GameState states;
    std::vector<std::string> scored_actors; //TODO

    std::string game_start_message;
    std::string game_over_good_message;
    std::string game_over_bad_message;

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
    Mix_Chunk* intro_bgm_chunk;
    bool intro_bgm_playing = false;

    std::string next_scene_name;

    GameEngine() {
        next_scene_name = "";
        initializeGame();
    }

    void initializeGame(bool isInitialLoad = true) {
        if  (isInitialLoad) {
            JsonParser parser;
            game_start_message = parser.getGameStartMessage();
            game_over_good_message = parser.getGameOverGoodMessage();
            game_over_bad_message = parser.getGameOverBadMessage();
            //viewSize = parser.getResolution();
            
            next_scene_name = parser.getInitialScene();

            //SDL
            game_title = parser.getGameTitle();
            window_size = parser.getResolution();
            clear_color = parser.getClearColor();

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

            intro_bgm_chunk = audioDB.readIntroBGM();
            intro_bgm_playing  = audioDB.hasIntroBGM();
        }

        // Clear mapHash before loading new scene to avoid stale actor indices
        mapHash.clear();

        // load scene module
        SceneDB sceneDB(next_scene_name, mapHash);
        actorList = sceneDB.getSceneActors();
        // After moving actorList, resolve mainActor to point inside actorList
        int mainIndex = sceneDB.getMainActorIndex();
        if (mainIndex >= 0 && actorList && mainIndex < static_cast<int>(actorList->size())){
            mainActor = &(*actorList)[mainIndex];
        } else {
            mainActor = nullptr;
        }
        mapSize = sceneDB.getMapSize();

        if (isInitialLoad){
            health = 3;
            score = 0;
        }
        
        states = GameState::Ongoing;
        scored_actors = std::vector<std::string>();
        next_scene_name = "";
        // Initialize game state, load map, actors, etc.
        //frameRender(isInitialLoad);
    }

    void gameLoop() {
        //initializeGame();
        size_t image_idx = 0;
        size_t text_idx = 0;
        if (intro_bgm_playing ) AudioHelper::Mix_PlayChannel(0, intro_bgm_chunk, -1); // Play intro BGM in a loop
        while (states == GameState::Ongoing){
            images_to_render.clear();
            text_to_render.clear();
            while (Helper::SDL_PollEvent(&event)){
                if (event.type == SDL_QUIT){
                    states = GameState::Lost;
                    imageDB->clearCache();
                    break;
                }
                
                if (intro_image && !intro_image->empty()) {
                    SDL_Keycode key_press = event.key.keysym.scancode;
                    if (key_press == SDL_SCANCODE_SPACE || key_press == SDL_SCANCODE_RETURN || 
                        (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)) {
                        image_idx++;
                        text_idx++;
                    }
                }
            }
            if (intro_image && (image_idx < intro_image->size() || (intro_text && text_idx < intro_text->size()))) {
                images_to_render.emplace_back((*intro_image)[image_idx >= intro_image->size()? intro_image->size()-1 : image_idx], SDL_FRect{0, 0, static_cast<float>(window_size.x), static_cast<float>(window_size.y)});
                if (intro_text && !intro_text->empty()) {
                    text_to_render.emplace_back((*intro_text)[text_idx >= intro_text->size()? intro_text->size()-1 : text_idx], 25, window_size.y - 50);
                }
            } else {
                if (intro_bgm_playing) AudioHelper::Mix_HaltChannel(0);
            }
                
            frameRender(false);
        };
        //finalRender();
    }

    PlayerAction getPlayerAction() { // input main
        std::string input = "";
        std::cin>>input;
        if (input=="quit") return PlayerAction::Quit;
        else if (input=="n") return PlayerAction::MoveUp;
        else if (input=="s") return PlayerAction::MoveDown;
        else if (input=="w") return PlayerAction::MoveLeft;
        else if (input=="e") return PlayerAction::MoveRight;
        else return PlayerAction::Invalid;
    }

    void updateActorPositions(PlayerAction playerAction) {
        // Update NPC positions based on their velocities
        if (!actorList) return;
        for (Actor& actor : *actorList){
            glm::ivec2 nextPosition;
            bool hasMoved = false;
            if (&actor == mainActor) {
                auto result = updatePlayerPosition(playerAction);
                nextPosition = result.first;
                hasMoved = result.second;
            } else {
                if (actor.velocity != glm::ivec2(0,0)){
                    hasMoved = true;
                }
                nextPosition = actor.position + actor.velocity;
            }
            if (!hasMoved) continue; // skip the following steps if the actor has not moved
            // collision detection only worry about whether collision happens between actor itself and others
            if (!collisionDetected(nextPosition, & actor)){// need to update mapHash
                // remove the actor's index from its old cell
                auto vectorIt = mapHash.find(hashPosition(actor.position));
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
                actor.position = nextPosition;
            } else {
                actor.velocity = -actor.velocity; // Reverse direction on collision
            }
        }
    }


    void updateGameState(PlayerAction action) { // update main
        // Update game state based on player action
        updateActorPositions(action);
        if (action == PlayerAction::Quit){
            health = 0; // End game
        } else {
            //updatePlayerPosition(action);
            std::vector<GameIncident> incidents;
            updateDialogues(incidents);
            updateGameIncidents(incidents);
            if (health <= 0){
                states = GameState::Lost;
            }
        }
    }

    // TO DO
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
        glm::ivec2 nextPosition = mainActor->position;
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
        // Update dialogues based on proximity to other actors
        // Use min-heap (std::greater) so that smaller actor IDs are processed first
        std::priority_queue<int, std::vector<int>, std::greater<int>> actorsToBeDealted;
        if (!actorList || !mainActor) return allIncidents;
        for (int i = -1; i<2; i++){
            for (int j = -1; j<2; j++){
                glm::ivec2 check_position = mainActor->position + glm::ivec2(i, j);
                auto it = mapHash.find(hashPosition(check_position));
                if (it != mapHash.end() && !it->second.empty()){
                    for (int actor_idx : it->second){
                        actorsToBeDealted.push(actor_idx);
                    }
                }
            }
        }
        while (!actorsToBeDealted.empty()){
            int actor_idx = actorsToBeDealted.top();
            actorsToBeDealted.pop();
            Actor& actor = (*actorList)[actor_idx];
                if (actor.position.x == mainActor->position.x && actor.position.y == mainActor->position.y){
                    if (&actor == mainActor) continue;
                    if (actor.contact_dialogue.empty()) continue;
                    checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
                    dialogue_ss<<actor.contact_dialogue<<"\n";
                } else {
                    if (actor.nearby_dialogue.empty()) continue;
                    checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
                    dialogue_ss<<actor.nearby_dialogue<<"\n";
                }
        }
        /*
        for (Actor& actor : *actorList){
            if (actor.position.x == mainActor->position.x && actor.position.y == mainActor->position.y){
                if (&actor == mainActor) continue;
                if (actor.contact_dialogue.empty()) continue;
                checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
                dialogue_ss<<actor.contact_dialogue<<"\n";
            } else if (glm::abs(actor.position.x - mainActor->position.x) <=1 && glm::abs(actor.position.y - mainActor->position.y) <=1){
                if (actor.nearby_dialogue.empty()) continue;
                checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
                dialogue_ss<<actor.nearby_dialogue<<"\n";
            }
        }
        */
        return allIncidents;
    }

    void updateGameIncidents(std::vector<GameIncident>& incidents) {
        for (GameIncident& incident : incidents){
            switch (incident){
                case GameIncident::HealthDown:
                    health -= 1;
                    break;
                case GameIncident::ScoreUp:
                    score += 1;
                    break;
                case GameIncident::YouWin:
                    states = GameState::Won;
                    break;
                case GameIncident::GameOver:
                    states = GameState::Lost;
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

    void dialogueRender(){
        std::cout<<dialogue_ss.str();
        //render_ss<<dialogue_ss.str();
        dialogue_ss.str(std::string());
        dialogue_ss.clear();
    }

    void generalRender(){
        // Render General Printing Messages
        //render_ss<<"health : "<<health<<", score : "<<score<<"\n";
        std::cout<<"health : "<<health<<", score : "<<score<<"\n";
    }

    void inquiryRender(){
        std::cout<<"Please make a decision...\n";
        std::cout<<input_query_message;
    }

    void frameRender(bool isInitialRender = false) {// render main
        (void)isInitialRender;
        SDL_SetRenderDrawColor(ren, clear_color.x, clear_color.y, clear_color.z, 255);
        SDL_RenderClear(ren);
        if (imageDB && !images_to_render.empty()) {
            for (const ImageRenderConfig& config : images_to_render) {
                if (!config.image_path.empty()) {
                    imageDB->renderImage(config.image_path, config.dst);
                }
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

    void mapRender(){
        if (!mainActor) return;
        for (int i=0; i<viewSize.y; i++){
            int row = mainActor->position.y - viewSize.y/2 + i;
            for (int j=0; j<viewSize.x; j++){
                int col = mainActor->position.x - viewSize.x/2 + j;
                char render_char = ' ';
                auto it = mapHash.find(hashPosition(glm::ivec2(col, row)));
                if (it!=mapHash.end() && !it->second.empty()){
                    // choose the actor with the largest id among those in this cell
                    int best_idx = it->second[0];
                    for (int idx : it->second){
                        if ((*actorList)[idx].id > (*actorList)[best_idx].id) best_idx = idx;
                    }
                    char rc = (*actorList)[best_idx].view;
                    std::cout<<rc;
                    continue;
                }
                std::cout<<render_char;
            }
            std::cout<<"\n";
        }
    }

    void finalRender() {
        switch (states){
        case GameState::Won:
            std::cout<<game_over_good_message;
            break;
        case GameState::Lost:
            std::cout<<game_over_bad_message;
            break;
        default:
            break;
        }
    }
};