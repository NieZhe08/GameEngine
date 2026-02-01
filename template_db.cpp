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
        EngineUtils::ReadJsonFile("resources/templates/" + templateName + ".template", templates);
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

    glm::ivec2 getPosition(){
        if (templates.HasMember("position")){
            int x = (templates["position"].HasMember("x"))? templates["position"]["x"].GetInt(): 0;
            int y = (templates["position"].HasMember("y"))? templates["position"]["y"].GetInt(): 0;
            return glm::ivec2(x, y);
        }
        return glm::ivec2(0, 0);
    }

    glm::ivec2 getVelocity(){
        if (templates.HasMember("velocity")){
            int vel_x = (templates["velocity"].HasMember("x"))? templates["velocity"]["x"].GetInt(): 0;
            int vel_y = (templates["velocity"].HasMember("y"))? templates["velocity"]["y"].GetInt(): 0;
            return glm::ivec2(vel_x, vel_y);
        }
        return glm::ivec2(0, 0);
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