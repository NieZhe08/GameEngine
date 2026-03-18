#ifndef SCENE_DB_H
#define SCENE_DB_H
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

#include "engineUtils.h"
#include "rapidjson/document.h"
#include "game_utils.h"
#include "image_db.h"
#include "template_db.h"
#include "component_db.h"
#include "ActorManager.h"

class SceneDB {
public:
    rapidjson::Document scenes;
    std::string sceneName;
    std::shared_ptr<ActorManager> actorManager;
    glm::ivec2 mapSize;
    //ImageDB* imageDB;
    std::vector<TemplateDB> template_cache;
    lua_State* L;                // 共享的全局 Lua 状态
    std::shared_ptr<ComponentDB> componentDB;    // 组件类型数据库

    SceneDB (std::string sceneName,
             //ImageDB* _imageDB,
             lua_State* L,
                         const std::shared_ptr<ComponentDB>& componentDB,
                         const std::shared_ptr<ActorManager>& actorManager)
        : sceneName(sceneName),
          actorManager(actorManager),
          mapSize(0, 0),
          //imageDB(_imageDB),
          L(L),
          componentDB(componentDB) {
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
        processSceneActors();
    }

    void processSceneActors() {
        if (scenes.HasMember("actors")){
            const rapidjson::Value& actors = scenes["actors"];
            if (actors.IsArray()){
                // Reserve capacity to avoid reallocation while taking pointers to elements.
                // Reserve mapHash buckets to roughly avoid rehashing when many distinct positions exist.
                for (rapidjson::SizeType i = 0; i < actors.Size(); i++){
                    const rapidjson::Value& actor = actors[i];
                    // default values
                    std::string name;
                    if (actor.HasMember("name") && actor["name"].IsString()){
                        name = actor["name"].GetString();
                    } 
                    Actor* new_actor = actorManager->CreateActor(name, componentDB);

                    if (actor.HasMember("template")){// actor would still have templates
                        std::string templateName = actor["template"].GetString();
                        TemplateDB& tempDB = getOrCreateTemplate(templateName);
                        const rapidjson::Value* componentsFromTemplate = tempDB.getComponentsObject();
                        if (componentsFromTemplate != nullptr){
                            for (rapidjson::Value::ConstMemberIterator it = componentsFromTemplate->MemberBegin(); it != componentsFromTemplate->MemberEnd(); ++it){
                                const std::string component_name = it->name.GetString();
                                const rapidjson::Value& component_data = it->value;
                                readAndaddComponent(component_data, component_name, componentDB.get(), new_actor);
                            }
                        }

                    }
                    if (actor.HasMember("components") && actor["components"].IsObject()){
                        // iteratr through every key-value pair in components object, for each component, load the component instance and apply the properties from json
                        const rapidjson::Value& components = actor["components"];
                        for (rapidjson::Value::ConstMemberIterator it = components.MemberBegin(); it != components.MemberEnd(); ++it){
                            const std::string component_name = it->name.GetString();
                            const rapidjson::Value& component_data = it->value;
                            readAndaddComponent(component_data, component_name, componentDB.get(), new_actor);
                        }
                    }
                }
            }
        }
    }


private:
    TemplateDB& getOrCreateTemplate(const std::string& templateName) {
        // check if template already loaded
        for (auto& tempDB : template_cache) {
            if (tempDB.getName() == templateName) {
                return tempDB;
            }
        }
        // if not loaded, load and cache it
        template_cache.emplace_back(templateName);
        return template_cache.back();
    }

};

#endif
