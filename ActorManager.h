#ifndef ACTOR_MANAGER_H
#define ACTOR_MANAGER_H

#include <vector>
#include <memory>
#include <string>
#include "game_utils.h"
#include "component_db.h"
#include "template_db.h"

class ActorManager {
public:
    // 所有的 Actor 都住在这里，由 shared_ptr 保证地址不动且生命周期受控
    std::vector<std::shared_ptr<Actor>> all_actors;
    // 存储本帧需要调用 OnStart 的 Actor，保证在 UpdateDestruction 之前调用
    std::vector<std::shared_ptr<Actor>> actors_to_call_onstart; 
    lua_State* L; // 共享的全局 Lua 状态
    int next_actor_id = 0;
    int runtime_component_add_counter = 0;

    ActorManager(lua_State* L) : L(L) {}

    // 提供给 Scene 或 Lua 调用的创建接口
    Actor* CreateActor( std::string name, const std::shared_ptr<ComponentDB>& componentDB) {
        auto new_actor = std::make_shared<Actor>(L, next_actor_id++, name, this, componentDB);
        all_actors.push_back(new_actor);
        actors_to_call_onstart.push_back(new_actor); // 新创建的 Actor 需要在本帧调用 OnStart
        return new_actor.get();
    }


    void Destroy(Actor* actor) {
        if (!actor) return;
        actor->MarkPendingDestroy();
    }

    //每帧开始时调用此函数
    void ProcessOnStartAllActor() {
        const auto actors_snapshot = all_actors;
        for (const auto& actor : actors_snapshot) {
            if (!actor || actor->pending_destroy) continue;
            actor->ProcessOnStart();
        }
        // Kept for compatibility with existing create path; otherwise this vector
        // grows forever when actors are created at runtime.
        actors_to_call_onstart.clear();
    }

    void ProcessOnUpdateAllActor() {
        const auto actors_snapshot = all_actors;
        for (const auto& actor : actors_snapshot) {
            if (actor && !actor->pending_destroy) {
                actor->ProcessOnUpdate();
            }
        }
    }

    void ProcessOnLateUpdateAllActor() {
        const auto actors_snapshot = all_actors;
        for (const auto& actor : actors_snapshot) {
            if (actor && !actor->pending_destroy) {
                actor->ProcessOnLateUpdate();
            }
        }
    }

    // 每帧结束时调用此函数
    void UpdateDestruction() {
        actors_to_call_onstart.erase(
            std::remove_if(actors_to_call_onstart.begin(), actors_to_call_onstart.end(),
                [](const std::shared_ptr<Actor>& actor) {
                    return actor && actor->pending_destroy;
                }
            ),
            actors_to_call_onstart.end()
        );

        // 使用 Erase-Remove 惯用法
        all_actors.erase(
            std::remove_if(all_actors.begin(), all_actors.end(),
                [](const std::shared_ptr<Actor>& actor) {
                    // 如果被标记为销毁，返回 true
                    if (actor->pending_destroy) {
                        // 在这里可以做一些清理工作，比如通知 Lua 该对象已死
                        // actor->OnDestroy(); 
                        return true;
                    }
                    return false;
                }
            ),
            all_actors.end()
        );
    }

    // 场景切换时的清理逻辑（复用同样的思路）
    void ClearSceneActors() {
        all_actors.erase(
            std::remove_if(all_actors.begin(), all_actors.end(),
                [](const std::shared_ptr<Actor>& actor) {
                    return actor->destroyOnSceneChange;
                }
            ),
            all_actors.end()
        );
    }

    luabridge::LuaRef Find(std::string name){
        for (auto& actor : all_actors){
            if (!actor || actor->pending_destroy) continue;
            if (actor->name == name){
                return luabridge::LuaRef(L, actor.get());
            }
        }
        return luabridge::LuaRef(L);
    }

    luabridge::LuaRef FindAll(std::string name) {
        luabridge::LuaRef result = luabridge::newTable(L);
        int index = 1;
        for (auto& actor_ptr : all_actors) {
            if (!actor_ptr || actor_ptr->pending_destroy) continue;
            if (actor_ptr->GetName() == name) {
                result[index++] = actor_ptr.get();
            }
        }
        return result;
    }

    Actor* Instantiate(std::string templateName, TemplateDB* templateDB, const std::shared_ptr<ComponentDB>& componentDB){
        const rapidjson::Value* components_obj = templateDB->getComponentsObject();
        if (!components_obj) return nullptr; // return nullptr if no components object in template

        Actor* new_actor = CreateActor(templateDB->getName(), componentDB);
        for (auto it = components_obj->MemberBegin(); it != components_obj->MemberEnd(); ++it) {
            const std::string component_name = it->name.GetString();
            const rapidjson::Value& component_data = it->value;
            readAndaddComponent(component_data, component_name, componentDB.get(), new_actor);
        }
        return new_actor;
    }
};

#endif