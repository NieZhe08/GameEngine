#ifndef GAME_UTILS_H
#define GAME_UTILS_H
#include <iostream>
#include "glm/glm.hpp"
#include <optional>
#include "SDL2/SDL.h"
#include "AudioManager.h" 
#include <string>
#include <algorithm>
#include <set>
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <map>
#include <vector>
#include <set>
#include <cstdint>
#include <memory>
#include "rapidjson/document.h"


class ActorManager;
class ComponentDB;

class Actor : public std::enable_shared_from_this<Actor> {
public:
    // 基础属性 
    std::string name;
    int id;
    lua_State* L;
    ActorManager* actorManager = nullptr;
    std::shared_ptr<ComponentDB> componentDB;
    bool pending_destroy = false;
    bool destroyOnSceneChange = true;
    
    // component system: key -> component instance (LuaRef)
    std::map<std::string, luabridge::LuaRef> components;
    // track components that have had OnStart called
    std::set<std::string> started_components;

    Actor(lua_State* L, int id, std::string name, ActorManager* am, const std::shared_ptr<ComponentDB>& cdb);

    ~Actor();

    void MarkPendingDestroy();

    // --- Lua API ---

    std::string GetName() const;
    int GetID() const;

    luabridge::LuaRef GetComponentByKey(std::string key);

    luabridge::LuaRef GetComponent(std::string type);

    luabridge::LuaRef GetComponents(std::string type);

    

    luabridge::LuaRef AddComponent(std::string type);

    void RemoveComponent(luabridge::LuaRef component);

    // --- 引擎内部生命周期驱动 ---

    // 处理 OnStart 
    void ProcessOnStart();

    // 处理 OnUpdate 
    void ProcessOnUpdate();

    // 处理 OnLateUpdate 
    void ProcessOnLateUpdate();

    void RemainWhenSceneChange();

// Error Handling for Lua exceptions, can be extended to log to file or show in-game message box
    void ReportError(const std::string actor_name, const luabridge::LuaException& e);

private:
    static ActorManager* s_lua_actor_manager;
    static lua_State* s_lua_state;

    
};

void visualizeBox (SDL_Renderer* renderer, glm::vec2 center, glm::vec2 box,
        glm::vec2 camera = glm::vec2(0,0), float zoom_factor = 1.0f,
        SDL_Color color = {255, 0, 0, 255});

bool checkAABB(glm::vec2 ctr1, glm::vec2 box1, glm::vec2 ctr2, glm::vec2 box2);

// 统一的组件读取与添加函数：
// 无论来自模板还是 actor 自身的 components，都调用这个函数。
void readAndaddComponent(const rapidjson::Value& component_data,
                         const std::string& component_name,
                         ComponentDB* componentDB,
                         Actor* new_actor);
#endif 