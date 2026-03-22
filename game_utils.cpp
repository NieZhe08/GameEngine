#include "game_utils.h"

#include <cctype>
#include "ActorManager.h"
#include "component_db.h"

// disabled forward declaration, we would NOT want to use that.
//ActorManager* Actor::s_lua_actor_manager = nullptr;
//lua_State* Actor::s_lua_state = nullptr;

Actor::Actor(lua_State* L, int id, std::string name, ActorManager* am, const std::shared_ptr<ComponentDB>& cdb)
    : name(std::move(name)),
      id(id),
      L(L),
      actorManager(am),
      componentDB(cdb) {}

Actor::~Actor() {
    // components will automatically release their Lua references when they go out of scope
    components.clear();
}

void Actor::MarkPendingDestroy() {
    pending_destroy = true;
}

std::string Actor::GetName() const {
    return name;
}

int Actor::GetID() const {
    return id;
}

luabridge::LuaRef Actor::GetComponentByKey(std::string key) {
    auto it = components.find(key);
    if (it != components.end()) {
        return it->second;
    }
    return luabridge::LuaRef(L);
}

luabridge::LuaRef Actor::GetComponent(std::string type) {
    //std::cout<<components.size() << std::endl;
    for (auto& [key, instance] : components) {
        (void)key;
        if (instance["type"].cast<std::string>() == type) {
            if (instance["enabled"].cast<bool>() == false) {
                return luabridge::LuaRef(L);
            }
            return instance;
        }
    }
    return luabridge::LuaRef(L);
}

luabridge::LuaRef Actor::GetComponents(std::string type) {
    luabridge::LuaRef result = luabridge::newTable(L);
    int index = 1;

    // std::map keeps keys sorted, so iterating preserves key order.
    for (const auto& [key, instance] : components) {
        (void)key;
        if (instance["type"].cast<std::string>() != type) {
            continue;
        }
        if (instance["enabled"].cast<bool>() == false) {
            continue;
        }
        result[index++] = instance;
    }

    return result;
}

luabridge::LuaRef Actor::AddComponent(std::string type) {
    luabridge::LuaRef instance = this->componentDB->AddComponentToActor(this, type);
    if (!instance.isNil() && this->actorManager) {
        this->actorManager->QueueActorForOnStart(this);
    }
    return instance;
}

void Actor::RemoveComponent(luabridge::LuaRef component) {
    component["enabled"] = false;
}

void Actor::ProcessOnStart() {
    for (auto& [key, instance] : components) {
        if (pending_destroy) break;
        if (instance["enabled"].cast<bool>() == false) continue;

        if (started_components.find(key) == started_components.end()) {
            luabridge::LuaRef onStart = instance["OnStart"];
            if (onStart.isFunction()) {
                try {
                    onStart(instance);
                } catch (luabridge::LuaException const& e) {
                    ReportError(this->name, e);
                }
            }
            started_components.insert(key);
        }
    }
}

void Actor::ProcessOnUpdate() {
    if (pending_destroy) return;

    for (auto& [key, instance] : components) {
        if (pending_destroy) break;
        if (started_components.find(key) == started_components.end()) continue;
        if (instance["enabled"].cast<bool>() == false) continue;

        luabridge::LuaRef onUpdate = instance["OnUpdate"];
        if (onUpdate.isFunction()) {
            try {
                onUpdate(instance);
            } catch (luabridge::LuaException const& e) {
                ReportError(this->name, e);
            }
        }
    }
}

void Actor::ProcessOnLateUpdate() {
    if (pending_destroy) return;

    for (auto& [key, instance] : components) {
        if (pending_destroy) break;
        if (started_components.find(key) == started_components.end()) continue;
        if (instance["enabled"].cast<bool>() == false) continue;

        luabridge::LuaRef onLateUpdate = instance["OnLateUpdate"];
        if (onLateUpdate.isFunction()) {
            try {
                onLateUpdate(instance);
            } catch (luabridge::LuaException const& e) {
                ReportError(this->name, e);
            }
        }
    }
}

void Actor::ProcessCollisionLifecycle(const char* lifecycle_name, const Collision& collision) {
    if (pending_destroy || !lifecycle_name) return;

    for (auto& [key, instance] : components) {
        if (pending_destroy) break;
        if (started_components.find(key) == started_components.end()) continue;
        if (instance["enabled"].cast<bool>() == false) continue;

        luabridge::LuaRef lifecycle_fn = instance[lifecycle_name];
        if (!lifecycle_fn.isFunction()) continue;

        try {
            lifecycle_fn(instance, collision);
        } catch (luabridge::LuaException const& e) {
            ReportError(this->name, e);
        }
    }
}

void Actor::RemainWhenSceneChange() {
    destroyOnSceneChange = false;
}

void Actor::ReportError(const std::string actor_name, const luabridge::LuaException& e) {
    std::string msg = e.what();
    std::replace(msg.begin(), msg.end(), '\\', '/');
    std::cout << "\033[31m"<<actor_name << " : " << msg << "\033[0m" << std::endl;
}

// Old utils we may use for physics debugging
void visualizeBox(SDL_Renderer* renderer,
                  glm::vec2 center,
                  glm::vec2 box,
                  glm::vec2 camera,
                  float zoom_factor,
                  SDL_Color color) {
    glm::vec2 screen_center = center * 100.0f * zoom_factor + camera;
    glm::vec2 screen_box = box * zoom_factor * 100.0f;

    SDL_FRect rect = {
        screen_center.x - screen_box.x / 2.0f,
        screen_center.y - screen_box.y / 2.0f,
        screen_box.x,
        screen_box.y
    };

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRectF(renderer, &rect);
}

bool checkAABB(glm::vec2 ctr1, glm::vec2 box1, glm::vec2 ctr2, glm::vec2 box2) {
    if (ctr1.x + box1.x / 2 > ctr2.x - box2.x / 2 && ctr1.x - box1.x / 2 < ctr2.x + box2.x / 2) {
        if (ctr1.y + box1.y / 2 > ctr2.y - box2.y / 2 && ctr1.y - box1.y / 2 < ctr2.y + box2.y / 2) {
            return true;
        }
    }
    return false;
}

// shared utility function to read component data from JSON and add to actor, used in both scene loading and AddComponentFromJson API.
void readAndaddComponent(const rapidjson::Value& component_data,
                         const std::string& component_name,
                         ComponentDB* componentDB,
                         Actor* new_actor) {
    if (!componentDB || !new_actor) return;
    if (!component_data.IsObject()) return;
   
    std::string type;
   
    auto it = new_actor->components.find(component_name);
    if (it != new_actor->components.end() && !it->second.isNil()) {
        // Existing component with same key: merge by overriding only fields present in JSON.
        // Keep prior fields that are absent from JSON, and explicitly allow type override.
        luabridge::LuaRef& instance = it->second;
        if (component_data.HasMember("type") && component_data["type"].IsString()) {
            // error: don;t delete the original instance, just update its type field and apply properties as usual. 
            type = component_data["type"].GetString();
            luabridge::LuaRef instance = componentDB->CreateInstance(type, component_name, new_actor);
            componentDB->ApplyProperties(instance, component_data);
            new_actor->components.insert_or_assign(component_name, instance);
        }
        componentDB->ApplyProperties(instance, component_data);
    } else {
        if (!component_data.HasMember("type") || !component_data["type"].IsString()) return;
        std::string type = component_data["type"].GetString();
        luabridge::LuaRef instance = componentDB->CreateInstance(type, component_name, new_actor);
        componentDB->ApplyProperties(instance, component_data);
        new_actor->components.insert_or_assign(component_name, instance);
    }
}

float RtoD(float radians){
    return (radians) * (180.0f / b2_pi);
}

float DtoR(float degrees){
    return (degrees) * (b2_pi / 180.0f);
}
