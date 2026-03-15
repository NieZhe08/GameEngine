#ifndef TEMPLATE_DB_H
#define TEMPLATE_DB_H

#include <iostream>
#include <filesystem>
#include <string>

#include "engineUtils.h"
#include "rapidjson/document.h"

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

    // 返回模板中的 components 对象（如果存在且为 object），否则返回 nullptr
    const rapidjson::Value* getComponentsObject() const {
        if (templates.HasMember("components") && templates["components"].IsObject()) {
            return &templates["components"];
        }
        return nullptr;
    }
};

#endif // TEMPLATE_DB_H