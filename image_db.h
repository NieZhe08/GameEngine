#ifndef IMAGE_DB_H
#define IMAGE_DB_H

#include <iostream>
#include <filesystem>
#include <vector>
#include <string.h>
#include "Helper.h"
#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "unordered_map"
#include "game_utils.h"
#include "Helper.h"
#include <queue>
#include <algorithm>
#include <cstdint>

struct PixelDrawRequest{
    int x, y, r, g, b, a;
};

struct ImageDrawRequest{
    std::uint64_t submit_order;
    std::string image_name;
    float x;
    float y;
    int rotation_degrees;
    float scale_x;
    float scale_y;
    float pivot_x;
    float pivot_y;
    int r;
    int g;
    int b;
    int a;
    int sorting_order;
};

inline bool compareImageRequests(const ImageDrawRequest& a, const ImageDrawRequest& b) {
    if (a.sorting_order != b.sorting_order) {
        return a.sorting_order < b.sorting_order;
    }
    return a.submit_order < b.submit_order;
}

class ImageManager{
private:
    SDL_Renderer* ren;
    std::unordered_map<std::string, SDL_Texture*> image_cache;
    std::vector<ImageDrawRequest> image_draw_queue;
    std::vector<ImageDrawRequest> ui_image_draw_queue;
    std::queue<PixelDrawRequest> pixel_draw_queue;
    std::uint64_t request_counter = 0;
    glm::vec2 camera_position = glm::vec2(0.0f, 0.0f);
    float camera_zoom_factor = 1.0f;

    void renderImageRequest(const ImageDrawRequest& request, bool scene_space, int render_width, int render_height) {
        SDL_Texture* tex = loadImage(request.image_name);
        if (!tex) {
            return;
        }

        float tex_width = 0.0f;
        float tex_height = 0.0f;
        Helper::SDL_QueryTexture(tex, &tex_width, &tex_height);

        int flip_mode = SDL_FLIP_NONE;
        if (request.scale_x < 0.0f) {
            flip_mode |= SDL_FLIP_HORIZONTAL;
        }
        if (request.scale_y < 0.0f) {
            flip_mode |= SDL_FLIP_VERTICAL;
        }

        float x_scale = glm::abs(request.scale_x);
        float y_scale = glm::abs(request.scale_y);

        int dst_width = static_cast<int>(tex_width * x_scale);
        int dst_height = static_cast<int>(tex_height * y_scale);

        int pivot_x = static_cast<int>(request.pivot_x * static_cast<float>(dst_width));
        int pivot_y = static_cast<int>(request.pivot_y * static_cast<float>(dst_height));

        int dst_x = 0;
        int dst_y = 0;

        if (scene_space) {
            const int pixels_per_meter = 100;
            glm::vec2 final_rendering_position = glm::vec2(request.x, request.y) - camera_position;

            dst_x = static_cast<int>(
                final_rendering_position.x * static_cast<float>(pixels_per_meter)
                + static_cast<float>(render_width) * 0.5f * (1.0f / camera_zoom_factor)
                - static_cast<float>(pivot_x)
            );
            dst_y = static_cast<int>(
                final_rendering_position.y * static_cast<float>(pixels_per_meter)
                + static_cast<float>(render_height) * 0.5f * (1.0f / camera_zoom_factor)
                - static_cast<float>(pivot_y)
            );
        } else {
            dst_x = static_cast<int>(request.x) - pivot_x;
            dst_y = static_cast<int>(request.y) - pivot_y;
        }

        SDL_FRect tex_rect = {
            static_cast<float>(dst_x),
            static_cast<float>(dst_y),
            static_cast<float>(dst_width),
            static_cast<float>(dst_height)
        };
        SDL_FPoint pivot_point = {
            static_cast<float>(pivot_x),
            static_cast<float>(pivot_y)
        };

        SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
        SDL_SetTextureAlphaMod(tex, request.a);

        Helper::SDL_RenderCopyEx(
            -1,
            "",
            ren,
            tex,
            nullptr,
            &tex_rect,
            static_cast<float>(request.rotation_degrees),
            &pivot_point,
            static_cast<SDL_RendererFlip>(flip_mode)
        );

        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    void renderQueuedImages(std::vector<ImageDrawRequest>& queue, bool scene_space) {
        std::stable_sort(queue.begin(), queue.end(), compareImageRequests);

        int render_width = 0;
        int render_height = 0;

        if (scene_space) {
            SDL_GetRendererOutputSize(ren, &render_width, &render_height);
            SDL_RenderSetScale(ren, camera_zoom_factor, camera_zoom_factor);
        }

        for (const auto& request : queue) {
            renderImageRequest(request, scene_space, render_width, render_height);
        }

        if (scene_space) {
            SDL_RenderSetScale(ren, 1.0f, 1.0f);
        }

        queue.clear();
    }

public:
    ImageManager(SDL_Renderer* renderer) : ren(renderer) {}

    ~ImageManager() {
        for (auto& entry : image_cache) {
            if (entry.second) {
                SDL_DestroyTexture(entry.second);
            }
        }
    }

    SDL_Texture* loadImage(const std::string& path) {
        if (image_cache.find(path) != image_cache.end()) {
            return image_cache[path];
        }
        std::string fullpath = path;
        if (!std::filesystem::exists(path)) {
            fullpath = "resources/images/" + path + ".png";
            if (!std::filesystem::exists(fullpath)) {
                std::cout << "error: missing image " << path ;
                return nullptr;
            }
        }
        SDL_Texture* tex = IMG_LoadTexture(ren, fullpath.c_str());
        if (!tex) {
            std::cout << "error: IMG_LoadTexture failed for " << fullpath << ": " << IMG_GetError() << "\n";
            return nullptr;
        }
        image_cache[path] = tex;
        return tex;
    }

    void setCameraState(const glm::vec2& camera, float zoom_factor) {
        camera_position = camera;
        camera_zoom_factor = (zoom_factor <= 0.0f) ? 1.0f : zoom_factor;
    }

    void pushDrawUIEx(const std::string &image_name, float x, float y, int r, int g, int b, int a, int sorting_order){
        this->ui_image_draw_queue.push_back({request_counter++, image_name, x, y, 0, 1.0f, 1.0f, 0.5f, 0.5f, r, g, b, a, sorting_order});
    }

    void pushDrawUI(const std::string &image_name, float x, float y){
        this->pushDrawUIEx(image_name, x, y, 255, 255, 255, 255, 0);
    }

    void pushDrawEx(const std::string &image_name, float x, float y, int rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, int r, int g, int b, int a, int sorting_order){
        this->image_draw_queue.push_back({request_counter++, image_name, x, y, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order});
    }

    void pushDraw(const std::string &image_name, float x, float y){
        this->pushDrawEx(image_name, x, y, 0, 1.0f, 1.0f, 0.5f, 0.5f, 255, 255, 255, 255, 0);
    }

    void pushDrawPixel(int x, int y, int r, int g, int b, int a){
        this->pixel_draw_queue.push({x, y, r, g, b, a});
    }

    void renderUI(){
        renderQueuedImages(ui_image_draw_queue, false);
    }

    void renderSSImages(){
        renderQueuedImages(image_draw_queue, true);
    }

    void renderPixels(){
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

        while (!pixel_draw_queue.empty()) {
            PixelDrawRequest request = pixel_draw_queue.front();
            pixel_draw_queue.pop();

            SDL_SetRenderDrawColor(
                ren,
                static_cast<Uint8>(request.r),
                static_cast<Uint8>(request.g),
                static_cast<Uint8>(request.b),
                static_cast<Uint8>(request.a)
            );
            SDL_RenderDrawPoint(ren, request.x, request.y);
        }

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
};

/* class ImageDB {
    rapidjson::Document game;
    rapidjson::Document rendering;
    std::unique_ptr<std::vector<std::string>> intro_image;
    SDL_Renderer* renderer_;
    std::unordered_map<std::string, int> image_index_map; // Map from image path to index in cache
    std::vector<SDL_Texture*> cache; // Cache of loaded textures

    bool x_scale_actor_flipping_on_movement = false;

public:
    ImageDB(SDL_Renderer* renderer, bool _flip) : renderer_(renderer) , x_scale_actor_flipping_on_movement(_flip) {
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

    std::string getGameWinImage() {
        std::string game_win_image = "";
        if (game.HasMember("game_over_good_image")){
            game_win_image = game["game_over_good_image"].GetString();
            //loadImage(game_win_image); // Preload game win image into cache
        }
        return game_win_image;
    }

    std::string getGameLoseImage() {
        std::string game_lose_image = "";
        if (game.HasMember("game_over_bad_image")){
            game_lose_image = game["game_over_bad_image"].GetString();
            //loadImage(game_lose_image); // Preload game lose image into cache
        }
        return game_lose_image;
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
                std::cout << "error: missing image " << path ;
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
            //if (!loadImage(path)) 
            return;
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

    void renderImageEx (const Actor* actor,
        glm::vec2 cam, float zoom_factor, int frame_number = 0) {
        if (image_index_map.find(actor->view_image) == image_index_map.end()){
            if (!loadImage(actor->view_image)) return;
        }

        // Determine which image to render based on actor state (normal vs back view)
        std::string image_to_render = actor->view_image;
        if (actor->has_view_image_back && ! actor->view_dir_down){
            image_to_render = actor->view_image_back;
        }
        if (actor->has_view_image_damage && actor->damage_view_duration_frames > 0){
            image_to_render = actor->view_image_damage;
        } else if (actor->has_view_image_attack && actor->attack_view_duration_frames > 0){
            image_to_render = actor->view_image_attack;
        }

        int idx = image_index_map[image_to_render];
        if (idx < 0 || idx >= (int)cache.size()) {
            std::cout << "error: renderImageEx cache index out of bounds for " << image_to_render << "\n";
            return;
        }
        SDL_Texture* tex = cache[idx];
        if (!tex) {
            std::cout << "error: renderImageEx cache texture is null for " << image_to_render << "\n";
            return;
        }

        float tex_width = 0.0f, tex_height = 0.0f;
        Helper::SDL_QueryTexture(tex,  &tex_width, &tex_height);

        glm::vec2 bouncing_offset = (actor->movement_bounce_enabled && actor->velocity != glm::vec2(0,0)) ? 
            glm::vec2(0.0f, 10.0f * - glm::abs(std::sin(frame_number * 0.15f))) : glm::vec2(0.0f, 0.0f);
        SDL_FRect dst_rect = {
                            (actor->transform_position.x) * 100 * zoom_factor + cam.x - (actor->view_pivot_offset.x * actor->transform_scale.x)*zoom_factor + bouncing_offset.x, 
                            (actor->transform_position.y) * 100 * zoom_factor + cam.y - (actor->view_pivot_offset.y * actor->transform_scale.y)*zoom_factor + bouncing_offset.y,
                            (tex_width * actor->transform_scale.x)*zoom_factor,
                            (tex_height * actor->transform_scale.y)*zoom_factor
                        };
        
        SDL_RendererFlip f;
        bool flip_x = actor->flip_x;
        bool flip_y = actor->flip_y;
        if (x_scale_actor_flipping_on_movement) {
            // do something!
            // do nothing, we have already change flip_x inplace.
        }
        if (flip_x && flip_y) {
            f = static_cast<SDL_RendererFlip>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
        } else if (flip_x) {
            f = SDL_FLIP_HORIZONTAL;
        } else if (flip_y) {
            f = SDL_FLIP_VERTICAL;
        } else {
            f = SDL_FLIP_NONE;
        }
        SDL_FPoint pivot = {  actor->view_pivot_offset.x * zoom_factor,  actor->view_pivot_offset.y * zoom_factor };
        Helper::SDL_RenderCopyEx(actor->id, actor->actor_name, renderer_, tex, NULL, &dst_rect,
            actor->transform_rotation_degrees, &pivot, f);
    }

    void clearCache() {
        for (auto& tex : cache) {
            SDL_DestroyTexture(tex);
        }
        cache.clear();
        image_index_map.clear();
    }

    bool isInScreen(Actor* actor, SDL_FRect window_rect, glm::vec2 cam, float zoom_factor) {
        glm::vec2 bouncing_offset = (actor->movement_bounce_enabled && actor->velocity != glm::vec2(0,0)) ? 
            glm::vec2(0.0f, 10.0f * - glm::abs(std::sin(Helper::GetFrameNumber() * 0.15f))) : glm::vec2(0.0f, 0.0f);
        SDL_FRect dst_rect = {
                            (actor->transform_position.x) * 100 * zoom_factor + cam.x - (actor->view_pivot_offset.x * actor->transform_scale.x)*zoom_factor + bouncing_offset.x, 
                            (actor->transform_position.y) * 100 * zoom_factor + cam.y - (actor->view_pivot_offset.y * actor->transform_scale.y)*zoom_factor + bouncing_offset.y,
                            (actor->tex_size.x * actor->transform_scale.x)*zoom_factor,
                            (actor->tex_size.y * actor->transform_scale.y)*zoom_factor
                        };
        return checkAABB(glm::vec2(dst_rect.x + dst_rect.w/2, dst_rect.y + dst_rect.h/2), glm::vec2(dst_rect.w, dst_rect.h), 
        glm::vec2(window_rect.x + window_rect.w/2, window_rect.y + window_rect.h/2), glm::vec2(window_rect.w, window_rect.h));
    }

};

class ImageRenderConfig {
public:

    int actor_id; // for renderImageEx
    std::string actor_name; // for renderImageEx

    std::string image_path;
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
 */
#endif // IMAGE_DB_H
