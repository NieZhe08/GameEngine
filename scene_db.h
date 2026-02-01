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
                std::unordered_map<std::uint64_t, std::vector<int>>& mapHash) : sceneName(sceneName) {
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

    void processSceneActors(std::unordered_map<std::uint64_t, std::vector<int>>& mapHash) {
        sceneActors->clear();
        int max_x = 0; int max_y = 0;
        int id_counter = 0;
        if (scenes.HasMember("actors") && scenes["actors"].IsArray()){
            const rapidjson::Value& actors = scenes["actors"];
            // Reserve capacity to avoid reallocation while taking pointers to elements.
            sceneActors->reserve(actors.Size());
            for (rapidjson::SizeType i = 0; i < actors.Size(); i++){
                const rapidjson::Value& actor = actors[i];
                // default values
                std::string actor_name = "";
                std::string view_str = "?";
                std::string nearby_dialogue = "";
                std::string contact_dialogue = "";
                int x = 0; 
                int y = 0;
                int vel_x = 0;
                int vel_y = 0;
                bool blocking = false;

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

                if (actor.HasMember("name")) 
                    actor_name = actor["name"].GetString();
                if (actor.HasMember("view"))
                    view_str = actor["view"].GetString();
                if (actor.HasMember("x"))
                    x = actor["x"].GetInt();
                if (actor.HasMember("y"))
                    y = actor["y"].GetInt();
                if (actor.HasMember("vel_x"))
                    vel_x = actor["vel_x"].GetInt();
                if (actor.HasMember("vel_y"))
                    vel_y =  actor["vel_y"].GetInt();
                if (actor.HasMember("blocking"))
                    blocking = actor["blocking"].GetBool() ;
                if (actor.HasMember("nearby_dialogue"))
                    nearby_dialogue = actor["nearby_dialogue"].GetString();
                if (actor.HasMember("contact_dialogue"))
                    contact_dialogue = actor["contact_dialogue"].GetString();

                if (x>max_x) max_x = x;
                if (y>max_y) max_y = y;
                char view = (!view_str.empty()) ? view_str[0] : '?';

                sceneActors->emplace_back(actor_name, id_counter++, view, glm::ivec2(x,y), glm::ivec2(vel_x, vel_y),
                 blocking, nearby_dialogue, contact_dialogue);

                // store the index of the just-emplaced actor into mapHash
                int actor_index = static_cast<int>(sceneActors->size()) - 1;
                mapHash[hashPosition(glm::ivec2(x,y))].push_back(actor_index);

                if (actor_name == "player"){
                    mainActor = &sceneActors->back(); 
                    mainActorIndex = actor_index;
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