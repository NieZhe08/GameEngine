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
#include "rapidjson/document.h"

enum class PlayerAction {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Quit,
    Invalid
};

enum class GameIncident {
    HealthDown,
    ScoreUp,
    YouWin,
    GameOver,
    None, 
    NextScene
};

enum class GameState {
    Ongoing,
    Won,
    Lost,
    NextScene,
    IntroAnimation,
    Quit
};

enum class ContactType {
    Nearby,
    Overlap
};

std::optional<std::string> extractProceedTarget(const std::string& input);
std::string obtain_word_after_phrase(const std::string& input, const std::string& phrase);
GameIncident checkGameIncidents(std::string dialogue);

class ActorManager;
class ComponentDB;



class Actor {
public:
    // 基础属性 
    std::string name;
    int id;
    lua_State* L;
    ActorManager* actorManager = nullptr;
    ComponentDB* componentDB = nullptr;
    bool pending_destroy = false;
    bool destroyOnSceneChange = true;
    
    // component system: key -> component instance (LuaRef)
    std::map<std::string, luabridge::LuaRef> components;
    // track components that have had OnStart called
    std::set<std::string> started_components;

    Actor(lua_State* L, int id, std::string name, ActorManager* am, ComponentDB* cdb);

    ~Actor();

    void MarkPendingDestroy();

    // --- Lua API ---

    std::string GetName() const;
    int GetID() const;

    luabridge::LuaRef GetComponentByKey(std::string key);

    luabridge::LuaRef GetComponent(std::string type);

    luabridge::LuaRef GetComponents(std::string type);

    

    void AddComponent(std::string type);

    void RemoveComponent(luabridge::LuaRef component);

    // --- 引擎内部生命周期驱动 ---

    // 处理 OnStart 
    void ProcessOnStart();

    // 处理 OnUpdate 
    void ProcessOnUpdate();

    // 处理 OnLateUpdate 
    void ProcessOnLateUpdate();

    void RemainWhenSceneChange();

private:
    static ActorManager* s_lua_actor_manager;
    static lua_State* s_lua_state;

    // Error Handling for Lua exceptions, can be extended to log to file or show in-game message box
    void ReportError(const luabridge::LuaException& e);
};

inline std::uint64_t hashPosition(const glm::ivec2& position) {
    // A simple hash function combining x and y coordinates into a 64-bit key.
    // Ensure we shift a 64-bit value to avoid 32-bit shift overflow/UB.
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(position.x)) << 32) |
           static_cast<std::uint64_t>(static_cast<std::uint32_t>(position.y));
}

struct ActorSmallerId {
    bool operator() (const Actor* a1, const Actor* a2) const {
        return a1->id < a2->id; 
    }
};

//functor ActorRenderComparator
//struct ActorRenderComparator {
//    bool operator() (const Actor* a1, const Actor* a2) const {
//        if (a1->render_order != a2->render_order) {
//            return a1->render_order > a2->render_order; // Higher render_order is higher priority
//        }
//        else if (a1->transform_position.y != a2->transform_position.y) {
//            return a1->transform_position.y > a2->transform_position.y; // Higher y is lower priority
//        }
//        return a1->id > a2->id; // For same y, smaller id is lower priority
//    }
//};

void visualizeBox (SDL_Renderer* renderer, glm::vec2 center, glm::vec2 box,
        glm::vec2 camera = glm::vec2(0,0), float zoom_factor = 1.0f,
        SDL_Color color = {255, 0, 0, 255});

bool checkAABB(glm::vec2 ctr1, glm::vec2 box1, glm::vec2 ctr2, glm::vec2 box2);

// Hash function for glm::ivec2 to use in unordered_map
struct Ivec2Hash {
    // Runtime configurable cell size for spatial hashing
    static inline glm::vec2 cell_size;
    
    std::size_t operator()(const glm::ivec2& v) const {
        // Combine hash of x and y components using bit shifting and XOR
        std::size_t h1 = std::hash<int>{}(v.x);
        std::size_t h2 = std::hash<int>{}(v.y);
        return h1 ^ (h2 << 1);
    }
};

struct Ivec2HashTrigger {
    // Runtime configurable cell size for spatial hashing (for triggers)
    static inline glm::vec2 cell_size;
    
    std::size_t operator()(const glm::ivec2& v) const {
        // Combine hash of x and y components using bit shifting and XOR
        std::size_t h1 = std::hash<int>{}(v.x);
        std::size_t h2 = std::hash<int>{}(v.y);
        return h1 ^ (h2 << 1);
    }
};

struct Ivec2HashWindow {
    // Runtime configurable cell size for spatial hashing (for window-based culling)
    static inline glm::vec2 cell_size;
    
    std::size_t operator()(const glm::ivec2& v) const {
        // Combine hash of x and y components using bit shifting and XOR
        std::size_t h1 = std::hash<int>{}(v.x);
        std::size_t h2 = std::hash<int>{}(v.y);
        return h1 ^ (h2 << 1);
    }
};

glm::ivec2 worldToCell(const glm::vec2& worldPos, const glm::vec2& cell_size);

// 统一的组件读取与添加函数：
// 无论来自模板还是 actor 自身的 components，都调用这个函数。
void readAndaddComponent(const rapidjson::Value& component_data,
                         const std::string& component_name,
                         ComponentDB* componentDB,
                         Actor* new_actor);
#endif 