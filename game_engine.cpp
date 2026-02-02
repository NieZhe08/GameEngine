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

class GameEngine {
public:
    Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> actorList;
    //Point camera; // Camera following the main actor
    glm::ivec2 mapSize; 
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

    std::string next_scene_name;

    GameEngine() {
        initializeGame();
    }

    void initializeGame(bool isInitialLoad = true) {
        JsonParser parser;
        game_start_message = parser.getGameStartMessage();
        game_over_good_message = parser.getGameOverGoodMessage();
        game_over_bad_message = parser.getGameOverBadMessage();
        viewSize = parser.getResolution(); 

        // load scene module
        SceneDB sceneDB(parser.getInitialScene(), mapHash);
        actorList = sceneDB.getSceneActors();
        // After moving actorList, resolve mainActor to point inside actorList
        int mainIndex = sceneDB.getMainActorIndex();
        if (mainIndex >= 0 && actorList && mainIndex < static_cast<int>(actorList->size())){
            mainActor = &(*actorList)[mainIndex];
        } else {
            mainActor = nullptr;
        }
        mapSize = sceneDB.getMapSize();

        health = 3;
        score = 0;
        states = GameState::Ongoing;
        scored_actors = std::vector<std::string>();
        next_scene_name = "";
        // Initialize game state, load map, actors, etc.
        frameRender(isInitialLoad);
    }

    void gameLoop() {
        //initializeGame();
        while (states == GameState::Ongoing){
            PlayerAction action = getPlayerAction();
            updateGameState(action);
            if (action == PlayerAction::Quit){
                states = GameState::Lost;
                break;
            }
            frameRender(false);
        };
        finalRender();
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
                    int idx = static_cast<int>(&actor - &(*actorList)[0]);
                    auto removeIt = std::remove(vectorIt->second.begin(), vectorIt->second.end(), idx);
                    vectorIt->second.erase(removeIt, vectorIt->second.end());
                }
                // push actor index into new cell
                int new_idx = static_cast<int>(&actor - &(*actorList)[0]);
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
        if (mapHash.find(hashPosition(position))!=mapHash.end()){
            for (int idx : mapHash[hashPosition(position)]){
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
        if (!actorList || !mainActor) return allIncidents;
        for (int i = -1; i<2; i++){
            for (int j = -1; j<2; j++){
                glm::ivec2 check_position = mainActor->position + glm::ivec2(i, j);
                auto it = mapHash.find(hashPosition(check_position));
                if (it != mapHash.end() && !it->second.empty()){
                    for (int actor_idx : it->second){
                        Actor& actor = (*actorList)[actor_idx];
                        if (actor.position.x == mainActor->position.x && actor.position.y == mainActor->position.y){
                            if (&actor == mainActor) continue;
                            if (actor.contact_dialogue.empty()) continue;
                            // dialogue handling in checkGameIncidents
                            checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
                            //dialogue_ss<<actor.contact_dialogue<<"\n";
                        } else {
                            if (actor.nearby_dialogue.empty()) continue;
                            checkGameIncidents(&actor, allIncidents, ContactType::Nearby);
                            //dialogue_ss<<actor.nearby_dialogue<<"\n";
                        }
                    }
                }
            }
        }
        /*
        for (Actor& actor : *actorList){
            if (actor.position.x == mainActor->position.x && actor.position.y == mainActor->position.y){
                if (&actor == mainActor) continue;
                if (actor.contact_dialogue.empty()) continue;
                checkGameIncidents(&actor, allIncidents, ContactType::Overlap);
                dialogue_ss<<actor.contact_dialogue<<"\n";
            } else if (abs(actor.position.x - mainActor->position.x) <=1 && abs(actor.position.y - mainActor->position.y) <=1){
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
                    dialogue_ss<<actor->nearby_dialogue<<"\n";
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
                    dialogue_ss<<actor->contact_dialogue<<"\n";
                }
                break;
        }
        return;
    }

    void dialogueRender(){
        render_ss<<dialogue_ss.str();
        dialogue_ss.str(std::string());
        dialogue_ss.clear();
    }

    void generalRender(){
        // Render General Printing Messages
        render_ss<<"health : "<<health<<", score : "<<score<<"\n";
    }

    void inquiryRender(){
        render_ss<<"Please make a decision...\n";
        render_ss<<input_query_message;
    }

    void frameRender(bool isInitialRender = false) {// render main
        if (isInitialRender){
            if (!game_start_message.empty()) {
            std::cout<<game_start_message<<"\n";
            }
        }
            
        mapRender();
        dialogueRender();
        generalRender();
        //renderDialogue();
        if (states == GameState::NextScene){
            initializeGame(false); // re-initialize game with next scene
        }
        if (states == GameState::Ongoing){
            inquiryRender();
        }
        std::cout<<render_ss.str();
        render_ss.str(std::string());   
        render_ss.clear();
    }

    void mapRender(){
        if (!mainActor) return;
        //render_ss<<"name of main actor: "<<mainActor->actor_name<<", view"<<mainActor->view<<"\n";
        for (int i=0; i<viewSize.y; i++){
            int row = mainActor->position.y - viewSize.y/2 + i;
            for (int j=0; j<viewSize.x; j++){
                int col = mainActor->position.x - viewSize.x/2 + j;
                /*
                if (row<0 || row>=mapSize.y || col<0 || col>=mapSize.x){ 
                    render_ss<<"-"; // out of bounds
                    continue;
                }
                */
                
               char render_char = ' ';
                auto it = mapHash.find(hashPosition(glm::ivec2(col, row)));
                if (it!=mapHash.end() && !it->second.empty()){
                    // choose the actor with the largest id among those in this cell
                    int best_idx = it->second[0];
                    for (int idx : it->second){
                        if ((*actorList)[idx].id > (*actorList)[best_idx].id) best_idx = idx;
                    }
                    char rc = (*actorList)[best_idx].view;
                    //if (rc == '\0') rc = '?'; // should not happen
                    render_ss<<rc;
                    continue;
                }
                render_ss<<render_char;
                /*
                bool has_actor = false;
                char actorView = ' ';
                if (hardcoded_map[row][col]==' '){
                    for (Actor& actor : hardcoded_actors){
                        if (actor.position.x == col && actor.position.y == row){
                            actorView = actor.view;
                            has_actor = true;
                        }
                    }
                } 
                if (has_actor) {
                    render_ss<<actorView;
                } else if (!has_actor){
                    render_ss<<hardcoded_map[row][col];
                }
                */ // TODO 
                
            }
            render_ss<<"\n";
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