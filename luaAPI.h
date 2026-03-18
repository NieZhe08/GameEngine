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
#include <memory>
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
            // 使用命名空间 Actor 暴露全局查找函数：Lua 写法为 Actor.Find / Actor.FindAll
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

public:
    explicit CameraAPI(const std::shared_ptr<CameraManager>& cameraManager): m_cameraManager(cameraManager) {}

    void RegisterLuaAPI(lua_State* L) {
        auto cameraManager = m_cameraManager;
        luabridge::getGlobalNamespace(L)
            .beginNamespace("Camera")
                .addFunction("SetPosition", std::function<void(float, float)>([cameraManager](float x, float y) {
                    cameraManager->setPosition(x, y);
                }))
                .addFunction("SetZoom", std::function<void(float)>([cameraManager](float zoom_factor) {
                    cameraManager->setZoom(zoom_factor);
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

#endif