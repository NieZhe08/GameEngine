#pragma once

#include <iostream>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "game_utils.h"
#include "ActorManager.h"
#include "Helper.h"
#include "input_manager.h"
#include <thread>
#include <chrono>
#include <memory>
#include "cameraManager.h"
#include "box2d/box2d.h"
#include "rigidBody.h"
#include "ParticleSystem.h"
#include "raycast.h"
#include "EventBus.h"

// Debug API 
class Debug {
public:
    static void CppLog(const luabridge::LuaRef& message) {
        if (message.isNil()) {
            std::cout  << '\n';
            return;
        }

        try {
            std::cout << message.tostring() << std::endl;
        } catch (...) {
            std::cout << "<unprintable Lua value>" << std::endl;
        }
    }

    static void CppLogError(const luabridge::LuaRef& message) {
        if (message.isNil()) {
            std::cout << '\n';
            return;
        }

        try {
            std::cout << message.tostring() << std::endl;
        } catch (...) {
            std::cout << "<unprintable Lua value>" << std::endl;
        }
    }

    void RegisterLuaAPI(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("Debug")
            .addFunction("Log", CppLog)
            .addFunction("LogError", CppLogError)
        .endNamespace();
    }
};

// Application API
class ApplicationAPI {
public:
    static void Quit() {
        std::exit(0);
    }

    static void Sleep(int milliseconds) {
        if (milliseconds <= 0) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    static int GetFrame() {
        return Helper::GetFrameNumber();
    }

    static void OpenURL(const std::string& url) {
        if (url.empty()) return;

#if defined(_WIN32)
        std::string cmd = "start " + url;
#elif defined(__APPLE__)
        std::string cmd = "open '" + url + "'";
#else
        std::string cmd = "xdg-open '" + url + "'";
#endif
        std::system(cmd.c_str());
    }

    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Application")
                .addFunction("Quit", &ApplicationAPI::Quit)
                .addFunction("Sleep", &ApplicationAPI::Sleep)
                .addFunction("GetFrame", &ApplicationAPI::GetFrame)
                .addFunction("OpenURL", &ApplicationAPI::OpenURL)
            .endNamespace();
    }
};


// Actor API 
class ActorAPI {
private:
    std::shared_ptr<ActorManager> m_actorManager;
    std::shared_ptr<ComponentDB> m_componentDB;
public:
    ActorAPI(const std::shared_ptr<ActorManager>& actorManager,
             const std::shared_ptr<ComponentDB>& componentDB)
        : m_actorManager(actorManager), m_componentDB(componentDB) {}

    void RegisterLuaAPI(lua_State* L) {
        ActorManager* actorManager = m_actorManager.get();
        std::shared_ptr<ComponentDB> componentDB = m_componentDB;
        luabridge::getGlobalNamespace(L)
            .beginClass<Actor>("Actor")
                .addFunction("GetName", &Actor::GetName)
                .addFunction("GetID", &Actor::GetID)
                .addFunction("GetComponent", &Actor::GetComponent)
                .addFunction("GetComponents", &Actor::GetComponents)
                .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
                .addFunction("AddComponent", &Actor::AddComponent)
                .addFunction("RemoveComponent", &Actor::RemoveComponent)
            .endClass()
            .beginNamespace("Actor")
                .addFunction("Find", std::function<luabridge::LuaRef(const std::string&)>([actorManager](const std::string& name) -> luabridge::LuaRef { return actorManager->Find(name); }))
                .addFunction("FindAll", std::function<luabridge::LuaRef(const std::string&)>([actorManager](const std::string& name) -> luabridge::LuaRef { return actorManager->FindAll(name); }))
                .addFunction("Instantiate", std::function<Actor*(const std::string&)>([actorManager, componentDB](const std::string& templateName) -> Actor* {
                    TemplateDB templateDB(templateName);
                    return actorManager->Instantiate(&templateDB, componentDB);
                }))
                .addFunction("Destroy", std::function<void(Actor*)>([actorManager](Actor* actor) { actorManager->Destroy(actor); }))
            .endNamespace();
    }
};

// Input API
class InputAPI {
public:

    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginClass<glm::vec2>("vec2")
                .addData("x", &glm::vec2::x)
                .addData("y", &glm::vec2::y)
            .endClass()
            .beginNamespace("Input")
                .addFunction("GetKey", &Input::GetKey)
                .addFunction("GetKeyDown", &Input::GetKeyDown)
                .addFunction("GetKeyUp", &Input::GetKeyUp)
                .addFunction("GetMousePosition", &Input::GetMousePosition)
                .addFunction("GetMouseButton", &Input::GetMouseButton)
                .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
                .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
                .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
                .addFunction("HideCursor", &Input::HideCursor)
                .addFunction("ShowCursor", &Input::ShowCursor)
            .endNamespace();
    }
};

class TextAPI {
    std::shared_ptr<TextManager> m_textManager;
    public:
        explicit TextAPI(const std::shared_ptr<TextManager>& textManager): m_textManager(textManager) {}

        void RegisterLuaAPI(lua_State* L) {
            auto textManager = m_textManager;
            luabridge::getGlobalNamespace(L)
                .beginNamespace("Text")
                    .addFunction("Draw", std::function<void(const std::string&, float, float, const std::string&, float, float, float, float, float)>([textManager](const std::string& text, float x, float y, const std::string& font_name, float font_size, float r, float g, float b, float a) {
                        textManager->addText(
                            text,
                            static_cast<int>(x),
                            static_cast<int>(y),
                            font_name,
                            static_cast<int>(font_size),
                            static_cast<int>(r),
                            static_cast<int>(g),
                            static_cast<int>(b),
                            static_cast<int>(a)
                        );
                    }))
                .endNamespace();
        }
};

class AudioAPI {
public:
    static void Play(int channel, const std::string& clip_name, bool does_loop) {
        int audio_number = AudioManager::loadAudio(clip_name);
        if (audio_number == -1) {
            std::cout << "error: failed to play audio clip " << clip_name << "\n";
            return;
        }
        AudioManager::PlayChannel(channel, audio_number, does_loop);
    }

    static void Halt(int channel) {
        AudioManager::HaltChannel(channel);
    }

    static void SetVolume(int channel, float volume) {
        AudioManager::SetVolume(channel, static_cast<int>(volume));
    }

    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Audio")
                .addFunction("Play", &AudioAPI::Play)
                .addFunction("Halt", &AudioAPI::Halt)
                .addFunction("SetVolume", &AudioAPI::SetVolume)
            .endNamespace();
    }
};

class ImageAPI {
    std::shared_ptr<ImageManager> m_imageManager;
public:
    explicit ImageAPI(const std::shared_ptr<ImageManager>& imageManager): m_imageManager(imageManager) {}
    
    void RegisterLuaAPI(lua_State* L) {
        auto imageManager = m_imageManager;
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Image")
                .addFunction("DrawUI", std::function<void(const std::string&, float, float)>([imageManager](const std::string& image_name, float x, float y) {
                    imageManager->pushDrawUI(image_name, x, y);
                }))
                .addFunction("DrawUIEx", std::function<void(const std::string&, float, float, float, float, float, float, float)>([imageManager](const std::string& image_name, float x, float y, float r, float g, float b, float a, float sorting_order) {
                    imageManager->pushDrawUIEx(
                        image_name,
                        x,
                        y,
                        static_cast<int>(r),
                        static_cast<int>(g),
                        static_cast<int>(b),
                        static_cast<int>(a),
                        static_cast<int>(sorting_order)
                    );
                }))
                .addFunction("Draw", std::function<void(const std::string&, float, float)>([imageManager](const std::string& image_name, float x, float y) {
                    imageManager->pushDraw(image_name, x, y);
                }))
                .addFunction("DrawEx", std::function<void(const std::string&, float, float, float, float, float, float, float, float, float, float, float, float)>([imageManager](const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order) {
                    imageManager->pushDrawEx(
                        image_name,
                        x,
                        y,
                        static_cast<int>(rotation_degrees),
                        scale_x,
                        scale_y,
                        pivot_x,
                        pivot_y,
                        static_cast<int>(r),
                        static_cast<int>(g),
                        static_cast<int>(b),
                        static_cast<int>(a),
                        static_cast<int>(sorting_order)
                    );
                }))
                .addFunction("DrawPixel", std::function<void(float, float, float, float, float, float)>([imageManager](float x, float y, float r, float g, float b, float a) {
                    imageManager->pushDrawPixel(
                        static_cast<int>(x),
                        static_cast<int>(y),
                        static_cast<int>(r),
                        static_cast<int>(g),
                        static_cast<int>(b),
                        static_cast<int>(a)
                    );
                }))
            .endNamespace();
    }
};

class CameraAPI {
    std::shared_ptr<CameraManager> m_cameraManager;
    std::shared_ptr<ImageManager> m_imageManager;

public:
    CameraAPI(const std::shared_ptr<CameraManager>& cameraManager,
              const std::shared_ptr<ImageManager>& imageManager)
        : m_cameraManager(cameraManager), m_imageManager(imageManager) {}

    void RegisterLuaAPI(lua_State* L) {
        auto cameraManager = m_cameraManager;
        auto imageManager = m_imageManager;
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Camera")
                .addFunction("SetPosition", std::function<void(float, float)>([cameraManager, imageManager](float x, float y) {
                    cameraManager->setPosition(x, y);
                    if (imageManager) {
                        imageManager->setCameraState(
                            glm::vec2(cameraManager->getPositionX(), cameraManager->getPositionY()),
                            cameraManager->getZoom()
                        );
                    }
                }))
                .addFunction("SetZoom", std::function<void(float)>([cameraManager, imageManager](float zoom_factor) {
                    cameraManager->setZoom(zoom_factor);
                    if (imageManager) {
                        imageManager->setCameraState(
                            glm::vec2(cameraManager->getPositionX(), cameraManager->getPositionY()),
                            cameraManager->getZoom()
                        );
                    }
                }))
                .addFunction("GetPositionX", std::function<float()>([cameraManager]() {
                    return cameraManager->getPositionX();
                }))
                .addFunction("GetPositionY", std::function<float()>([cameraManager]() {
                    return cameraManager->getPositionY();
                }))
                .addFunction("GetZoom", std::function<float()>([cameraManager]() {
                    return cameraManager->getZoom();
                }))
            .endNamespace();
    }
};

class SceneAPI {
    std::shared_ptr<ActorManager> m_actorManager;
    std::string* m_nextSceneName;
    std::string* m_currentSceneName;

public:
    SceneAPI(const std::shared_ptr<ActorManager>& actorManager,
             std::string* nextSceneName,
             std::string* currentSceneName)
        : m_actorManager(actorManager),
          m_nextSceneName(nextSceneName),
          m_currentSceneName(currentSceneName) {}

    void RegisterLuaAPI(lua_State* L) {
        auto actorManager = m_actorManager;
        std::string* nextSceneName = m_nextSceneName;
        std::string* currentSceneName = m_currentSceneName;

        luabridge::getGlobalNamespace(L)
            .beginNamespace("Scene")
                .addFunction("Load", std::function<void(const std::string&)>([nextSceneName](const std::string& sceneName) {
                    if (!nextSceneName) return;
                    *nextSceneName = sceneName;
                }))
                .addFunction("GetCurrent", std::function<std::string()>([currentSceneName]() -> std::string {
                    if (!currentSceneName) return "";
                    return *currentSceneName;
                }))
                .addFunction("DontDestroy", std::function<void(Actor*)>([actorManager](Actor* actor) {
                    if (!actorManager || !actor) return;
                    actor->RemainWhenSceneChange();
                }))
            .endNamespace();
    }
};

class EventLuaAPI {
public:
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Event")
                .addFunction("Publish", &EventBus::Publish)
                .addFunction("Subscribe", &EventBus::Subscribe)
                .addFunction("Unsubscribe", &EventBus::Unsubscribe)
            .endNamespace();
    }
};

class Vector2API {
public:
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginClass<b2Vec2>("Vector2")
            .addConstructor<void(*)(float, float)>()
            .addProperty("x", &b2Vec2::x)
            .addProperty("y", &b2Vec2::y)
            .addFunction("Normalize", &b2Vec2::Normalize)
            .addFunction("Length", &b2Vec2::Length)
            .addFunction("__add", &b2Vec2::operator_add)
            .addFunction("__sub", &b2Vec2::operator_sub)
            .addFunction("__mul", &b2Vec2::operator_mul)
            .addStaticFunction("Distance", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Distance))
            .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
            .endClass();
        }
};

class RigidbodyAPI {
public:
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginClass<Rigidbody>("Rigidbody")
            .addData("enabled", &Rigidbody::enabled)
            .addData("do_destroy", &Rigidbody::do_destroy)
            .addData("key", &Rigidbody::key)
            .addData("type", &Rigidbody::type)
            .addData("actor", &Rigidbody::actor)
            .addData("x", &Rigidbody::x)
            .addData("y", &Rigidbody::y)
            .addData("body_type", &Rigidbody::body_type)
            .addData("precise", &Rigidbody::precise)
            .addData("gravity_scale", &Rigidbody::gravity_scale)
            .addData("density", &Rigidbody::density)
            .addData("angular_friction", &Rigidbody::angular_friction)
            .addData("rotation", &Rigidbody::rotation)
            .addData("has_collider", &Rigidbody::has_collider)
            .addData("has_trigger", &Rigidbody::has_trigger)

            .addData("collider_type", &Rigidbody::collider_type)
            .addData("width", &Rigidbody::width)
            .addData("height", &Rigidbody::height)
            .addData("radius", &Rigidbody::radius)
            .addData("friction", &Rigidbody::friction)
            .addData("bounciness", &Rigidbody::bounciness)

            .addData("trigger_type", &Rigidbody::trigger_type)
            .addData("trigger_width", &Rigidbody::trigger_width)
            .addData("trigger_height", &Rigidbody::trigger_height)
            .addData("trigger_radius", &Rigidbody::trigger_radius)

            .addFunction("GetPosition", &Rigidbody::GetPosition)
            .addFunction("GetRotation", &Rigidbody::GetRotation)
            .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
            .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
            .addFunction("OnStart", &Rigidbody::OnStart)
            .addFunction("OnUpdate", &Rigidbody::OnUpdate)
            .addFunction("OnLateUpdate", &Rigidbody::OnLateUpdate)
            .addFunction("OnDestroy", &Rigidbody::OnDestroy)

            .addFunction("AddForce", &Rigidbody::AddForce)
            .addFunction("SetVelocity", &Rigidbody::SetVelocity)
            .addFunction("SetPosition", &Rigidbody::SetPosition)
            .addFunction("SetRotation", &Rigidbody::SetRotation)
            .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
            .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
            .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
            .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
            .addFunction("GetVelocity", &Rigidbody::GetVelocity)
            .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
            .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
            .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
            .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)

            .endClass();
    }
};

class ParticleSystemAPI {
    ImageManager* m_imageManager = nullptr;

public:
    explicit ParticleSystemAPI(ImageManager* imageManager)
        : m_imageManager(imageManager) {}

    void RegisterLuaAPI(lua_State* L) {
        ImageManager* imageManager = m_imageManager;
        luabridge::getGlobalNamespace(L)
            .beginClass<ParticleSystem>("ParticleSystem")
            .addData("enabled", &ParticleSystem::enabled)
            .addData("do_destroy", &ParticleSystem::do_destroy)
            .addData("key", &ParticleSystem::key)
            .addData("type", &ParticleSystem::type)
            .addData("actor", &ParticleSystem::actor)

            .addData("emit_angle_min", &ParticleSystem::emit_angle_min)
            .addData("emit_angle_max", &ParticleSystem::emit_angle_max)
            .addData("emit_radius_min", &ParticleSystem::emit_radius_min)
            .addData("emit_radius_max", &ParticleSystem::emit_radius_max)
            .addData("rotation_min", &ParticleSystem::rotation_min)
            .addData("rotation_max", &ParticleSystem::rotation_max)
            .addData("start_scale_min", &ParticleSystem::start_scale_min)
            .addData("start_scale_max", &ParticleSystem::start_scale_max)
            .addData("start_color_r", &ParticleSystem::start_color_r)
            .addData("start_color_g", &ParticleSystem::start_color_g)
            .addData("start_color_b", &ParticleSystem::start_color_b)
            .addData("start_color_a", &ParticleSystem::start_color_a)
            .addData("image", &ParticleSystem::image)
            .addData("start_speed_min", &ParticleSystem::start_speed_min)
            .addData("start_speed_max", &ParticleSystem::start_speed_max)
            .addData("rotation_speed_min", &ParticleSystem::rotation_speed_min)
            .addData("rotation_speed_max", &ParticleSystem::rotation_speed_max)
            .addData("x", &ParticleSystem::x)
            .addData("y", &ParticleSystem::y)
            .addData("frames_between_bursts", &ParticleSystem::frames_between_bursts)
            .addData("burst_quantity", &ParticleSystem::burst_quantity)
            .addData("sorting_order", &ParticleSystem::sorting_order)
            .addData("duration_frames", &ParticleSystem::duration_frames)
            .addData("gravity_scale_x", &ParticleSystem::gravity_scale_x)
            .addData("gravity_scale_y", &ParticleSystem::gravity_scale_y)
            .addData("drag_factor", &ParticleSystem::drag_factor)
            .addData("angular_drag_factor", &ParticleSystem::angular_drag_factor)
            .addData("linear_damping", &ParticleSystem::linear_damping)
            .addData("angular_damping", &ParticleSystem::angular_damping)
            .addData("end_scale", &ParticleSystem::end_scale)
            .addData("end_color_r", &ParticleSystem::end_color_r)
            .addData("end_color_g", &ParticleSystem::end_color_g)
            .addData("end_color_b", &ParticleSystem::end_color_b)
            .addData("end_color_a", &ParticleSystem::end_color_a)
            .addData("enable_burst", &ParticleSystem::enable_burst)

            .addFunction("OnStart", &ParticleSystem::OnStart)
            .addFunction("OnUpdate", &ParticleSystem::OnUpdate)
            .addFunction("OnLateUpdate", &ParticleSystem::OnLateUpdate)
            .addFunction("OnDestroy", &ParticleSystem::OnDestroy)
            .addFunction("Stop", &ParticleSystem::Stop)
            .addFunction("Play", &ParticleSystem::Play)
            .addFunction("Burst", &ParticleSystem::Burst)
            .addFunction("SetImageManager", std::function<void(ParticleSystem*)>([imageManager](ParticleSystem* ps) {
                if (!ps) return;
                ps->setImageManager(imageManager);
            }))
            .endClass();
    }
};

class PhysicsAPI {
public:
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Physics")
            .addFunction("Raycast", std::function<luabridge::LuaRef(const b2Vec2&, const b2Vec2&, float)>([L](const b2Vec2& pos, const b2Vec2& dir, float dist) -> luabridge::LuaRef {
                return RayCastManager::Raycast(pos, dir, dist, L);
            }))
            .addFunction("RaycastAll", std::function<luabridge::LuaRef(const b2Vec2&, const b2Vec2&, float)>([L](const b2Vec2& pos, const b2Vec2& dir, float dist) -> luabridge::LuaRef {
                return RayCastManager::RaycastAll(pos, dir, dist, L);
             }))
            .endNamespace();
    }
};

class CollisionAPI {
public:
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginClass<Collision>("Collision")
            .addData("other", &Collision::other)
            .addData("point", &Collision::point)
            .addData("relative_velocity", &Collision::relative_velocity)
            .addData("normal", &Collision::normal)
            .endClass();
    }
};

class RayCastingAPI {
public:
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginClass<HitResult>("HitResult")
            .addData("actor", &HitResult::actor)
            .addData("point", &HitResult::point)
            .addData("normal", &HitResult::normal)
            .addData("is_trigger", &HitResult::is_trigger)
            .endClass();
    }
};