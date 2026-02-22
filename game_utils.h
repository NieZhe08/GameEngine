#ifndef GAME_UTILS_H
#define GAME_UTILS_H
#include <iostream>
#include "glm/glm.hpp"
#include <optional>
#include "SDL2/SDL.h"
#include "AudioManager.h"
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
    None, 
    NextScene
};

enum class GameState {
    Ongoing,
    Won,
    Lost,
    NextScene,
    IntroAnimation,
    Quit
};

enum class ContactType {
    Nearby,
    Overlap
};


#include <regex>
#include <optional>

inline std::optional<std::string> extractProceedTarget(const std::string& input) {
    std::regex pattern(R"(proceed to (\w+))", std::regex::icase);
    std::smatch matches;
    
    if (std::regex_search(input, matches, pattern)) {
        return matches[1].str();
    }
    
    return std::nullopt;
}

static std::string obtain_word_after_phrase(const std::string& input, const std::string& phrase){
    size_t pos = input.find(phrase);
    if (pos == std::string::npos) return "";
    pos += phrase.length();
    while (pos < input.size() && isspace(input[pos])) {
        pos++;
    }
    if (pos == input.size()) return "";
    size_t endPos = pos;
    while (endPos < input.size() && !isspace(input[endPos])) {
        endPos++;
    }
    return input.substr(pos, endPos - pos);
}

struct Actor
{
private: 
    GameIncident checkGameIncidents(std::string dialogue){
        if (dialogue.find("health down") != std::string::npos){
            return (GameIncident::HealthDown);
        } else if (dialogue.find("score up") != std::string::npos){
            return (GameIncident::ScoreUp);
        } else if (dialogue.find("you win") != std::string::npos){
            return (GameIncident::YouWin);
        } else if (dialogue.find("game over") != std::string::npos){
            return (GameIncident::GameOver);
        } 
        return (GameIncident::None);
    }
public:
	std::string actor_name;
    int id;
	glm::vec2 transform_position;
	glm::vec2 velocity;
	//bool blocking;

    // SDL stuff
    std::string view_image;
    bool has_view_image;
    std::string view_image_back;
    bool has_view_image_back;
    std::string view_image_damage;
    bool has_view_image_damage;
    int damage_view_duration_frames; // valid 1-30 (<31)
    std::string view_image_attack;
    bool has_view_image_attack;
    int attack_view_duration_frames; // valid 1-30 (<31)
    glm::vec2 transform_scale;
    bool flip_x ;
    bool flip_y ;
    float transform_rotation_degrees;
    glm::vec2 view_pivot_offset;
    bool triggered_scoreUp;
    int render_order;
	std::string nearby_dialogue;
	std::string contact_dialogue;
    GameIncident nearby_incident;
    GameIncident contact_incident;
    
    std::string nearby_scene;
    std::string contact_scene;

    bool  movement_bounce_enabled;

    glm::vec2 box_collider;
    glm::vec2 box_trigger;
    bool has_box_collider;
    bool has_box_trigger;

    bool view_dir_down = true; // down is default

    AudioInfo dialogue_info;
   

	Actor(std::string actor_name, int id, glm::vec2 _transform_position,
        glm::vec2 _velocity, 
        std::string _view_image, std::string _view_image_back, 
        std::string _view_image_damage, std::string _view_image_attack,
        glm::vec2 _transform_scale, 
        float _transform_rotation_degrees, glm::vec2 _view_pivot_offset, int _render_order,
        std::string _nearby_dialogue , std::string _contact_dialogue,
        bool _movement_bounce_enabled, 
        float bcw, float bch, 
        float btw, float bth,
        std::string dialogue_audio_path) : 

        actor_name(actor_name), id(id), transform_position(_transform_position),
        velocity(_velocity), 
        view_image(_view_image), has_view_image(!_view_image.empty()), 
        view_image_back(_view_image_back), has_view_image_back(!_view_image_back.empty()),
        view_image_damage(_view_image_damage),  has_view_image_damage(!_view_image_damage.empty()),damage_view_duration_frames(0),
        view_image_attack(_view_image_attack),  has_view_image_attack(!_view_image_attack.empty()),attack_view_duration_frames(0),
        transform_scale(glm::abs(_transform_scale)),
        flip_x(_transform_scale.x <0), flip_y(_transform_scale.y <0),
        transform_rotation_degrees(_transform_rotation_degrees), view_pivot_offset(_view_pivot_offset),
        triggered_scoreUp(false), render_order(_render_order),
        nearby_dialogue(_nearby_dialogue), contact_dialogue(_contact_dialogue),
        movement_bounce_enabled(_movement_bounce_enabled),
        box_collider(bcw, bch), box_trigger(btw, bth),
        has_box_collider(bcw > 0 && bch > 0), has_box_trigger(btw > 0 && bth > 0),
        dialogue_info(dialogue_audio_path, false)
        {
            nearby_incident = checkGameIncidents(nearby_dialogue);
            contact_incident = checkGameIncidents(contact_dialogue);
            
            auto nearby_scene_opt = extractProceedTarget(nearby_dialogue);
            if (nearby_scene_opt.has_value()){
                nearby_incident = GameIncident::NextScene;
                nearby_scene = nearby_scene_opt.value();
            } else {
                nearby_scene = "";
            }
            auto contact_scene_opt = extractProceedTarget(contact_dialogue);
            if (contact_scene_opt.has_value()){
                contact_incident = GameIncident::NextScene;
                contact_scene = contact_scene_opt.value();
            } else {
                contact_scene = "";
            }
            
           std::string nb_scene = obtain_word_after_phrase(nearby_dialogue, "proceed to ");
           if (!nb_scene.empty()){
               nearby_incident = GameIncident::NextScene;
               nearby_scene = nb_scene;
               //std::cout<<"nearby_scene: "<<nearby_scene<<"\n";
           } else {
               nearby_scene = "";
           }
           std::string ct_scene = obtain_word_after_phrase(contact_dialogue, "proceed to ");
           if (!ct_scene.empty()){
               contact_incident = GameIncident::NextScene;
               contact_scene = ct_scene;
                //std::cout<<"contact_scene: "<<contact_scene<<"\n";
           } else {
               contact_scene = "";
           }

	    }

    void setDamageViewDuration(){
        if (has_view_image_damage){
            damage_view_duration_frames = 30;
        }
    }

    void setAttackViewDuration(){
        if (has_view_image_attack){
            attack_view_duration_frames = 30;
        }
    }

    bool canSetDamageView(){
        return has_view_image_damage && damage_view_duration_frames == 0;
    }

    bool canSetAttackView(){
        return has_view_image_attack && attack_view_duration_frames == 0;
    }
};


inline std::uint64_t hashPosition(const glm::ivec2& position) {
    // A simple hash function combining x and y coordinates into a 64-bit key.
    // Ensure we shift a 64-bit value to avoid 32-bit shift overflow/UB.
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(position.x)) << 32) |
           static_cast<std::uint64_t>(static_cast<std::uint32_t>(position.y));
}

struct ActorSmallerId {
    bool operator() (const Actor* a1, const Actor* a2) const {
        return a1->id < a2->id; 
    }
};

//functor ActorRenderComparator
struct ActorRenderComparator {
    bool operator() (const Actor* a1, const Actor* a2) const {
        if (a1->render_order != a2->render_order) {
            return a1->render_order > a2->render_order; // Higher render_order is higher priority
        }
        else if (a1->transform_position.y != a2->transform_position.y) {
            return a1->transform_position.y > a2->transform_position.y; // Higher y is lower priority
        }
        return a1->id > a2->id; // For same y, smaller id is lower priority
    }
};

static inline void visualizeBox (SDL_Renderer* renderer, glm::vec2 center, glm::vec2 box, 
        glm::vec2 camera = glm::vec2(0,0), float zoom_factor = 1.0f,
        SDL_Color color = {255, 0, 0, 255}) {
    
    glm::vec2 screen_center = center * 100.0f * zoom_factor + camera;
    glm::vec2 screen_box = box * zoom_factor * 100.0f;

    SDL_FRect rect = {
        screen_center.x - screen_box.x / 2.0f,
        screen_center.y - screen_box.y / 2.0f,
        screen_box.x,
        screen_box.y
    };
    //std::cout << "visualizeBox: center=(" << screen_center.x << "," << screen_center.y 
    //          << ") box=(" << screen_box.x << "," << screen_box.y 
    //          << ") rect=(" << rect.x << "," << rect.y << "," << rect.w << "," << rect.h << ")\n";
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a); // Red for box
    SDL_RenderDrawRectF(renderer, &rect); // Draw outline
}

inline bool checkAABB(glm::vec2 ctr1, glm::vec2 box1, glm::vec2 ctr2, glm::vec2 box2,
    SDL_Renderer* renderer = nullptr, glm::vec2 camera = glm::vec2(0,0), float zoom_factor = 1.0f, bool visualize = false) {
    // box1 and box2 are width and height, not half-width and half-height
    // Perform collision detection in world space (no transform) to avoid precision loss
    

    // Collision detection logic in world space (no scale/camera offset)
    if (ctr1.x + box1.x/2 > ctr2.x - box2.x/2 && ctr1.x - box1.x/2 < ctr2.x + box2.x/2){
        if (ctr1.y + box1.y/2 > ctr2.y - box2.y/2 && ctr1.y - box1.y/2 < ctr2.y + box2.y/2){
            return true;
        }
    }
    return false;
}





#endif 