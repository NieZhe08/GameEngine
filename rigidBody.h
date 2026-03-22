#ifndef RIGID_BODY_H
#define RIGID_BODY_H

#include "box2d/box2d.h"
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "PhysicManager.h"
#include "game_utils.h"

#include <string>

class Rigidbody {
public:
    // component common methods
    bool enabled = true; // Whether the component is active. Inactive components should not perform their behavior, but their data should still be queryable and modifiable.
    std::string key = "";
    std::string type = "Rigidbody";
    Actor* actor = nullptr; // set when the component is added to an actor, should not be set manually in .scene files.

    float x = 0.0f; // Warning : Only valid during setup-time (it’s a .scene parameter)
    float y = 0.0f; // Warning : Only valid during setup-time (it’s a .scene parameter)
    
    std::string body_type = "dynamic"; // "static", "dynamic", or "kinematic". Warning : Only valid during setup-time (it’s a .scene parameter)
    
    bool precise = true; // Use b2BodyDef.bullet
    float gravity_scale = 1.0f; // Use b2BodyDef.gravityScale
    float density = 1.0f; // see below to set density on a fixture.
    float angular_friction = 0.3f; // Use b2BodyDef.angularDamping
    float rotation = 0.0f; // Units of degrees (not radians).
    
    bool has_collider = true; // Not used for anything just yet, but please add it.
    bool has_trigger = true; // Not used for anything just yet, but please add it.

    // ====== runtime ======
    b2Body* body = nullptr; // set in RigidBodySystem when creating the body in the physics world, used for runtime queries and updates.
    b2World* world = nullptr; // set in RigidBodySystem when creating the body in the physics world, used for runtime queries and updates.

    Rigidbody(b2World* world) : world(world) {}

    b2Vec2 GetPosition(){// use b2Body->GetPosition();
        return body->GetPosition();
    }

    float GetRotation(){// Use b2Body->GetAngle() and convert to degrees
        return RtoD(body->GetAngle());
    }

    void initialize(){
        b2BodyDef bodyDef;
        if (body_type == "static") {
            bodyDef.type = b2_staticBody;
        } else if (body_type == "kinematic") {
            bodyDef.type = b2_kinematicBody;
        } else {
            bodyDef.type = b2_dynamicBody;
        }
        bodyDef.position.Set(x, y);
        bodyDef.angle = DtoR(rotation);
        bodyDef.bullet = precise;
        bodyDef.gravityScale = gravity_scale;
        bodyDef.angularDamping = angular_friction;

        body = world->CreateBody(&bodyDef);

        b2PolygonShape my_shape;
        my_shape.SetAsBox(0.5f, 0.5f); // default to a 1x1 box collider, can be overridden by adding a Collider component to the

        b2FixtureDef fixture;
        fixture.shape = &my_shape;
        fixture.density = 1.0f;
        body->CreateFixture(&fixture);
    }

    void OnStart(){
        initialize();
    }

    void OnUpdate(){
        // For now we don't have any runtime behavior for Rigidbody, but this is where you would add it.
    }

    void OnLateUpdate(){
        // For now we don't have any runtime behavior for Rigidbody, but this is where you would add it.
    }

    void OnDestroy(){
        if (body) {
            b2World* world = body->GetWorld();
            world->DestroyBody(body);
            body = nullptr;
        }
    }
};

#endif // RIGID_BODY_H