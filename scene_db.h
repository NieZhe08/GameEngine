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

    std::string stepSfx = "";
    std::string damageSfx = "";

   
    glm::vec2 largestColliderSize = glm::vec2(0.0f, 0.0f);
     glm::vec2 largestTriggerSize = glm::vec2(0.0f, 0.0f);

    bool hasCollision = false;
    bool hasNearbyDialogue = false;

    void updateLargestColliderSize(float x, float y){
        if (x > largestColliderSize.x) largestColliderSize.x = x;
        if (y > largestColliderSize.y) largestColliderSize.y = y;
    }

    void updateLargestTriggerSize(float x, float y){
        if (x > largestTriggerSize.x) largestTriggerSize.x = x;
        if (y > largestTriggerSize.y) largestTriggerSize.y = y;
    }

    SceneDB (std::string sceneName, 
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
        processSceneActors();
    }

    void processSceneActors() {
        sceneActors->clear();
        int max_x = 0; int max_y = 0;
        int id_counter = 0;
        if (scenes.HasMember("actors")){
            const rapidjson::Value& actors = scenes["actors"];
            if (actors.IsArray()){
                // Reserve capacity to avoid reallocation while taking pointers to elements.
                sceneActors->reserve(actors.Size());
                // Reserve mapHash buckets to roughly avoid rehashing when many distinct positions exist.
                for (rapidjson::SizeType i = 0; i < actors.Size(); i++){
                    const rapidjson::Value& actor = actors[i];
                    // default values
                    std::string actor_name = "";
                    //std::string nearby_dialogue = "";
                    //std::string contact_dialogue = "";
                    glm::vec2 transform_position = glm::vec2(0.0f, 0.0f);
                    glm::vec2 vel = glm::vec2(0.0f, 0.0f);
                    std::string view_str = "";
                    std::string view_back_str = "";
                    std::string view_image_damage_str = "";
                    std::string view_image_attack_str = "";
                    glm::vec2 transform_scale = glm::vec2(1.0f, 1.0f);
                    float transform_rotation_degrees = 0.0f;
                    glm::vec2 view_pivot_offset = glm::vec2(0.0f, 0.0f);
                    int render_order = 0;
                    std::string nearby_dialogue = "";
                    std::string contact_dialogue = "";
                    bool  movement_bounce_enabled = false;
                    float box_collider_width = 0.0f;
                    float box_collider_height = 0.0f;
                    float box_trigger_width = 0.0f;
                    float box_trigger_height = 0.0f;
                    std::string nearby_dialogue_sfx = "";
                    glm::vec2 tex_size = glm::vec2(0.0f, 0.0f);
                    //int vel_x = 0;
                    //int vel_y = 0;
                    //bool blocking = false;

                    if (actor.HasMember("template")){
                        std::string templateName = actor["template"].GetString();
                        TemplateDB& actorTemplate = getOrCreateTemplate(templateName);
                        transform_scale.x = actorTemplate.getTransformScaleX();
                        transform_scale.y = actorTemplate.getTransformScaleY();
                        actor_name = actorTemplate.getName();
                        view_str = actorTemplate.getViewImage();
                        view_pivot_offset = setDefaultPivotForValidTexture(view_str);
                        tex_size =  view_pivot_offset *glm::vec2(2,2); // default collider size based on texture size, can be overridden by template or scene
                        view_back_str = actorTemplate.getViewImageBack();
                        transform_position.x = actorTemplate.getTransformPositionX();
                        transform_position.y = actorTemplate.getTransformPositionY();
                        vel.x = actorTemplate.getVelX();
                        vel.y = actorTemplate.getVelY();
                        transform_rotation_degrees = actorTemplate.getTransformRotationDegrees();
                        view_pivot_offset.x = (actorTemplate.getViewPivotOffsetX()==0.0f) ? view_pivot_offset.x : actorTemplate.getViewPivotOffsetX();
                        view_pivot_offset.y = (actorTemplate.getViewPivotOffsetY()==0.0f) ? view_pivot_offset.y : actorTemplate.getViewPivotOffsetY();
                        render_order = actorTemplate.getRenderOrder();
                        nearby_dialogue = actorTemplate.getNearbyDialogue();
                        contact_dialogue = actorTemplate.getContactDialogue();
                        movement_bounce_enabled = actorTemplate.getMovementBounceEnabled();
                        box_collider_width = actorTemplate.getBoxColliderWidth();
                        box_collider_height = actorTemplate.getBoxColliderHeight();
                        box_trigger_width = actorTemplate.getBoxTriggerWidth();
                        box_trigger_height = actorTemplate.getBoxTriggerHeight();
                        nearby_dialogue_sfx = actorTemplate.getNearbyDialogueSFX();
                        
                        if (!view_back_str.empty()){
                            imageDB->loadImage(view_back_str);
                        }
                    }
                    if (actor.HasMember("name")) 
                        actor_name = actor["name"].GetString();
                    if (actor.HasMember("transform_position_x"))
                        transform_position.x = actor["transform_position_x"].GetFloat();
                    if (actor.HasMember("transform_position_y"))
                        transform_position.y = actor["transform_position_y"].GetFloat();
                    if (actor.HasMember("transform_scale_x"))
                        transform_scale.x = actor["transform_scale_x"].GetFloat();
                    if (actor.HasMember("transform_scale_y"))
                        transform_scale.y = actor["transform_scale_y"].GetFloat();
                    if (actor.HasMember("view_image")){
                        view_str = actor["view_image"].GetString();
                        if (view_str.length() > 0){
                            SDL_Texture* tex = imageDB->loadImage(view_str);
                            float tex_width = 0.0f, tex_height = 0.0f;
                            Helper::SDL_QueryTexture(tex,  &tex_width, &tex_height);
                            view_pivot_offset.x = (tex_width) / 2.0f; // default pivot at center of the image
                            view_pivot_offset.y = (tex_height) / 2.0f;
                            //updateLargestTextureSize(tex_width * transform_scale.x, tex_height * transform_scale.y);
                            tex_size = glm::vec2(tex_width, tex_height);
                        }
                    }
                    if (actor.HasMember("view_image_back")){
                        view_back_str = actor["view_image_back"].GetString();
                        if (view_back_str.length() > 0){
                            imageDB->loadImage(view_back_str);
                        }
                    }
                    
                    if (actor.HasMember("vel_x"))
                        vel.x = actor["vel_x"].GetFloat();
                    if (actor.HasMember("vel_y"))
                        vel.y = actor["vel_y"].GetFloat();
                    
                    if (actor.HasMember("transform_rotation_degrees"))
                        transform_rotation_degrees = actor["transform_rotation_degrees"].GetFloat();
                    if (actor.HasMember("view_pivot_offset_x")){
                        view_pivot_offset.x = actor["view_pivot_offset_x"].GetFloat();
                    }
                    if (actor.HasMember("view_pivot_offset_y")){
                        view_pivot_offset.y = actor["view_pivot_offset_y"].GetFloat();
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
                    if (actor.HasMember("nearby_dialogue_sfx")){
                        nearby_dialogue_sfx = actor["nearby_dialogue_sfx"].GetString();
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

                    if (transform_position.x > max_x) max_x = transform_position.x;
                    if (transform_position.y > max_y) max_y = transform_position.y;
                    updateLargestColliderSize(box_collider_width, box_collider_height);
                    updateLargestTriggerSize(box_trigger_width, box_trigger_height);

                    if (box_collider_width > 0.0f && box_collider_height > 0.0f){
                        hasCollision = true;
                    }
                    if (!nearby_dialogue.empty() && box_trigger_height > 0.0f && box_trigger_width > 0.0f){
                        hasNearbyDialogue = true;
                    }
                    //char view = (!view_str.empty()) ? view_str[0] : '?';

                    sceneActors->emplace_back(actor_name, id_counter++, transform_position, 
                        vel, 
                        view_str, view_back_str,
                        view_image_damage_str, view_image_attack_str,
                        transform_scale, transform_rotation_degrees, 
                        view_pivot_offset, render_order,
                        nearby_dialogue, contact_dialogue,
                        movement_bounce_enabled,
                        box_collider_width, box_collider_height,
                        box_trigger_width, box_trigger_height,
                        nearby_dialogue_sfx,
                        tex_size
                        );

                    // store the index of the just-emplaced actor into mapHash
                    int actor_index = static_cast<int>(sceneActors->size()) - 1;

                    if (actor_name == "player"){
                        //mainActor = &sceneActors->back(); 
                        mainActorIndex = actor_index;
                        if (actor.HasMember("step_sfx")){
                            stepSfx = actor["step_sfx"].GetString();
                        }
                        if (actor.HasMember("damage_sfx")){
                            damageSfx = actor["damage_sfx"].GetString();
                        }
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

    std::string getStepSFX() {
        return stepSfx;
    }

    std::string getDamageSFX() {
        return damageSfx;
    }

    glm::vec2 getLargestColliderSize() {
        return largestColliderSize;
    }

    glm::vec2 getLargestTriggerSize() {
        return largestTriggerSize;
    }

    bool hasAnyCollision() {
        return hasCollision;
    }

    bool hasAnyNearbyDialogue() {
        return hasNearbyDialogue;
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

    glm::vec2 setDefaultPivotForValidTexture(const std::string& view_str, float transform_scale_x = 1.0f, float transform_scale_y = 1.0f) {
        glm::vec2 pivot_offset(0.0f, 0.0f);
        if (view_str.length() > 0){
            SDL_Texture* tex = imageDB->loadImage(view_str);
            float tex_width = 0.0f, tex_height = 0.0f;
            Helper::SDL_QueryTexture(tex,  &tex_width, &tex_height);
            pivot_offset.x = (tex_width) / 2.0f; // default pivot at center of the image
            pivot_offset.y = (tex_height) / 2.0f;
        }
        return pivot_offset;
    }

    //getMapHash() : mapHash is passed by reference in constructor
};

#endif // SCENE_DB_H