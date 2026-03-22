#ifndef RIGID_BODY_H
#define RIGID_BODY_H

#include "box2d/box2d.h"
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "PhysicManager.h"
#include "game_utils.h"
#include "glm/glm.hpp"

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

    std::string collider_type = "box"; // "box", "circle"
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;

    // ====== runtime ======
    b2Body* body = nullptr; // set in RigidBodySystem when creating the body in the physics world, used for runtime queries and updates.
    b2World* world = nullptr; // set in RigidBodySystem when creating the body in the physics world, used for runtime queries and updates.

    Rigidbody(b2World* world) : world(world) {}

    b2Vec2 GetPosition(){// use b2Body->GetPosition();
        return body->GetPosition();
    }

    float GetRotation(){// Use b2Body->GetAngle() and convert to degrees
        if (!body) {
            return rotation;
        }
        return RtoD((body->GetAngle() ) ); // Screen-space convention: angle is 0 when facing up (0, -1), and increases clockwise.
    }

    void _setShapeAndFixture(){
        if (!body) return;

        b2Shape* shape = nullptr;
        if (collider_type == "circle") {
            b2CircleShape* circle_shape = new b2CircleShape();
            circle_shape->m_radius = radius;
            shape = circle_shape;
        }else { // collider_type == "box" and also default
            b2PolygonShape* box_shape = new b2PolygonShape();
            box_shape->SetAsBox(width / 2.0f, height / 2.0f);
            shape = box_shape;
        } 

        if (shape) {
            b2FixtureDef fixtureDef;
            fixtureDef.shape = shape;
            fixtureDef.density = density;
            if (!has_collider && !has_trigger) {
                fixtureDef.isSensor = true;
            } else {
                fixtureDef.friction = friction;
                fixtureDef.restitution = bounciness;
            }
            body->CreateFixture(&fixtureDef);
        }
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

        _setShapeAndFixture();
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

    void AddForce(b2Vec2 vec){
        if (body) {
            body->ApplyForceToCenter(vec, true);
        }
    }

    void SetVelocity(b2Vec2 vec){
        if (body) {
            body->SetLinearVelocity(vec);
        }
    }

    void SetPosition(b2Vec2 vec){
        if (body) {
            body->SetTransform(vec, body->GetAngle());
        }
    }

    void SetRotation(float degrees_clockwise){
        if (body) {
            body->SetTransform(body->GetPosition(), DtoR(degrees_clockwise));
        }
    }

    void SetAngularVelocity(float degrees_clockwise){
        if (body) {
            body->SetAngularVelocity(DtoR(degrees_clockwise));
        }
    }

    void SetGravityScale(float scale){
        if (body) {
            body->SetGravityScale(scale);
        }
    }

    void SetUpDirection(b2Vec2 direction){
        if (!body) return;
        if (direction.Normalize() <= b2_epsilon) {
            return;
        }

        float up_angle_radians = glm::atan(direction.x, -direction.y);
        body->SetTransform(body->GetPosition(), up_angle_radians);
    }

    void SetRightDirection(b2Vec2 direction){
        if (!body) return;
        if (direction.Normalize() <= b2_epsilon) {
            return;
        }

        float up_angle_radians = glm::atan(direction.x, -direction.y);
        float right_angle_radians = up_angle_radians - (b2_pi / 2.0f);
        body->SetTransform(body->GetPosition(), right_angle_radians);
    }

    b2Vec2 GetVelocity(){
        if (body) {
            return body->GetLinearVelocity();
        }
        return b2Vec2(0.0f, 0.0f);
    }

    float GetAngularVelocity(){
        if (body) {
            return RtoD(body->GetAngularVelocity());
        }
        return 0.0f;
    }

    float GetGravityScale(){
        if (body) {
            return body->GetGravityScale();
        }
        return 1.0f;
    }

    b2Vec2 GetUpDirection(){
        if (!body) return b2Vec2(0.0f, -1.0f);
        float angle = (body->GetAngle());
        // Screen-space convention: up is (0, -1) when angle is 0.
        b2Vec2 result = b2Vec2(glm::sin(angle), -glm::cos(angle));
        result.Normalize();
        return result;
    }

    b2Vec2 GetRightDirection(){
        if (!body) return b2Vec2(1.0f, 0.0f);
        float angle = (body->GetAngle());
        // Right is +90 degrees clockwise from up in screen-space.
        b2Vec2 result = b2Vec2(glm::cos(angle), glm::sin(angle));
        result.Normalize();
        return result;
    }
};

#endif // RIGID_BODY_H