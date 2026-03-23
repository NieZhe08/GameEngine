#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

class EventBus {
private:
    struct Subscription {
        luabridge::LuaRef component;
        luabridge::LuaRef function;
    };

    enum class PendingType {
        Subscribe,
        Unsubscribe
    };

    struct PendingAction {
        PendingType type;
        std::string eventType;
        luabridge::LuaRef component;
        luabridge::LuaRef function;
    };

    static std::unordered_map<std::string, std::vector<Subscription>>& Subscriptions() {
        static std::unordered_map<std::string, std::vector<Subscription>> subscriptions;
        return subscriptions;
    }

    static std::vector<PendingAction>& PendingActions() {
        static std::vector<PendingAction> pending_actions;
        return pending_actions;
    }

    static bool IsSameLuaRef(const luabridge::LuaRef& lhs, const luabridge::LuaRef& rhs) {
        if (lhs.state() != rhs.state()) {
            return false;
        }
        return lhs.rawequal(rhs);
    }

    static bool IsSameSubscription(const Subscription& sub,
                                   const luabridge::LuaRef& component,
                                   const luabridge::LuaRef& function) {
        return IsSameLuaRef(sub.component, component) && IsSameLuaRef(sub.function, function);
    }

    static void ApplySubscribe(const std::string& event_type,
                               const luabridge::LuaRef& component,
                               const luabridge::LuaRef& function) {
        auto& list = Subscriptions()[event_type];
        list.push_back(Subscription{component, function});
    }

    static void ApplyUnsubscribe(const std::string& event_type,
                                 const luabridge::LuaRef& component,
                                 const luabridge::LuaRef& function) {
        auto map_it = Subscriptions().find(event_type);
        if (map_it == Subscriptions().end()) {
            return;
        }

        auto& list = map_it->second;
        list.erase(
            std::remove_if(list.begin(), list.end(), [&](const Subscription& sub) {
                return IsSameSubscription(sub, component, function);
            }),
            list.end());

        if (list.empty()) {
            Subscriptions().erase(map_it);
        }
    }

public:
    static void Publish(const std::string& event_type, const luabridge::LuaRef& event_object) {
        auto it = Subscriptions().find(event_type);
        if (it == Subscriptions().end()) {
            return;
        }

        const auto subscribers = it->second;
        for (const auto& sub : subscribers) {
            if (!sub.function.isFunction()) {
                continue;
            }

            try {
                sub.function(sub.component, event_object);
            } catch (luabridge::LuaException const& e) {
                std::string msg = e.what();
                std::replace(msg.begin(), msg.end(), '\\', '/');
                std::cout << "\033[31m" << "event callback: " << msg << "\033[0m" << std::endl;
            }
        }
    }

    static void Subscribe(const std::string& event_type,
                          const luabridge::LuaRef& component,
                          const luabridge::LuaRef& function) {
        if (!function.isFunction()) {
            return;
        }
        PendingActions().push_back(PendingAction{PendingType::Subscribe, event_type, component, function});
    }

    static void Unsubscribe(const std::string& event_type,
                            const luabridge::LuaRef& component,
                            const luabridge::LuaRef& function) {
        if (!function.isFunction()) {
            return;
        }
        PendingActions().push_back(PendingAction{PendingType::Unsubscribe, event_type, component, function});
    }

    static void FlushPending() {
        auto& pending = PendingActions();
        for (const auto& action : pending) {
            if (action.type == PendingType::Subscribe) {
                ApplySubscribe(action.eventType, action.component, action.function);
            } else {
                ApplyUnsubscribe(action.eventType, action.component, action.function);
            }
        }
        pending.clear();
    }

    static void ClearAll() {
        PendingActions().clear();
        Subscriptions().clear();
    }
};
