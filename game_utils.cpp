#include "game_utils.h"

#include <cctype>
#include "component_db.h"

std::optional<std::string> extractProceedTarget(const std::string& input) {
    const std::string prefix = "proceed to ";

    auto it = std::search(
        input.begin(), input.end(),
        prefix.begin(), prefix.end(),
        [](unsigned char ch1, unsigned char ch2) {
            return std::tolower(ch1) == std::tolower(ch2);
        }
    );

    if (it != input.end()) {
        size_t startPos = std::distance(input.begin(), it) + prefix.length();

        size_t endPos = startPos;
        while (endPos < input.length() &&
               (std::isalnum(static_cast<unsigned char>(input[endPos])) || input[endPos] == '_')) {
            endPos++;
        }

        if (endPos > startPos) {
            return input.substr(startPos, endPos - startPos);
        }
    }

    return std::nullopt;
}

std::string obtain_word_after_phrase(const std::string& input, const std::string& phrase) {
    size_t pos = input.find(phrase);
    if (pos == std::string::npos) return "";

    pos += phrase.length();
    while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) {
        pos++;
    }

    if (pos == input.size()) return "";

    size_t endPos = pos;
    while (endPos < input.size() && !std::isspace(static_cast<unsigned char>(input[endPos]))) {
        endPos++;
    }

    return input.substr(pos, endPos - pos);
}

GameIncident checkGameIncidents(std::string dialogue) {
    if (dialogue.find("health down") != std::string::npos) {
        return GameIncident::HealthDown;
    } else if (dialogue.find("score up") != std::string::npos) {
        return GameIncident::ScoreUp;
    } else if (dialogue.find("you win") != std::string::npos) {
        return GameIncident::YouWin;
    } else if (dialogue.find("game over") != std::string::npos) {
        return GameIncident::GameOver;
    }
    return GameIncident::None;
}

ActorManager* Actor::s_lua_actor_manager = nullptr;
lua_State* Actor::s_lua_state = nullptr;

Actor::Actor(lua_State* L, int id, std::string name, ActorManager* am, ComponentDB* cdb)
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

void Actor::AddComponent(std::string type) {
    this->componentDB->AddComponentToActor(this, type);
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
                    ReportError(e);
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
                ReportError(e);
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
                ReportError(e);
            }
        }
    }
}

void Actor::RemainWhenSceneChange() {
    destroyOnSceneChange = false;
}

void Actor::ReportError(const luabridge::LuaException& e) {
    std::string msg = e.what();
    std::cout << msg << std::endl;
}

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

glm::ivec2 worldToCell(const glm::vec2& worldPos, const glm::vec2& cell_size) {
    return glm::ivec2(std::floor(worldPos.x / cell_size.x),
                      std::floor(worldPos.y / cell_size.y));
}

void readAndaddComponent(const rapidjson::Value& component_data,
                         const std::string& component_name,
                         ComponentDB* componentDB,
                         Actor* new_actor) {
    if (!component_data.IsObject()) return;
    if (!component_data.HasMember("type") || !component_data["type"].IsString()) return;

    std::string type = component_data["type"].GetString();
    luabridge::LuaRef instance = componentDB->CreateInstance(type, component_name, new_actor);

    componentDB->ApplyProperties(instance, component_data);
    new_actor->components.insert_or_assign(component_name, instance);
}
