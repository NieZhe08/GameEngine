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

    char getView(){
        if (templates.HasMember("view")){
            std::string view_str = templates["view"].GetString();
            return (!view_str.empty()) ? view_str[0] : '?';
        }
        return '?';
    }
    
    int get_x (){
        return (templates.HasMember("x"))? templates["x"].GetInt(): 0;
    }

    int get_y (){
        return (templates.HasMember("y"))? templates["y"].GetInt(): 0;
    }

    int get_vel_x (){
        return (templates.HasMember("vel_x"))? templates["vel_x"].GetInt(): 0;
    }

    int get_vel_y (){
        return (templates.HasMember("vel_y"))? templates["vel_y"].GetInt(): 0;
    }

    bool getBlocking(){
        if (templates.HasMember("blocking")){
            return templates["blocking"].GetBool();
        }
        return false;
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
};

#endif // TEMPLATE_DB_H