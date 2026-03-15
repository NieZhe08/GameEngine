#ifndef TEXT_DB_H
#define TEXT_DB_H

#include <iostream>
#include <filesystem>
#include <vector>
#include <string.h>
#include "Helper.h"
#include "SDL2/SDL.h"
#include "SDL_ttf/SDL_ttf.h"
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "unordered_map"
#include <queue>

struct TextInfo {
    std::string text;
    int x, y;
    std::string font_name;
    int font_size;
    int r, g, b, a;

    TextInfo(const std::string& text, int x, int y, const std::string& font_name, int font_size, int r, int g, int b, int a) 
        : text(text), x(x), y(y), font_name(font_name), font_size(font_size), r(r), g(g), b(b), a(a) {}
};

class TextManager{
private:
    //std::unordered_map<std::string, CachedText> text_cache;
    std::queue<TextInfo> text_queue; // Queue to store text rendering requests
    std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> font_cache;
    SDL_Renderer* renderer_;

    void loadFont(const std::string& font_name, int font_size) {
        std::string font_path = "resources/fonts/" + font_name + ".ttf";
        if (!std::filesystem::exists(font_path)){
            std::cout<<"error: font "<<font_name<<" missing";// no newline at end
            exit(0);
        }
        TTF_Font* font = TTF_OpenFont(font_path.c_str(), font_size);
        if (!font) {
            std::cout << "Failed to load font: " << TTF_GetError() << std::endl;// should not happen
            exit(0);
        }
        font_cache[font_name][font_size] = font;
    }
public:

    TextManager(SDL_Renderer* renderer) : renderer_(renderer) {}

    ~TextManager() {
        for (auto& font_entry : font_cache) {
            for (auto& size_entry : font_entry.second) {
                if (size_entry.second) {
                    TTF_CloseFont(size_entry.second);
                }
            }
        }
    }
    
    void addText(const std::string& text, int x, int y, const std::string& font_name, int font_size, int r, int g, int b, int a) {
        text_queue.emplace(text, x, y, font_name, font_size, r, g, b, a);
    }

    void renderAllText(){
        while (!text_queue.empty()) {
            TextInfo info = text_queue.front();
            text_queue.pop();

            // Load font if not already loaded
            auto font_it = font_cache.find(info.font_name);
            if (font_it == font_cache.end() || font_it->second.find(info.font_size) == font_it->second.end()) {
                loadFont(info.font_name, info.font_size);
                font_it = font_cache.find(info.font_name);
            }
            TTF_Font* font = font_it->second[info.font_size];

            // Render text to surface
            SDL_Color color = { static_cast<Uint8>(info.r), static_cast<Uint8>(info.g), static_cast<Uint8>(info.b), static_cast<Uint8>(info.a) };
            SDL_Surface* surface = TTF_RenderText_Blended(font, info.text.c_str(), color);
            if (!surface) {
                std::cout << "Failed to create text surface: " << TTF_GetError() << std::endl;
                continue;
            }

            // Create texture from surface
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                std::cout << "Failed to create text texture: " << SDL_GetError() << std::endl;
                continue;
            }

            // Get texture dimensions
            float width = 0.0f;
            float height = 0.0f;
            Helper::SDL_QueryTexture(texture, &width, &height);

            // Render texture to screen
            SDL_FRect dstRect = { static_cast<float>(info.x), static_cast<float>(info.y), width, height };
            Helper::SDL_RenderCopy(renderer_, texture, nullptr, &dstRect);

            // Clean up texture
            SDL_DestroyTexture(texture);
        }
    }

};

/* class TextDB {
    rapidjson::Document game;
    rapidjson::Document rendering;
    std::unique_ptr<std::vector<std::string>> intro_text;
    SDL_Renderer* renderer_;
    std::unordered_map<std::string, int> text_cache_map; // Map from text content to cache index
    std::vector<CachedText> text_cache; // Cache of rendered text textures
    
public:
    TTF_Font* font;
    bool do_text_rendering;

public:
    TextDB(SDL_Renderer* renderer, bool _do_text_rendering) : renderer_(renderer), do_text_rendering(_do_text_rendering){
    //std::cout << "TextDB constructor called with do_text_rendering = " << do_text_rendering << std::endl;
        if (!std::filesystem::exists("resources/")){
            std::cout<<"error: resources/ missing"; // no newline at end
            exit(0);
        }
        if (!std::filesystem::exists("resources/game.config")){
            std::cout<<"error: resources/game.config missing";// no newline at end
            exit(0);
        }
        EngineUtils::ReadJsonFile("resources/game.config", game);
    }

    ~TextDB() {
        // Clean up cached textures
        for (auto& cached : text_cache) {
            if (cached.texture) {
                SDL_DestroyTexture(cached.texture);
            }
        }
        text_cache.clear();
        text_cache_map.clear();
    }

    void readIntroText(){
        //std::cout << "TextDB::readIntroText called, do_text_rendering"<<do_text_rendering<<"\n";
        if (do_text_rendering){
            intro_text = std::make_unique<std::vector<std::string>>();
            //std::cout << "Reading intro text from game.config\n";
            if (game.HasMember("intro_text") && game["intro_text"].IsArray()){
                const rapidjson::Value& text = game["intro_text"];
                //std::cout << "Found intro_text with " << text.Size() << " entries.\n";
                for (rapidjson::SizeType i = 0; i < text.Size(); i++){
                    //std::cout << "Adding intro text: " << text[i].GetString() << std::endl;
                    intro_text->emplace_back(text[i].GetString());
                }

                if (game.HasMember("font")){
                    std::string font_path = game["font"].GetString();
                    if (!std::filesystem::exists("resources/fonts/" + font_path + ".ttf")){
                        std::cout<<"error: font "<<font_path<<" missing";// no newline at end
                        exit(0);
                    }
                    font = TTF_OpenFont(("resources/fonts/" + font_path + ".ttf").c_str(), 16);
                    if (!font) {
                        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;// should not happen
                        exit(0);
                    }
                } else {
                   std::cout << "error: text render failed. No font configured";
                    exit(0);
                }
            } else {
                //std::cout << "No intro_text found in game.config or intro_text is not an array.\n";
            }
        }
        if (!intro_text || intro_text->empty()) {
            //std::cout << "No intro text found or intro text is empty. Text rendering will be disabled.\n";
            do_text_rendering = false;
        }
    }

    std::unique_ptr<std::vector<std::string>> getIntroTextVector(){
        if (intro_text) {
            return std::make_unique<std::vector<std::string>>(*intro_text);
        } else {
            return std::make_unique<std::vector<std::string>>();
        }
    }

    void loadFont(){
        if (game.IsObject()){
            if (game.HasMember("font")){
                std::string font_path = game["font"].GetString();
                if (!std::filesystem::exists("resources/fonts/" + font_path + ".ttf")){
                    std::cout<<"error: font "<<font_path<<" missing";// no newline at end
                    exit(0);
                }
                font = TTF_OpenFont(("resources/fonts/" + font_path + ".ttf").c_str(), 16);
                if (!font) {
                    std::cout << "Failed to load font: " << TTF_GetError() << std::endl;// should not happen
                    exit(0);
                }
                do_text_rendering = true;
            } else {
                std::cout << "error: text render failed. No font configured";
                exit(0);
            }
        } else {
            std::cout << "error: game.config is not an object. Cannot load font.";
            exit(0);
        }
    }

    void drawText(const std::string& text, float x, float y, SDL_Color color = {255, 255, 255, 255}) {
        if (!do_text_rendering) {
            //std::cout << "Text rendering is disabled. Skipping drawText call.\n";
            return;
        }
        if (!font) std::cout << "error: cannot draw text because font is not loaded\n";
        
        // Check if text is already in cache
        auto it = text_cache_map.find(text);
        SDL_Texture* texture = nullptr;
        int width = 0, height = 0;
        
        if (it != text_cache_map.end()) {
            // Text found in cache
            int cache_index = it->second;
            CachedText& cached = text_cache[cache_index];
            texture = cached.texture;
            width = cached.width;
            height = cached.height;
        } else {
            // Text not in cache, create and cache it
            SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
            if (!surface) {
                std::cout << "Failed to create text surface: " << TTF_GetError() << std::endl; // should not happen
                return;
            }
            texture = SDL_CreateTextureFromSurface(renderer_, surface);
            if (!texture) {
                std::cout << "Failed to create text texture: " << SDL_GetError() << std::endl; // should not happen
                SDL_FreeSurface(surface);
                return;
            }
            
            width = surface->w;
            height = surface->h;
            SDL_FreeSurface(surface);
            
            // Add to cache
            CachedText cached_text = {texture, width, height};
            text_cache.push_back(cached_text);
            text_cache_map[text] = text_cache.size() - 1;
        }
        
        // Render the texture
        SDL_FRect dstRect = { x, y, static_cast<float>(width), static_cast<float>(height) };
        Helper::SDL_RenderCopy(renderer_, texture, nullptr, &dstRect);
        //std::cout << "[drawText] finished rendering text\n";
    }

};

class TextRenderConfig {
public:
    std::string text;
    int x;
    int y;
    TextRenderConfig(const std::string& text, int x, int y) : text(text), x(x), y(y) {}       
};
 */
#endif // TEXT_DB_H
