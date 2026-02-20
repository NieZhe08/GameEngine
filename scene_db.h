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
#include "template_db.h"

class SceneDB {
public:
    rapidjson::Document scenes;
    std::string sceneName;
    //Actor* mainActor;
    std::unique_ptr<std::vector<Actor>> sceneActors;
    glm::ivec2 mapSize;
    int mainActorIndex;
    ImageDB* imageDB;
    std::vector<TemplateDB> template_cache;

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
                    float x = 0.0f; 
                    float y = 0.0f;
                    float vel_x = 0.0f;
                    float vel_y = 0.0f;
                    std::string view_str = "";
                    std::string view_back_str = "";
                    std::string view_image_damage_str = "";
                    std::string view_image_attack_str = "";
                    float transform_scale_x = 1.0f;
                    float transform_scale_y = 1.0f;
                    float transform_rotation_degrees = 0.0f;
                    float view_pivot_offset_x = 0.0f;
                    float view_pivot_offset_y = 0.0f;
                    int render_order = 0;
                    std::string nearby_dialogue = "";
                    std::string contact_dialogue = "";
                    bool  movement_bounce_enabled = false;
                    float box_collider_width = 0.0f;
                    float box_collider_height = 0.0f;
                    float box_trigger_width = 0.0f;
                    float box_trigger_height = 0.0f;
                    //int vel_x = 0;
                    //int vel_y = 0;
                    //bool blocking = false;

                    if (actor.HasMember("template")){
                        std::string templateName = actor["template"].GetString();
                        TemplateDB& actorTemplate = getOrCreateTemplate(templateName);
                        actor_name = actorTemplate.getName();
                        view_str = actorTemplate.getViewImage();
                        view_back_str = actorTemplate.getViewImageBack();
                        x = actorTemplate.getTransformPositionX();
                        y = actorTemplate.getTransformPositionY();
                        vel_x = actorTemplate.getVelX();
                        vel_y = actorTemplate.getVelY();
                        transform_scale_x = actorTemplate.getTransformScaleX();
                        transform_scale_y = actorTemplate.getTransformScaleY();
                        transform_rotation_degrees = actorTemplate.getTransformRotationDegrees();
                        view_pivot_offset_x = actorTemplate.getViewPivotOffsetX();
                        view_pivot_offset_y = actorTemplate.getViewPivotOffsetY();
                        render_order = actorTemplate.getRenderOrder();
                        nearby_dialogue = actorTemplate.getNearbyDialogue();
                        contact_dialogue = actorTemplate.getContactDialogue();
                        movement_bounce_enabled = actorTemplate.getMovementBounceEnabled();
                        box_collider_width = actorTemplate.getBoxColliderWidth();
                        box_collider_height = actorTemplate.getBoxColliderHeight();
                        box_trigger_width = actorTemplate.getBoxTriggerWidth();
                        box_trigger_height = actorTemplate.getBoxTriggerHeight();

                        if (!view_str.empty()){
                            SDL_Texture* tex = imageDB->loadImage(view_str);
                            float tex_width = 0.0f, tex_height = 0.0f;
                            Helper::SDL_QueryTexture(tex,  &tex_width, &tex_height);
                            if (view_pivot_offset_x == 0.0f && view_pivot_offset_y == 0.0f){
                                view_pivot_offset_x = (tex_width) / 2.0f;
                                view_pivot_offset_y = (tex_height) / 2.0f;
                            }
                        }
                        if (!view_back_str.empty()){
                            imageDB->loadImage(view_back_str);
                        }
                    }
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
                    if (actor.HasMember("view_image_back")){
                        view_back_str = actor["view_image_back"].GetString();
                        if (view_back_str.length() > 0){
                        imageDB->loadImage(view_back_str);
                        }
                    }
                    if (actor.HasMember("transform_position_x"))
                        x = actor["transform_position_x"].GetFloat();
                    if (actor.HasMember("transform_position_y"))
                        y = actor["transform_position_y"].GetFloat();
                    if (actor.HasMember("vel_x"))
                        vel_x = actor["vel_x"].GetFloat();
                    if (actor.HasMember("vel_y"))
                        vel_y = actor["vel_y"].GetFloat();
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
                    if (actor.HasMember("render_order")){
                        render_order = actor["render_order"].GetInt();
                    }
                    if (actor.HasMember("nearby_dialogue"))
                        nearby_dialogue = actor["nearby_dialogue"].GetString();
                    if (actor.HasMember("contact_dialogue"))
                        contact_dialogue = actor["contact_dialogue"].GetString();
                    if (actor.HasMember("movement_bounce_enabled"))
                        movement_bounce_enabled = actor["movement_bounce_enabled"].GetBool();

                    if (actor.HasMember("box_collider_width"))
                        box_collider_width = actor["box_collider_width"].GetFloat();
                    if (actor.HasMember("box_collider_height"))
                        box_collider_height = actor["box_collider_height"].GetFloat();
                    if (actor.HasMember("box_trigger_width"))
                        box_trigger_width = actor["box_trigger_width"].GetFloat();
                    if (actor.HasMember("box_trigger_height"))
                        box_trigger_height = actor["box_trigger_height"].GetFloat();
                    if (actor.HasMember("view_image_damage")){
                        view_image_damage_str = actor["view_image_damage"].GetString();
                        imageDB->loadImage(view_image_damage_str);
                    }
                    if (actor.HasMember("view_image_attack")){
                        view_image_attack_str = actor["view_image_attack"].GetString();
                        imageDB->loadImage(view_image_attack_str);
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

                    sceneActors->emplace_back(actor_name, id_counter++, glm::vec2(x,y), 
                        glm::vec2(vel_x, vel_y), 
                        view_str, view_back_str,
                        view_image_damage_str, view_image_attack_str,
                        glm::vec2(transform_scale_x, transform_scale_y), transform_rotation_degrees, 
                        glm::vec2(view_pivot_offset_x, view_pivot_offset_y), render_order,
                        nearby_dialogue, contact_dialogue,
                        movement_bounce_enabled,
                        box_collider_width, box_collider_height,
                        box_trigger_width, box_trigger_height);

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

private:
    TemplateDB& getOrCreateTemplate(const std::string& templateName) {
        for (TemplateDB& tmpl : template_cache) {
            if (tmpl.templateName == templateName) {
                return tmpl;
            }
        }
        template_cache.emplace_back(templateName);
        return template_cache.back();
    }

    //getMapHash() : mapHash is passed by reference in constructor
};

#endif // SCENE_DB_H