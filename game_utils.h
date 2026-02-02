#ifndef GAME_UTILS_H
#define GAME_UTILS_H
#include <iostream>
#include "glm/glm.hpp"

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
    NextScene
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
	char view;
	glm::ivec2 position;
	glm::ivec2 velocity;
	bool blocking;
	std::string nearby_dialogue;
	std::string contact_dialogue;
    GameIncident nearby_incident;
    GameIncident contact_incident;
    bool triggered_scoreUp;
    std::string nearby_scene;
    std::string contact_scene;

	Actor(std::string actor_name, int id, char view, glm::ivec2 position, glm::ivec2 initial_velocity,
		bool blocking, std::string nearby_dialogue, std::string contact_dialogue)
		: actor_name(actor_name), id(id), view(view), position(position), velocity(initial_velocity),
         blocking(blocking), nearby_dialogue(nearby_dialogue), contact_dialogue(contact_dialogue),
         triggered_scoreUp(false){
            nearby_incident = checkGameIncidents(nearby_dialogue);
            contact_incident = checkGameIncidents(contact_dialogue);
            /*
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
            */
            std::string nb_scene = obtain_word_after_phrase(nearby_dialogue, "proceed to ");
            if (!nb_scene.empty()){
                nearby_incident = GameIncident::NextScene;
                nearby_scene = nb_scene;
            } else {
                nearby_scene = "";
            }
            std::string ct_scene = obtain_word_after_phrase(contact_dialogue, "proceed to ");
            if (!ct_scene.empty()){
                contact_incident = GameIncident::NextScene;
                contact_scene = ct_scene;
            } else {
                contact_scene = "";
            }

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

#endif // GAME_UTILS_H