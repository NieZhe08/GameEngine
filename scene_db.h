#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "game_utils.h"
#include "template_db.h"

class SceneDB {
public:
    rapidjson::Document scenes;
    std::string sceneName;
    Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> sceneActors;
    glm::ivec2 mapSize;
    int mainActorIndex;

        SceneDB (std::string sceneName, 
            std::unordered_map<std::uint64_t, std::vector<Actor*>>& mapHash) : sceneName(sceneName) {
        if (!std::filesystem::exists("resources/scenes/" + sceneName + ".scene")){
            std::cout<<"error: scene "<<sceneName<<" is missing";
            exit(0);
        }
        EngineUtils::ReadJsonFile("resources/scenes/" + sceneName + ".scene", scenes);
        /*
        if (!scenes.IsObject()){
            exit(0);
        }
        */
        
        mainActor = nullptr;
        mainActorIndex = -1;
        sceneActors = std::make_unique<std::vector<Actor>>();
        processSceneActors(mapHash);
    }

    void processSceneActors(std::unordered_map<std::uint64_t, std::vector<Actor*>>& mapHash) {
        sceneActors->clear();
        int max_x = 0; int max_y = 0;
        int id_counter = 0;
        if (scenes.HasMember("actors") && scenes["actors"].IsArray()){
            const rapidjson::Value& actors = scenes["actors"];
            for (rapidjson::SizeType i = 0; i < actors.Size(); i++){
                const rapidjson::Value& actor = actors[i];
                std::string actor_name, view_str, nearby_dialogue, contact_dialogue;
                int x, y, vel_x, vel_y;;
                bool blocking;

                if (actor.HasMember("template")){
                    std::string templateName = actor["template"].GetString();
                    TemplateDB actorTemplate(templateName);

                    actor_name = actorTemplate.getName();
                    view_str = std::string(1, actorTemplate.getView());
                    glm::ivec2 position = actorTemplate.getPosition();
                    x = position.x;
                    y = position.y;
                    glm::ivec2 velocity = actorTemplate.getVelocity();
                    vel_x = velocity.x;
                    vel_y = velocity.y;
                    blocking = actorTemplate.getBlocking();
                    nearby_dialogue = actorTemplate.getNearbyDialogue();
                    contact_dialogue = actorTemplate.getContactDialogue();
                }

                actor_name = (actor.HasMember("name"))? actor["name"].GetString() : "";
                view_str = (actor.HasMember("view"))? actor["view"].GetString() : "?";
                char view = (!view_str.empty()) ? view_str[0] : '?';
                x = (actor.HasMember("x"))? actor["x"].GetInt() : 0;
                y = (actor.HasMember("y"))? actor["y"].GetInt() : 0;
                vel_x = (actor.HasMember("vel_x"))? actor["vel_x"].GetInt() : 0;
                vel_y = (actor.HasMember("vel_y"))? actor["vel_y"].GetInt() : 0;
                blocking = (actor.HasMember("blocking"))? actor["blocking"].GetBool() : false;
                nearby_dialogue = (actor.HasMember("nearby_dialogue"))? actor["nearby_dialogue"].GetString() : "";
                contact_dialogue = (actor.HasMember("contact_dialogue"))? actor["contact_dialogue"].GetString() : "";

                if (x>max_x) max_x = x;
                if (y>max_y) max_y = y;

                sceneActors->emplace_back(actor_name, id_counter++, view, glm::ivec2(x,y), glm::ivec2(vel_x, vel_y),
                 blocking, nearby_dialogue, contact_dialogue);

                mapHash[hashPosition(glm::ivec2(x,y))].push_back(&sceneActors->back());

                if (actor_name == "player"){
                    mainActor = &sceneActors->back(); 
                    mainActorIndex = static_cast<int>(sceneActors->size()) - 1;
                }
            }
        }
        mapSize = glm::ivec2(max_x+1, max_y+1);
    }

    std::unique_ptr<std::vector<Actor>> getSceneActors() {
        return std::move(sceneActors);
    }

    // Return index of main actor inside the moved vector (or -1 if none)
    int getMainActorIndex() {
        return mainActorIndex;
    }

    glm::ivec2 getMapSize() {
        return mapSize;
    }

    //getMapHash() : mapHash is passed by reference in constructor
};