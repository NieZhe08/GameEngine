// PhysicsManager.h
#pragma once
#include "box2d/box2d.h"

class PhysicsManager {
public:
    static PhysicsManager& Instance() {
        static PhysicsManager instance;
        return instance;
    }

    b2World* GetOrCreateWorld() {
        if (!world_) {
            world_ = new b2World(b2Vec2(0.0f, 9.8f));
        }
        return world_;
    }

    void Step() {
        if (world_) {
            world_->Step(1.0f / 60.0f, 8, 3);
        }
    }

    ~PhysicsManager() {
        delete world_;
        world_ = nullptr;
    }

    PhysicsManager(const PhysicsManager&) = delete;
    PhysicsManager& operator=(const PhysicsManager&) = delete;

private:
    PhysicsManager() = default;
    b2World* world_ = nullptr;
};