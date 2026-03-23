// PhysicsManager.h
#pragma once
#include "box2d/box2d.h"
#include "game_utils.h"
#include "rigidBody.h"

class PhysicsContactListener : public b2ContactListener {
public:

    static b2Vec2 ComputeRelativeVelocity(const b2Fixture* fixture_A, const b2Fixture* fixture_B) {
        if (!fixture_A || !fixture_B) {
            return b2Vec2(0.0f, 0.0f);
        }
        const b2Body* self_body = fixture_A->GetBody();
        const b2Body* other_body = fixture_B->GetBody();
        if (!self_body || !other_body) {
            return b2Vec2(0.0f, 0.0f);
        }
        return -other_body->GetLinearVelocity() + self_body->GetLinearVelocity();
    }

    void Dispatch(b2Contact* contact, const char* lifecycle_name, bool use_contact_point_and_normal) {
        if (!contact || !lifecycle_name) return;

        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();
        if (!fixtureA || !fixtureB) return;

        Actor* actorA = ActorFromFixture(fixtureA);
        Actor* actorB = ActorFromFixture(fixtureB);

        b2WorldManifold world_manifold;
        contact->GetWorldManifold(&world_manifold);

        const b2Vec2 invalid_vector(-999.0f, -999.0f);
        const b2Vec2 point = use_contact_point_and_normal ? world_manifold.points[0] : invalid_vector;
        const b2Vec2 normal = use_contact_point_and_normal ? world_manifold.normal : invalid_vector;
        const b2Vec2 relative_velocity = ComputeRelativeVelocity(fixtureA, fixtureB);

        if (actorA) {
            Collision collision_for_a;
            collision_for_a.other = actorB;
            collision_for_a.point = point;
            collision_for_a.normal = normal;
            collision_for_a.relative_velocity = relative_velocity;
            actorA->ProcessPhysicsLifecycle(lifecycle_name, collision_for_a);
        }

        if (actorB) {
            Collision collision_for_b;
            collision_for_b.other = actorA;
            collision_for_b.point = point;
            collision_for_b.normal = normal;
            collision_for_b.relative_velocity = relative_velocity;
            actorB->ProcessPhysicsLifecycle(lifecycle_name, collision_for_b);
        }
    }

    void BeginContact(b2Contact* contact) override {
        if (!contact) return;
        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();
        if (!fixtureA || !fixtureB) return;

        b2Filter filterA = fixtureA->GetFilterData();
        b2Filter filterB = fixtureB->GetFilterData();

        const bool a_is_collider = (filterA.categoryBits == Rigidbody::kColliderCategory);
        const bool b_is_collider = (filterB.categoryBits == Rigidbody::kColliderCategory);
        const bool a_is_trigger = (filterA.categoryBits == Rigidbody::kTriggerCategory);
        const bool b_is_trigger = (filterB.categoryBits == Rigidbody::kTriggerCategory);

        if (a_is_collider && b_is_collider) {
            Dispatch(contact, "OnCollisionEnter", true);
        }

        if (a_is_trigger && b_is_trigger) {
            // Trigger callbacks always carry sentinel point/normal values.
            Dispatch(contact, "OnTriggerEnter", false);
            return;
        }
    }

    void EndContact(b2Contact* contact) override {
        if (!contact) return;
        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();
        if (!fixtureA || !fixtureB) return;

        b2Filter filterA = fixtureA->GetFilterData();
        b2Filter filterB = fixtureB->GetFilterData();

        const bool a_is_collider = (filterA.categoryBits == Rigidbody::kColliderCategory);
        const bool b_is_collider = (filterB.categoryBits == Rigidbody::kColliderCategory);
        const bool a_is_trigger = (filterA.categoryBits == Rigidbody::kTriggerCategory);
        const bool b_is_trigger = (filterB.categoryBits == Rigidbody::kTriggerCategory);

        if (a_is_collider && b_is_collider) {
            Dispatch(contact, "OnCollisionExit", false);
        }

        if (a_is_trigger && b_is_trigger) {
            Dispatch(contact, "OnTriggerExit", false);
            return;
        }
        
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