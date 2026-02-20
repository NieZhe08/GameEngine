#ifndef TEMPLATE_DB_H
#define TEMPLATE_DB_H

#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "game_utils.h"

class TemplateDB {
public:
    rapidjson::Document templates;  
    std::string templateName;

        TemplateDB (std::string templateName) : templateName(templateName) {
        if (!std::filesystem::exists("resources/actor_templates/" + templateName + ".template")){
            std::cout<<"error: template "<<templateName<<" is missing";
            exit(0);
        }
        EngineUtils::ReadJsonFile("resources/actor_templates/" + templateName + ".template", templates);
    }

    std::string getName() {
        if (templates.HasMember("name"))
            return templates["name"].GetString();
        return "";
    }

    std::string getViewImage() {
        if (templates.HasMember("view_image"))
            return templates["view_image"].GetString();
        return "";
    }

    std::string getViewImageBack() {
        if (templates.HasMember("view_image_back"))
            return templates["view_image_back"].GetString();
        return "";
    }

    float getTransformPositionX() {
        return (templates.HasMember("transform_position_x"))
            ? templates["transform_position_x"].GetFloat()
            : 0.0f;
    }

    float getTransformPositionY() {
        return (templates.HasMember("transform_position_y"))
            ? templates["transform_position_y"].GetFloat()
            : 0.0f;
    }

    float getVelX() {
        return (templates.HasMember("vel_x")) ? templates["vel_x"].GetFloat() : 0.0f;
    }

    float getVelY() {
        return (templates.HasMember("vel_y")) ? templates["vel_y"].GetFloat() : 0.0f;
    }

    float getTransformScaleX() {
        return (templates.HasMember("transform_scale_x"))
            ? templates["transform_scale_x"].GetFloat()
            : 1.0f;
    }

    float getTransformScaleY() {
        return (templates.HasMember("transform_scale_y"))
            ? templates["transform_scale_y"].GetFloat()
            : 1.0f;
    }

    float getTransformRotationDegrees() {
        return (templates.HasMember("transform_rotation_degrees"))
            ? templates["transform_rotation_degrees"].GetFloat()
            : 0.0f;
    }

    float getViewPivotOffsetX() {
        return (templates.HasMember("view_pivot_offset_x"))
            ? templates["view_pivot_offset_x"].GetFloat()
            : 0.0f;
    }

    float getViewPivotOffsetY() {
        return (templates.HasMember("view_pivot_offset_y"))
            ? templates["view_pivot_offset_y"].GetFloat()
            : 0.0f;
    }

    int getRenderOrder() {
        return (templates.HasMember("render_order"))
            ? templates["render_order"].GetInt()
            : 0;
    }

    std::string getNearbyDialogue() {
        if (templates.HasMember("nearby_dialogue"))
            return templates["nearby_dialogue"].GetString();
        return "";
    }

    std::string getContactDialogue() {
        if (templates.HasMember("contact_dialogue"))
            return templates["contact_dialogue"].GetString();
        return "";
    }

    bool getMovementBounceEnabled() {
        if (templates.HasMember("movement_bounce_enabled"))
            return templates["movement_bounce_enabled"].GetBool();
        return false;
    }

    float getBoxColliderWidth() {
        return (templates.HasMember("box_collider_width"))
            ? templates["box_collider_width"].GetFloat()
            : 0.0f;
    }

    float getBoxColliderHeight() {
        return (templates.HasMember("box_collider_height"))
            ? templates["box_collider_height"].GetFloat()
            : 0.0f;
    }

    float getBoxTriggerWidth() {
        return (templates.HasMember("box_trigger_width"))
            ? templates["box_trigger_width"].GetFloat()
            : 0.0f;
    }

    float getBoxTriggerHeight() {
        return (templates.HasMember("box_trigger_height"))
            ? templates["box_trigger_height"].GetFloat()
            : 0.0f;
    }

    std::string getViewImageDamage() {
        if (templates.HasMember("view_image_damage"))
            return templates["view_image_damage"].GetString();
        return "";
    }

    std::string getViewImageAttack() {
        if (templates.HasMember("view_image_attack"))
            return templates["view_image_attack"].GetString();
        return "";
    }
};

#endif // TEMPLATE_DB_H