#pragma once
#include <iostream>
#include "Helper.h"
#include "glm/glm.hpp"
#include <vector>
#include <string>
#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"
#include "image_db.h"
#include "PhysicManager.h"
#include <queue>
#include <algorithm>

class Actor;

class ParticleSystem {
public:
    bool enabled = true;
    bool do_destroy = false;
    std::string key;
    std::string type = "ParticleSystem";
    Actor* actor = nullptr;

    RandomEngine* emit_angle_distribution = nullptr; // in degrees, 0 means facing up (0, -1), and increases clockwise.
    RandomEngine* emit_radius_distribution = nullptr;
    RandomEngine* rotation_distribution = nullptr; // in degrees, 0 means facing up (0, -1), and increases clockwise.
    RandomEngine* scale_distribution = nullptr;
    RandomEngine* speed_distribution = nullptr;
    RandomEngine* rotation_speed_distribution = nullptr; // in degrees per second, positive means clockwise
    ImageManager* image_manager = nullptr;

    // data
    std::vector<bool> is_active;
    std::vector<int> start_frame;
    std::queue<int> free_list;
    std::queue<int> pending_delete;
    std::vector<b2Body*> particle_bodies;
    std::vector<float> particle_x;
    std::vector<float> particle_y;
    std::vector<float> particle_vx;
    std::vector<float> particle_vy;
    std::vector<float> particle_rotation;
    std::vector<float> particle_angular_velocity;
    std::vector<float> particle_start_scale;

    // testsuite #0
    float emit_angle_min = 0.0f;
    float emit_angle_max = 360.0f;
    float emit_radius_min = 0.0f;
    float emit_radius_max = 0.5f;

    // testsuite #1
    float rotation_min = 0.0f;
    float rotation_max = 0.0f;
    float start_scale_min = 1.0f;
    float start_scale_max = 1.0f;

    int start_color_r = 255, start_color_g = 255, start_color_b = 255, start_color_a = 255;

    std::string image = "";

    //testsuite #2
    float start_speed_min = 0.0f;
    float start_speed_max = 0.0f;
    float rotation_speed_min = 0.0f;
    float rotation_speed_max = 0.0f;

    float gravity_scale_x = 0.0f;
    float gravity_scale_y = 0.0f;
    float drag_factor = 1.0f;
    float angular_drag_factor = 1.0f;
    float linear_damping = 0.0f;
    float angular_damping = 0.0f;
    float end_scale = -1.0f; // dont use unless specified
    int end_color_r = -1, end_color_g = -1, end_color_b = -1, end_color_a = -1; // dont use unless specified

    float x = 0.0f;
    float y = 0.0f;
    int local_frame_number = 0;
    int frames_between_bursts = 1;
    int burst_quantity = 1;

    int sorting_order = 9999;
    int duration_frames = 300;

    int allocateParticleSlot() {
        if (!free_list.empty()) {
            int index = free_list.front();
            free_list.pop();
            return index;
        }

        is_active.push_back(false);
        start_frame.push_back(0);
        particle_bodies.push_back(nullptr);
        particle_x.push_back(0.0f);
        particle_y.push_back(0.0f);
        particle_vx.push_back(0.0f);
        particle_vy.push_back(0.0f);
        particle_rotation.push_back(0.0f);
        particle_angular_velocity.push_back(0.0f);
        particle_start_scale.push_back(1.0f);
        return static_cast<int>(is_active.size() - 1);
    }

    void destroyParticleBody(int index) {
        if (index < 0 || index >= static_cast<int>(particle_bodies.size())) {
            return;
        }
        b2Body* body = particle_bodies[index];
        if (!body) {
            return;
        }
        b2World* world = body->GetWorld();
        if (world) {
            world->DestroyBody(body);
        }
        particle_bodies[index] = nullptr;
    }


    void setImageManager(ImageManager* manager) {
        image_manager = manager;
    }

    void OnStart() {
        //validates all the value...
        if (frames_between_bursts < 1) frames_between_bursts = 1;
        if (burst_quantity < 1) burst_quantity = 1;
        if (duration_frames < 1) duration_frames = 1;
        if (drag_factor < 0.0f) drag_factor = 0.0f;
        if (angular_drag_factor < 0.0f) angular_drag_factor = 0.0f;
        if (linear_damping < 0.0f) linear_damping = 0.0f;
        if (angular_damping < 0.0f) angular_damping = 0.0f;
        if (start_color_r > 255 || start_color_r < 0) start_color_r = 255;
        if (start_color_g > 255 || start_color_g < 0) start_color_g = 255;
        if (start_color_b > 255 || start_color_b < 0) start_color_b = 255;
        if (start_color_a > 255 || start_color_a < 0) start_color_a = 255;
        if (end_color_r != -1 && (end_color_r > 255 || end_color_r < 0)) end_color_r = 255;
        if (end_color_g != -1 && (end_color_g > 255 || end_color_g < 0)) end_color_g = 255;
        if (end_color_b != -1 && (end_color_b > 255 || end_color_b < 0)) end_color_b = 255;
        if (end_color_a != -1 && (end_color_a > 255 || end_color_a < 0)) end_color_a = 255;

        // Initialize the random engine with as specific range and seed.
        emit_angle_distribution = new RandomEngine(emit_angle_min, emit_angle_max, 298); // Replace with the correct seed value from the assignment spec.
        emit_radius_distribution = new RandomEngine(emit_radius_min, emit_radius_max, 404); // Replace with the correct seed value from the assignment spec.
        rotation_distribution = new RandomEngine(rotation_min, rotation_max, 440); //
        scale_distribution = new RandomEngine(start_scale_min, start_scale_max, 494); //
        speed_distribution = new RandomEngine(start_speed_min, start_speed_max, 498); //
        rotation_speed_distribution = new RandomEngine(rotation_speed_min, rotation_speed_max, 305); //

        if (image_manager) {
            // Empty image key maps to ImageManager's built-in default particle texture.
            image_manager->loadImage(image);
        }

    }

    void generateNewParticles(){
        if (!image_manager) {
            return;
        }

        for (int i = 0; i < burst_quantity; i++) {
            const float angle_radians = glm::radians(emit_angle_distribution->Sample());
            const float radius = emit_radius_distribution->Sample();
            const float speed = speed_distribution->Sample();

            const float cos_angle = glm::cos(angle_radians);
            const float sin_angle = glm::sin(angle_radians);

            const float spawn_x = x + cos_angle * radius;
            const float spawn_y = y + sin_angle * radius;

            const float start_scale = scale_distribution->Sample();
            const float start_rotation = rotation_distribution->Sample();
            const float start_rotation_speed = rotation_speed_distribution->Sample();
            const float start_vx = cos_angle * speed;
            const float start_vy = sin_angle * speed;

            const int index = allocateParticleSlot();
            is_active[index] = true;
            start_frame[index] = local_frame_number;
            particle_x[index] = spawn_x;
            particle_y[index] = spawn_y;
            particle_vx[index] = start_vx;
            particle_vy[index] = start_vy;
            particle_rotation[index] = start_rotation;
            particle_angular_velocity[index] = start_rotation_speed;
            particle_start_scale[index] = start_scale;

            destroyParticleBody(index);
            particle_bodies[index] = nullptr;
        }
    }

    void OnUpdate(){
        const bool should_burst = (local_frame_number % frames_between_bursts == 0);

        for (int i = 0; i < static_cast<int>(is_active.size()); i++) {
            // 1) Skip inactive particles.
            if (!is_active[i]) {
                continue;
            }

            const int life_frames = local_frame_number - start_frame[i];
            if (life_frames >= duration_frames) {
                is_active[i] = false;
                free_list.push(i);
                pending_delete.push(i);
                continue;
            }

            b2Vec2 velocity(particle_vx[i], particle_vy[i]);
            float angular_velocity = particle_angular_velocity[i];

            // 2) Apply gravity to velocity.
            velocity.x += gravity_scale_x;
            velocity.y += gravity_scale_y;

            // 3) Apply drag and angular drag.
            velocity *= drag_factor;
            angular_velocity *= angular_drag_factor;

            // 4) Apply velocities to position and rotation.
            particle_x[i] += velocity.x;
            particle_y[i] += velocity.y;
            particle_rotation[i] += angular_velocity;

            particle_vx[i] = velocity.x;
            particle_vy[i] = velocity.y;
            particle_angular_velocity[i] = angular_velocity;

            // 5) Process color/scale over lifetime.
            const float lifetime_progress = std::clamp(static_cast<float>(life_frames) / static_cast<float>(duration_frames), 0.0f, 1.0f);
            const float scale = (end_scale < 0.0f)
                ? particle_start_scale[i]
                : glm::mix(particle_start_scale[i], end_scale, lifetime_progress);

            const int r = (end_color_r < 0)
                ? start_color_r
                : static_cast<int>(glm::mix(static_cast<float>(start_color_r), static_cast<float>(end_color_r), lifetime_progress));
            const int g = (end_color_g < 0)
                ? start_color_g
                : static_cast<int>(glm::mix(static_cast<float>(start_color_g), static_cast<float>(end_color_g), lifetime_progress));
            const int b = (end_color_b < 0)
                ? start_color_b
                : static_cast<int>(glm::mix(static_cast<float>(start_color_b), static_cast<float>(end_color_b), lifetime_progress));
            const int a = (end_color_a < 0)
                ? start_color_a
                : static_cast<int>(glm::mix(static_cast<float>(start_color_a), static_cast<float>(end_color_a), lifetime_progress));

            image_manager->pushDrawEx(
                image,
                particle_x[i],
                particle_y[i],
                static_cast<int>(particle_rotation[i]),
                scale,
                scale,
                0.5f,
                0.5f,
                r,
                g,
                b,
                a,
                sorting_order
            );
        }

        if (should_burst) {
            generateNewParticles();
        }

        local_frame_number++;
    }

    void OnLateUpdate() {
        while (!pending_delete.empty()) {
            const int index = pending_delete.front();
            pending_delete.pop();

            destroyParticleBody(index);
            free_list.push(index);
        }
    }

    void OnDestroy() {
        while (!free_list.empty()) {
            free_list.pop();
        }

        while (!pending_delete.empty()) {
            pending_delete.pop();
        }

        for (int i = 0; i < static_cast<int>(particle_bodies.size()); i++) {
            destroyParticleBody(i);
        }

        delete emit_angle_distribution;
        emit_angle_distribution = nullptr;
        delete emit_radius_distribution;
        emit_radius_distribution = nullptr;
        delete rotation_distribution;
        rotation_distribution = nullptr;
        delete scale_distribution;
        scale_distribution = nullptr;
        delete speed_distribution;
        speed_distribution = nullptr;
        delete rotation_speed_distribution;
        rotation_speed_distribution = nullptr;
    }

};