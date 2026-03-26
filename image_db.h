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
    float x, y;
    int rotation_degrees;
    float scale_x, scale_y, pivot_x, pivot_y;
    int r, g, b, a;
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

    SDL_Texture* createDefaultParticleTexture(const std::string& cache_key) {
        auto found = image_cache.find(cache_key);
        if (found != image_cache.end()) {
            return found->second;
        }

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
            0,
            8,
            8,
            32,
            SDL_PIXELFORMAT_RGBA8888
        );
        if (!surface) {
            std::cout << "error: failed to create default particle surface: " << SDL_GetError() << "\n";
            return nullptr;
        }

        Uint32 white_color = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
        SDL_FillRect(surface, nullptr, white_color);

        SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, surface);
        SDL_FreeSurface(surface);

        if (!texture) {
            std::cout << "error: failed to create default particle texture: " << SDL_GetError() << "\n";
            return nullptr;
        }

        image_cache[cache_key] = texture;
        return texture;
    }

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
        // Textures are tied to renderer/driver lifetime in this project.
        // Let renderer teardown reclaim them to avoid shutdown double-free
        // hazards on some driver stacks.
        image_cache.clear();
    }

    SDL_Texture* loadImage(const std::string& path) {
        if (path.empty()) {
            return createDefaultParticleTexture(path);
        }

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
        // UI coordinates are top-left anchored, so use a (0,0) pivot.
        this->ui_image_draw_queue.push_back({request_counter++, image_name, x, y, 0, 1.0f, 1.0f, 0.0f, 0.0f, r, g, b, a, sorting_order});
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

#endif // IMAGE_DB_H
