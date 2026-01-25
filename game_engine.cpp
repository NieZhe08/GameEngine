#include "MapHelper.h"
#include "glm/glm.hpp"
#include "string.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <vector>

enum class PlayerAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Quit,
    Invalid
};

enum class GameIncident {
    HealthDown,
    ScoreUp,
    YouWin,
    GameOver,
    None
};

enum class GameState {
    Ongoing,
    Won,
    Lost
};

class GameEngine {
public:
    Actor& mainActor = hardcoded_actors.back();
    //Point camera; // Camera following the main actor
    glm::ivec2 mapSize;
    glm::ivec2 viewSize;
    // VIEWSIZE: here viewsize refers to viewSize (row, col)
    //std::string _game_start_message;
    int health;
    int score;
    std::string input_query_message = std::string("Your options are \"n\", \"e\", \"s\", \"w\", \"quit\"");
    std::stringstream render_ss;
    std::stringstream dialogue_ss;
    GameState states;

    GameEngine() {
        mapSize = glm::ivec2(HARDCODED_MAP_WIDTH, HARDCODED_MAP_HEIGHT);
        viewSize = glm::ivec2(13, 9); 
        health = 3;
        score = 0;
        states = GameState::Ongoing;
    }

    void initializeGame() {
        // Initialize game state, load map, actors, etc.
        std::cout<<game_start_message<<"\n";
        renderFrame(true);
    }

    void gameLoop() {
        initializeGame();
        while (states == GameState::Ongoing){
            PlayerAction action = getPlayerAction();
            updateGameState(action);
            if (action == PlayerAction::Quit){
                states = GameState::Lost;
                break;
            }
            renderFrame(false);
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

    void updateNPCPositions() {
        // Update NPC positions based on their velocities
        for (Actor& actor : hardcoded_actors){
            if (actor.actor_name == "player") continue;
            glm::ivec2 nextPosition = actor.position + actor.velocity;
            if (!collisionDetected(nextPosition, actor.actor_name)){
                actor.position = nextPosition;
            } else {
                actor.velocity = -actor.velocity; // Reverse direction on collision
            }
        }
    }

    void updateGameState(PlayerAction action) { // update main
        // Update game state based on player action
        updateNPCPositions();
        if (action == PlayerAction::Quit){
            health = 0; // End game
        } else if (action == PlayerAction::Invalid){
            return;
        } else {
            updatePlayerPosition(action);
            std::vector<GameIncident> incidents;
            updateDialogues(incidents);
            updateGameIncidents(incidents);
            if (health <= 0){
                states = GameState::Lost;
            }
        }
    }

    bool collisionDetected(glm::ivec2 position, std::string actor_name) {
        //assert (true);// do check of index
        if (hardcoded_map[position.y][position.x]=='b'){
            return true;
        }
        for (const Actor& actor : hardcoded_actors){
            if (actor.actor_name == actor_name) continue;
            if (actor.blocking && actor.position == position){
                return true;
            }
        }
        return false;
    }

    void updatePlayerPosition(PlayerAction action) {
        // Update player position based on action
        glm::ivec2 nextPosition = mainActor.position;
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
                break;
        }
        //std::cout<<"collision"<<(collisionDetected(nextPosition) ? " detected\n" : " not detected\n");
        if (!collisionDetected(nextPosition, mainActor.actor_name)){
            mainActor.position = nextPosition;
        }
    }

    std::vector<GameIncident> updateDialogues(std::vector<GameIncident>& allIncidents) {
        // Update dialogues based on proximity to other actors
        for (Actor& actor : hardcoded_actors){
            if (actor.position.x == mainActor.position.x && actor.position.y == mainActor.position.y){
                if (actor.actor_name == "player") continue;
                if (actor.contact_dialogue.empty()) continue;
                checkGameIncidents(actor.contact_dialogue, allIncidents);
                dialogue_ss<<actor.contact_dialogue<<"\n";
            } else if (abs(actor.position.x - mainActor.position.x) <=1 && abs(actor.position.y - mainActor.position.y) <=1){
                if (actor.nearby_dialogue.empty()) continue;
                checkGameIncidents(actor.nearby_dialogue, allIncidents);
                dialogue_ss<<actor.nearby_dialogue<<"\n";
            }
        }
        return allIncidents;
    }

    void updateGameIncidents(std::vector<GameIncident>& incidents) {
        bool hasScoreUp = false;
        for (GameIncident& incident : incidents){
            switch (incident){
                case GameIncident::HealthDown:
                    health -= 1;
                    break;
                case GameIncident::ScoreUp:
                    if (!hasScoreUp) {
                        hasScoreUp = true;
                        score += 1;
                    }
                    break;
                case GameIncident::YouWin:
                    states = GameState::Won;
                    break;
                case GameIncident::GameOver:
                    states = GameState::Lost;
                    break;
                default:
                    break;
            }
        }
    }

    void checkGameIncidents(std::string dialogue, std::vector<GameIncident>& allIncidents){
        if (dialogue.find("[health down]") != std::string::npos){
            allIncidents.push_back(GameIncident::HealthDown);
        } else if (dialogue.find("[score up]") != std::string::npos){
            allIncidents.push_back(GameIncident::ScoreUp);
        } else if (dialogue.find("[you win]") != std::string::npos){
            allIncidents.push_back(GameIncident::YouWin);
        } else if (dialogue.find("[game over]") != std::string::npos){
            allIncidents.push_back(GameIncident::GameOver);
        } 
        return;
    }

    void renderDialogue(){
        render_ss<<dialogue_ss.str();
        dialogue_ss.str(std::string());
        dialogue_ss.clear();
    }

    void generalRender(){
        render_ss<<"health : "<<health<<", score : "<<score<<"\n";
        render_ss<<"Please make a decision...\n";
        render_ss<<input_query_message;
    }

    void renderFrame(bool isInitialRender = false) {// render main
        if (isInitialRender){
            std::cout<<initial_render<<"\n";
        } else {
            mapRender();
        }
        renderDialogue();
        generalRender();
        std::cout<<render_ss.str();
        render_ss.str(std::string());   
        render_ss.clear();
    }

    void mapRender(){
        assert(true);// do check of index
        for (int i=0; i<viewSize.y; i++){
            int row = mainActor.position.y - viewSize.y/2 + i;
            for (int j=0; j<viewSize.x; j++){
                int col = mainActor.position.x - viewSize.x/2 + j;
                if (row<0 || row>=mapSize.y || col<0 || col>=mapSize.x){
                    render_ss<<" "; // out of bounds
                    continue;
                }
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