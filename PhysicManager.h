// PhysicsManager.h
#pragma once
#include "box2d/box2d.h"
#include "game_utils.h"

class PhysicsContactListener : public b2ContactListener {
public:
    static Actor* ActorFromFixture(b2Fixture* fixture) {
        if (!fixture) return nullptr;
        const uintptr_t ptr = fixture->GetUserData().pointer;
        if (ptr == 0) return nullptr;
        return reinterpret_cast<Actor*>(ptr);
    }

    static b2Vec2 ComputeRelativeVelocity(const b2Fixture* self_fixture, const b2Fixture* other_fixture) {
        if (!self_fixture || !other_fixture) {
            return b2Vec2(0.0f, 0.0f);
        }
        const b2Body* self_body = self_fixture->GetBody();
        const b2Body* other_body = other_fixture->GetBody();
        if (!self_body || !other_body) {
            return b2Vec2(0.0f, 0.0f);
        }
        return other_body->GetLinearVelocity() - self_body->GetLinearVelocity();
    }

    void Dispatch(b2Contact* contact, const char* lifecycle_name, bool is_begin_contact) {
        if (!contact || !lifecycle_name) return;

        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();
        if (!fixtureA || !fixtureB) return;

        Actor* actorA = ActorFromFixture(fixtureA);
        Actor* actorB = ActorFromFixture(fixtureB);

        b2WorldManifold world_manifold;
        contact->GetWorldManifold(&world_manifold);

        const b2Vec2 invalid_vector(-999.0f, -999.0f);
        const b2Vec2 point = is_begin_contact ? world_manifold.points[0] : invalid_vector;
        const b2Vec2 normal = is_begin_contact ? world_manifold.normal : invalid_vector;

        if (actorA) {
            Collision collision_for_a;
            collision_for_a.other = actorB;
            collision_for_a.point = point;
            collision_for_a.normal = normal;
            collision_for_a.relative_velocity = ComputeRelativeVelocity(fixtureA, fixtureB);
            actorA->ProcessCollisionLifecycle(lifecycle_name, collision_for_a);
        }

        if (actorB) {
            Collision collision_for_b;
            collision_for_b.other = actorA;
            collision_for_b.point = point;
            collision_for_b.normal = normal;
            collision_for_b.relative_velocity = ComputeRelativeVelocity(fixtureB, fixtureA);
            actorB->ProcessCollisionLifecycle(lifecycle_name, collision_for_b);
        }
    }

    void BeginContact(b2Contact* contact) override {
        Dispatch(contact, "OnCollisionEnter", true);
    }

    void EndContact(b2Contact* contact) override {
        Dispatch(contact, "OnCollisionExit", false);
    }
};

class PhysicsManager {
public:
    static PhysicsManager& Instance() {
        static PhysicsManager instance;
        return instance;
    }

    b2World* GetOrCreateWorld() {
        if (!world_) {
            world_ = new b2World(b2Vec2(0.0f, 9.8f));
            world_->SetContactListener(&contact_listener_);
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
    PhysicsContactListener contact_listener_;
};