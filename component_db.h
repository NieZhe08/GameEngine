#ifndef COMPONENT_DB_H
#define COMPONENT_DB_H

#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <string>

#include "rapidjson/document.h"
#include "game_utils.h" // for Actor and lua includes
#include <lua/lua.hpp>
#include <LuaBridge/LuaBridge.h>

// ComponentDB: 管理 Lua 组件类型（resources/component_types/*.lua）
// 负责：
//  - 按类型名加载 / 缓存基础表
//  - 为某个 Actor 实例化组件 table
//  - 设置继承、self.key、self.actor、enabled 等基础字段
class ComponentDB {
    
public:
    explicit ComponentDB(lua_State* L)
        : L(L) {}

    // 使用 Lua metatable 在 Lua 中建立继承关系：
    // setmetatable(instance_table, { __index = parent_table })
    void EstablishInheritance(luabridge::LuaRef& instance_table,
                              luabridge::LuaRef& parent_table) {
        lua_State* state = instance_table.state();

        // 创建新的 metatable，并设置 __index 指向父表
        luabridge::LuaRef new_metatable = luabridge::newTable(state);
        new_metatable["__index"] = parent_table;
        
        instance_table.push(state);       // [..., instance_table]
        new_metatable.push(state);       // [..., instance_table, new_metatable]
        lua_setmetatable(state, -2);     // [..., instance_table] with new
        lua_pop(state, -1);
    }

    // 为给定类型和 key 创建一个新的组件实例，并绑定到 owner 上
    // 不处理属性覆盖，属性由调用方根据 json 再写入 instance 字段
    luabridge::LuaRef CreateInstance(const std::string& typeName,
                                     const std::string& key,
                                     Actor* owner) {
        luabridge::LuaRef base = getOrLoadBase(typeName);
        lua_State* state = base.state();

        luabridge::LuaRef instance = luabridge::newTable(state);

        // 使用统一的继承函数，让实例继承组件类型基础表
        EstablishInheritance(instance, base);

        // 设置基础字段：key / actor / enabled
        instance["key"] = key;
        instance["type"] = typeName;
        if (owner) {
            instance["actor"] = owner;
        }
        instance["enabled"] = true; // Test #6 要求的初始 enabled = true

        return instance;
    }

    // 将 json 对象中的属性写入组件实例（除去 "type" 字段）
    void ApplyProperties(luabridge::LuaRef& instance, const rapidjson::Value& obj) {
        if (!obj.IsObject()) return;

        for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
            const std::string propName = it->name.GetString();
            if (propName == "type") continue; // 保留 type 只用于选择脚本

            const rapidjson::Value& v = it->value;
            if (v.IsBool()) {
                instance[propName] = v.GetBool();
            } else if (v.IsInt()) {
                instance[propName] = v.GetInt();
            } else if (v.IsUint()) {
                instance[propName] = static_cast<int>(v.GetUint());
            } else if (v.IsInt64()) {
                instance[propName] = static_cast<long long>(v.GetInt64());
            } else if (v.IsUint64()) {
                instance[propName] = static_cast<long long>(v.GetUint64());
            } else if (v.IsDouble()) {
                instance[propName] = v.GetDouble();
            } else if (v.IsString()) {
                instance[propName] = std::string(v.GetString());
            }
            // 其它类型（数组 / 对象）本作业不需要，忽略即可
        }
    }

    void AddComponentToActor(Actor* actor, const std::string& typeName){
        std::string key = "r" + std::to_string(addComponentCounter++) + typeName;
        luabridge::LuaRef instance = CreateInstance(typeName, key, actor);
    }

private:
    lua_State* L;
    // 缓存组件类型的基础 Lua 表：typeName -> base table
    int addComponentCounter = 0;
    std::unordered_map<std::string, luabridge::LuaRef> baseCache;

    // 加载或获取缓存的组件基础表
    luabridge::LuaRef getOrLoadBase(const std::string& typeName) {
        auto it = baseCache.find(typeName);
        if (it != baseCache.end()) {
            return it->second;
        }

        // 检查脚本是否存在
        std::filesystem::path path = std::filesystem::path("resources/component_types/")
                                     / (typeName + ".lua");
        if (!std::filesystem::exists(path)) {
            // 规格：error: failed to locate component <component_type>
            std::cout << "error: failed to locate component " << typeName << std::endl;
            std::exit(0);
        }

        // 加载 lua 文件
        int status = luaL_dofile(L, path.string().c_str());
        if (status != 0) {
            // 规格：problem with lua file <lua_filename>（不带扩展名）
            std::cout << "problem with lua file " << typeName << std::endl;
            std::exit(0);
        }

        // 从全局表中取出同名表
        luabridge::LuaRef base = luabridge::getGlobal(L, typeName.c_str());
        if (base.isNil() || !base.isTable()) {
            std::cout << "problem with lua file " << typeName << std::endl;
            std::exit(0);
        }

        baseCache.emplace(typeName, base);
        return base;
    }
};

#endif // COMPONENT_DB_H
