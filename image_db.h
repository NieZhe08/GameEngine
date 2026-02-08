#ifndef IMAGE_DB_H
#define IMAGE_DB_H

#include <iostream>
#include <filesystem>
#include <vector>
#include <string.h>
#include "Helper.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "unordered_map"

class ImageDB {
    rapidjson::Document game;
    rapidjson::Document rendering;
    std::unique_ptr<std::vector<std::string>> intro_image;
    SDL_Renderer* renderer_;
    std::unordered_map<std::string, int> image_index_map; // Map from image path to index in cache
    std::vector<SDL_Texture*> cache; // Cache of loaded textures

public:
    ImageDB(SDL_Renderer* renderer) : renderer_(renderer) {
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

    void readIntroImage(){
        intro_image = std::make_unique<std::vector<std::string>>();
        if (game.HasMember("intro_image") && game["intro_image"].IsArray()){
            const rapidjson::Value& image = game["intro_image"];
            for (rapidjson::SizeType i = 0; i < image.Size(); i++){
                intro_image->emplace_back(image[i].GetString());
            }
        }
        if (intro_image && !intro_image->empty()) {
            for (const auto& path : *intro_image) {
                loadImage(path); // Preload intro images into cache
            }
        }
    }

    std::unique_ptr<std::vector<std::string>> getIntroImageVector(){
        if (intro_image) {
            return std::make_unique<std::vector<std::string>>(*intro_image);
        } else {
            return std::make_unique<std::vector<std::string>>();
        }
    }

    SDL_Texture* loadImage(const std::string& path) {
        if (image_index_map.find(path) != image_index_map.end()) {
            int idx = image_index_map[path];
            if (idx >= 0 && idx < (int)cache.size() && cache[idx]) {
                return cache[idx];
            } else {
                std::cout << "error: cache index out of bounds or null for " << path << "\n";
                return nullptr;
            }
        }
        std::string fullpath = path;
        if (!std::filesystem::exists(path)) {
            fullpath = "resources/images/" + path + ".png";
            if (!std::filesystem::exists(fullpath)) {
                std::cout << "error: missing image " << path << "\n";
                return nullptr;
            }
        }
        SDL_Texture* tex = IMG_LoadTexture(renderer_, fullpath.c_str());
        if (!tex) {
            std::cout << "error: IMG_LoadTexture failed for " << fullpath << ": " << IMG_GetError() << "\n";
            return nullptr;
        }
        cache.emplace_back(tex);
        image_index_map[path] = static_cast<int>(cache.size()) - 1;
        return tex;
    }

    void renderImage(const std::string& path, SDL_FRect dst) {
        if (image_index_map.find(path) == image_index_map.end()){
            if (!loadImage(path)) return;
        }
        int idx = image_index_map[path];
        if (idx < 0 || idx >= (int)cache.size()) {
            std::cout << "error: renderImage cache index out of bounds for " << path << "\n";
            return;
        }
        SDL_Texture* tex = cache[idx];
        if (!tex) {
            std::cout << "error: renderImage cache texture is null for " << path << "\n";
            return;
        }
        Helper::SDL_RenderCopy(renderer_, tex, NULL, &dst);
    }

    void renderImageEx (int actor_id, const std::string& actor_name,
        const std::string& path, SDL_FRect dst, float angle, SDL_FPoint* center,
        glm::vec2 cam,
        SDL_RendererFlip flip = SDL_FLIP_NONE) {
        if (image_index_map.find(path) == image_index_map.end()){
            if (!loadImage(path)) return;
        }
        int idx = image_index_map[path];
        if (idx < 0 || idx >= (int)cache.size()) {
            std::cout << "error: renderImageEx cache index out of bounds for " << path << "\n";
            return;
        }
        SDL_Texture* tex = cache[idx];
        if (!tex) {
            std::cout << "error: renderImageEx cache texture is null for " << path << "\n";
            return;
        }
        Helper::SDL_RenderCopyEx(actor_id, actor_name, renderer_, tex, NULL, &dst, angle, center, flip);
    }

    void clearCache() {
        for (auto& tex : cache) {
            SDL_DestroyTexture(tex);
        }
        cache.clear();
        image_index_map.clear();
    }

};

class ImageRenderConfig {
public:

    int actor_id; // for renderImageEx
    std::string actor_name; // for renderImageEx

    const std::string& image_path;
    SDL_FRect dst;

    float angle; // for renderImageEx
    SDL_FPoint* view_pivot; // for renderImageEx

    ImageRenderConfig(const std::string& path, SDL_FRect dst_rect, 
    int _id = -1, std::string _name = "", float _agl = 0.0f, SDL_FPoint* _vp = nullptr) : 
    actor_id(_id), actor_name(_name), 
    image_path(path), dst(dst_rect),
    angle(_agl), view_pivot(_vp)
    {}
};

#endif // IMAGE_DB_H
