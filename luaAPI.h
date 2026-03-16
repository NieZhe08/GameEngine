#ifndef LUAAPI_H
#define LUAAPI_H

#include <iostream>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "game_utils.h"
#include "ActorManager.h"
#include "Helper.h"
#include "input_manager.h"
#include <thread>
#include <chrono>
#include "cameraManager.h"

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
            std::cerr << '\n';
            return;
        }

        try {
            std::cerr << message.tostring() << std::endl;
        } catch (...) {
            std::cerr << "<unprintable Lua value>" << std::endl;
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
    // 立刻退出程序（供 Lua 调用）
    static void Quit() {
        std::exit(0);
    }

    // 睡眠指定毫秒数
    static void Sleep(int milliseconds) {
        if (milliseconds <= 0) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    // 获取当前帧号（由 Helper 维护）
    static int GetFrame() {
        return Helper::GetFrameNumber();
    }

    // 打开指定 URL
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
    ActorManager* m_actorManager;
    ComponentDB* m_componentDB;
public:
    ActorAPI(ActorManager* actorManager, ComponentDB* componentDB): m_actorManager(actorManager), m_componentDB(componentDB) {}

    void RegisterLuaAPI(lua_State* L) {
        ActorManager* actorManager = m_actorManager;
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
            // 使用命名空间 Actor 暴露全局查找函数：Lua 写法为 Actor.Find / Actor.FindAll
            .beginNamespace("Actor")
                .addFunction("Find", std::function<luabridge::LuaRef(const std::string&)>([actorManager](const std::string& name) -> luabridge::LuaRef { return actorManager->Find(name); }))
                .addFunction("FindAll", std::function<luabridge::LuaRef(const std::string&)>([actorManager](const std::string& name) -> luabridge::LuaRef { return actorManager->FindAll(name); }))
                .addFunction("Instantiate", std::function<Actor*(const std::string&, ComponentDB*)>([actorManager](const std::string& name, ComponentDB* componentDB) -> Actor* { return actorManager->CreateActor(name, componentDB); }))
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
    TextManager* m_textManager;
    public:
        TextAPI(TextManager* textManager): m_textManager(textManager) {}

        void RegisterLuaAPI(lua_State* L) {
            luabridge::getGlobalNamespace(L)
                .beginNamespace("Text")
                    .addFunction("Draw", std::function<void(const std::string&, int, int, const std::string&, int, int, int, int, int)>([this](const std::string& text, int x, int y, const std::string& font_name, int font_size, int r, int g, int b, int a) {
                        m_textManager->addText(text, x, y, font_name, font_size, r, g, b, a);
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
    ImageManager* m_imageManager;
public:
    ImageAPI(ImageManager* imageManager): m_imageManager(imageManager) {}
    
    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Image")
                .addFunction("DrawUI", std::function<void(const std::string&, float, float)>([this](const std::string& image_name, float x, float y) {
                    m_imageManager->pushDrawUI(image_name, x, y);
                }))
                .addFunction("DrawUIEx", std::function<void(const std::string&, float, float, int, int, int, int, int)>([this](const std::string& image_name, float x, float y, int r, int g, int b, int a, int sorting_order) {
                    m_imageManager->pushDrawUIEx(image_name, x, y, r, g, b, a, sorting_order);
                }))
                .addFunction("Draw", std::function<void(const std::string&, float, float)>([this](const std::string& image_name, float x, float y) {
                    m_imageManager->pushDraw(image_name, x, y);
                }))
                .addFunction("DrawEx", std::function<void(const std::string&, float, float, int, float, float, float, float, int, int, int, int, int)>([this](const std::string& image_name, float x, float y, int rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, int r, int g, int b, int a, int sorting_order) {
                    m_imageManager->pushDrawEx(image_name, x, y, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order);
                }))
                .addFunction("DrawPixel", std::function<void(int, int, int, int, int, int)>([this](int x, int y, int r, int g, int b, int a) {
                    m_imageManager->pushDrawPixel(x, y, r, g, b, a);
                }))
            .endNamespace();
    }
};

class CameraAPI {
    CameraManager* m_cameraManager;

public:
    CameraAPI(CameraManager* cameraManager): m_cameraManager(cameraManager) {}

    void RegisterLuaAPI(lua_State* L) {
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Camera")
                .addFunction("SetPosition", std::function<void(float, float)>([this](float x, float y) {
                    m_cameraManager->setPosition(x, y);
                }))
                .addFunction("SetZoom", std::function<void(float)>([this](float zoom_factor) {
                    m_cameraManager->setZoom(zoom_factor);
                }))
                .addFunction("GetPositionX", std::function<float()>([this]() {
                    return m_cameraManager->getPositionX();
                }))
                .addFunction("GetPositionY", std::function<float()>([this]() {
                    return m_cameraManager->getPositionY();
                }))
                .addFunction("GetZoom", std::function<float()>([this]() {
                    return m_cameraManager->getZoom();
                }))
            .endNamespace();
    }
};

#endif