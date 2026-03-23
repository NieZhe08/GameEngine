#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "game_utils.h"
#include "PhysicManager.h"

class HitResult{
    public:
        Actor* actor = nullptr; // the actor our raycast found.
        b2Vec2 point = b2Vec2(0.0f, 0.0f); // the point at which the raycast struck a fixture of the actor.
        b2Vec2 normal = b2Vec2(0.0f, 0.0f); // the normal vector at the point.
        bool is_trigger = false; // whether or not the fixture encountered is a trigger (sensor).
};

class ClosestActorRayCastCallback : public b2RayCastCallback {
public:
    b2Fixture* fixture = nullptr;
    b2Vec2 point = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal = b2Vec2(0.0f, 0.0f);
    bool hit = false;

    float ReportFixture(b2Fixture* fixture_in,
                        const b2Vec2& point_in,
                        const b2Vec2& normal_in,
                        float fraction) override {
        Actor* actor = ActorFromFixture(fixture_in);
        if (!actor || fixture_in->GetFilterData().categoryBits == Rigidbody::kNotDefinedCategory) {
            // Ignore fixtures that are not tied to an Actor and continue.
            return -1.0f;
        }

        hit = true;
        fixture = fixture_in;
        point = point_in;
        normal = normal_in;

        // Clip the ray to the current hit so the final stored hit is the closest one.
        return fraction;
    }
};

class AllActorsRayCastCallback : public b2RayCastCallback {
public:
    struct HitWithFraction {
        HitResult hit;
        float fraction = 0.0f;
    };

    std::vector<HitWithFraction> hits;

    float ReportFixture(b2Fixture* fixture_in,
                        const b2Vec2& point_in,
                        const b2Vec2& normal_in,
                        float fraction) override {
        Actor* actor = ActorFromFixture(fixture_in);
        if (!actor || fixture_in->GetFilterData().categoryBits == Rigidbody::kNotDefinedCategory) {
            // Ignore fixtures that are not tied to an Actor and continue.
            return -1.0f;
        }

        HitWithFraction entry;
        entry.hit.actor = actor;
        entry.hit.point = point_in;
        entry.hit.normal = normal_in;
        entry.hit.is_trigger = fixture_in->IsSensor();
        entry.fraction = fraction;
        hits.push_back(entry);

        // Continue the ray cast to find all fixtures in the path of the ray.
        return 1.0f;
    }
};

class RayCastManager {
public:
    static luabridge::LuaRef Raycast(const b2Vec2& pos, const b2Vec2& dir, float dist, lua_State* L) {
        b2World* world_ = PhysicsManager::Instance().GetOrCreateWorld();
        if (!world_ || dist <= 0.0f) return luabridge::LuaRef(L);
        if (b2Dot(dir, dir) <= b2_epsilon * b2_epsilon) return luabridge::LuaRef(L);

        b2Vec2 normalized_dir = dir;
        normalized_dir.Normalize();
        const b2Vec2 end = pos + dist * normalized_dir;

        ClosestActorRayCastCallback callback;
        world_->RayCast(&callback, pos, end);

        if (!callback.hit || !callback.fixture) {
            return luabridge::LuaRef(L);
        }

        Actor* actor = ActorFromFixture(callback.fixture);
        if (!actor) {
            return luabridge::LuaRef(L);
        }

        luabridge::LuaRef result = luabridge::newTable(L);
        result["actor"] = actor;
        result["point"] = callback.point;
        result["normal"] = callback.normal;
        result["is_trigger"] = callback.fixture->IsSensor();

        return result;
    }

    static luabridge::LuaRef RaycastAll(const b2Vec2& pos, const b2Vec2& dir, float dist, lua_State* L) {
        b2World* world_ = PhysicsManager::Instance().GetOrCreateWorld();
        luabridge::LuaRef results = luabridge::newTable(L);
        if (!world_ || dist <= 0.0f) return results;
        if (b2Dot(dir, dir) <= b2_epsilon * b2_epsilon) return results;

        b2Vec2 normalized_dir = dir;
        normalized_dir.Normalize();
        const b2Vec2 end = pos + dist * normalized_dir;

        AllActorsRayCastCallback callback;
        world_->RayCast(&callback, pos, end);

        std::sort(callback.hits.begin(), callback.hits.end(),
                  [](const AllActorsRayCastCallback::HitWithFraction& a,
                     const AllActorsRayCastCallback::HitWithFraction& b) {
                      return a.fraction < b.fraction;
                  });

        int index = 1;
        for (const auto& entry : callback.hits) {
            luabridge::LuaRef hit = luabridge::newTable(L);
            hit["actor"] = entry.hit.actor;
            hit["point"] = entry.hit.point;
            hit["normal"] = entry.hit.normal;
            hit["is_trigger"] = entry.hit.is_trigger;
            results[index++] = hit;
        }

        return results;
    }
};


