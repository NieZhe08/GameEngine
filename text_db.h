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
        if (text.empty()) {
            return;
        }
        text_queue.emplace(text, x, y, font_name, font_size, r, g, b, a);
    }

    void renderAllText(){
        while (!text_queue.empty()) {
            TextInfo info = text_queue.front();
            text_queue.pop();

            if (info.text.empty()) {
                continue;
            }

            // Load font if not already loaded
            auto font_it = font_cache.find(info.font_name);
            if (font_it == font_cache.end() || font_it->second.find(info.font_size) == font_it->second.end()) {
                loadFont(info.font_name, info.font_size);
                font_it = font_cache.find(info.font_name);
            }
            TTF_Font* font = font_it->second[info.font_size];

            // Render text to surface
            SDL_Color color = { static_cast<Uint8>(info.r), static_cast<Uint8>(info.g), static_cast<Uint8>(info.b), static_cast<Uint8>(info.a) };
            SDL_Surface* surface = TTF_RenderText_Solid(font, info.text.c_str(), color);
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
            Helper::SDL_RenderCopyEx(0, "", renderer_, texture, nullptr, &dstRect, 0, nullptr, SDL_FLIP_NONE);

            // Clean up texture
            SDL_DestroyTexture(texture);
        }
    }

};

#endif // TEXT_DB_H
