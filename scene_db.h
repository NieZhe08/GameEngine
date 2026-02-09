#ifndef SCENE_DB_H
#define SCENE_DB_H
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "game_utils.h"
#include "image_db.h"
//#include "template_db.h"

class SceneDB {
public:
    rapidjson::Document scenes;
    std::string sceneName;
    //Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> sceneActors;
    glm::ivec2 mapSize;
    int mainActorIndex;
    ImageDB* imageDB;

    SceneDB (std::string sceneName, 
                std::unordered_map<std::uint64_t, std::vector<int>>& mapHash,
            ImageDB* _imageDB) : sceneName(sceneName), imageDB(_imageDB) {
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
        
        mainActorIndex = -1;
        sceneActors = std::make_unique<std::vector<Actor>>();
        processSceneActors(mapHash);
    }

    void processSceneActors(std::unordered_map<std::uint64_t, std::vector<int>>& mapHash) {
        sceneActors->clear();
        int max_x = 0; int max_y = 0;
        int id_counter = 0;
        if (scenes.HasMember("actors")){
            const rapidjson::Value& actors = scenes["actors"];
            if (actors.IsArray()){
                // Reserve capacity to avoid reallocation while taking pointers to elements.
                sceneActors->reserve(actors.Size());
                // Reserve mapHash buckets to roughly avoid rehashing when many distinct positions exist.
                mapHash.reserve(static_cast<size_t>(actors.Size()));
                for (rapidjson::SizeType i = 0; i < actors.Size(); i++){
                    const rapidjson::Value& actor = actors[i];
                // default values
                std::string actor_name = "";
                //std::string nearby_dialogue = "";
                //std::string contact_dialogue = "";
                int x = 0; 
                int y = 0;
                std::string view_str = "";
                float transform_scale_x = 1.0f;
                float transform_scale_y = 1.0f;
                float transform_rotation_degrees = 0.0f;
                float view_pivot_offset_x = 0.0f;
                float view_pivot_offset_y = 0.0f;
                //int vel_x = 0;
                //int vel_y = 0;
                //bool blocking = false;

                //if (actor.HasMember("template")){
                //    std::string templateName = actor["template"].GetString();
                //    TemplateDB actorTemplate(templateName);
                //    actor_name = actorTemplate.getName();
                //    view_str = std::string(1, actorTemplate.getView());
                //    x = actorTemplate.get_x();
                //    y = actorTemplate.get_y();
                //    vel_x = actorTemplate.get_vel_x();
                //    vel_y = actorTemplate.get_vel_y();
                //    blocking = actorTemplate.getBlocking();
                //    nearby_dialogue = actorTemplate.getNearbyDialogue();
                //    contact_dialogue = actorTemplate.getContactDialogue();
                //}
                if (actor.HasMember("name")) 
                    actor_name = actor["name"].GetString();
                if (actor.HasMember("view_image")){
                    view_str = actor["view_image"].GetString();
                    if (view_str.length() > 0){
                        SDL_Texture* tex = imageDB->loadImage(view_str);
                        float tex_width = 0.0f, tex_height = 0.0f;
                        Helper::SDL_QueryTexture(tex,  &tex_width, &tex_height);
                        view_pivot_offset_x = (tex_width) / 2.0f; // default pivot at center of the image
                        view_pivot_offset_y = (tex_height) / 2.0f;
                    }
                }
                if (actor.HasMember("transform_position_x"))
                    x = actor["transform_position_x"].GetInt();
                if (actor.HasMember("transform_position_y"))
                    y = actor["transform_position_y"].GetInt();
                if (actor.HasMember("transform_scale_x"))
                    transform_scale_x = actor["transform_scale_x"].GetFloat();
                if (actor.HasMember("transform_scale_y"))
                    transform_scale_y = actor["transform_scale_y"].GetFloat();
                if (actor.HasMember("transform_rotation_degrees"))
                    transform_rotation_degrees = actor["transform_rotation_degrees"].GetFloat();
                if (actor.HasMember("view_pivot_offset_x")){
                    view_pivot_offset_x = actor["view_pivot_offset_x"].GetFloat();
                }
                if (actor.HasMember("view_pivot_offset_y")){
                    view_pivot_offset_y = actor["view_pivot_offset_y"].GetFloat();
                }
                    
                //if (actor.HasMember("vel_x"))
                //    vel_x = actor["vel_x"].GetInt();
                //if (actor.HasMember("vel_y"))
                //    vel_y =  actor["vel_y"].GetInt();
                //if (actor.HasMember("blocking"))
                //    blocking = actor["blocking"].GetBool() ;
                //if (actor.HasMember("nearby_dialogue"))
                //    nearby_dialogue = actor["nearby_dialogue"].GetString();
                //if (actor.HasMember("contact_dialogue"))
                //    contact_dialogue = actor["contact_dialogue"].GetString();

                if (x>max_x) max_x = x;
                if (y>max_y) max_y = y;
                //char view = (!view_str.empty()) ? view_str[0] : '?';

                sceneActors->emplace_back(actor_name, id_counter++, glm::ivec2(x,y), view_str,
                    glm::vec2(transform_scale_x, transform_scale_y), transform_rotation_degrees, 
                    glm::vec2(view_pivot_offset_x, view_pivot_offset_y));

                // store the index of the just-emplaced actor into mapHash
                int actor_index = static_cast<int>(sceneActors->size()) - 1;
                mapHash[hashPosition(glm::vec2(x,y))].push_back(actor_index);

                if (actor_name == "player"){
                    //mainActor = &sceneActors->back(); 
                    mainActorIndex = actor_index;
                }
            }
        }
        mapSize = glm::ivec2(max_x, max_y+1); // TODO
    }}

    std::unique_ptr<std::vector<Actor>> getSceneActors() {
        return std::move(sceneActors);
    }

    // Return index of main actor inside the moved vector (or -1 if none)
    int getMainActorIndex() {
        return mainActorIndex;
    }

    bool hasMainActor() {
        return mainActorIndex >=0;
    }

    glm::ivec2 getMapSize() {
        return mapSize;
    }

    //getMapHash() : mapHash is passed by reference in constructor
};

#endif // SCENE_DB_H