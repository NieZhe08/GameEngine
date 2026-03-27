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

struct ImageDrawQueue {
    std::vector<std::uint64_t> submit_order;
    std::vector<std::string> image_name;
    std::vector<float> x;
    std::vector<float> y;
    std::vector<int> rotation_degrees;
    std::vector<float> scale_x;
    std::vector<float> scale_y;
    std::vector<float> pivot_x;
    std::vector<float> pivot_y;
    std::vector<int> r;
    std::vector<int> g;
    std::vector<int> b;
    std::vector<int> a;
    std::vector<int> sorting_order;

    void push(std::uint64_t new_submit_order,
              const std::string& new_image_name,
              float new_x,
              float new_y,
              int new_rotation_degrees,
              float new_scale_x,
              float new_scale_y,
              float new_pivot_x,
              float new_pivot_y,
              int new_r,
              int new_g,
              int new_b,
              int new_a,
              int new_sorting_order) {
        submit_order.push_back(new_submit_order);
        image_name.push_back(new_image_name);
        x.push_back(new_x);
        y.push_back(new_y);
        rotation_degrees.push_back(new_rotation_degrees);
        scale_x.push_back(new_scale_x);
        scale_y.push_back(new_scale_y);
        pivot_x.push_back(new_pivot_x);
        pivot_y.push_back(new_pivot_y);
        r.push_back(new_r);
        g.push_back(new_g);
        b.push_back(new_b);
        a.push_back(new_a);
        sorting_order.push_back(new_sorting_order);
    }

    size_t size() const {
        return submit_order.size();
    }

    void clear() {
        submit_order.clear();
        image_name.clear();
        x.clear();
        y.clear();
        rotation_degrees.clear();
        scale_x.clear();
        scale_y.clear();
        pivot_x.clear();
        pivot_y.clear();
        r.clear();
        g.clear();
        b.clear();
        a.clear();
        sorting_order.clear();
    }
};

class ImageManager{
private:
    SDL_Renderer* ren;
    std::unordered_map<std::string, SDL_Texture*> image_cache;
    ImageDrawQueue image_draw_queue;
    ImageDrawQueue ui_image_draw_queue;
    std::queue<PixelDrawRequest> pixel_draw_queue;
    std::uint64_t request_counter = 0;
    glm::vec2 camera_position = glm::vec2(0.0f, 0.0f);
    float camera_zoom_factor = 1.0f;

    bool intersectsScreenRect(float x, float y, float w, float h, float screen_w, float screen_h) const {
        return !(x + w <= 0.0f || y + h <= 0.0f || x >= screen_w || y >= screen_h);
    }

    bool shouldQueueSceneDraw(const std::string& image_name,
                              float x,
                              float y,
                              float scale_x,
                              float scale_y,
                              float pivot_x,
                              float pivot_y) {
        SDL_Texture* tex = loadImage(image_name);
        if (!tex) {
            return false;
        }

        float tex_width = 0.0f;
        float tex_height = 0.0f;
        Helper::SDL_QueryTexture(tex, &tex_width, &tex_height);

        const float dst_width = tex_width * glm::abs(scale_x);
        const float dst_height = tex_height * glm::abs(scale_y);
        if (dst_width <= 0.0f || dst_height <= 0.0f) {
            return false;
        }

        int render_width = 0;
        int render_height = 0;
        SDL_GetRendererOutputSize(ren, &render_width, &render_height);
        if (render_width <= 0 || render_height <= 0) {
            return true;
        }

        const float draw_pivot_x = pivot_x * dst_width;
        const float draw_pivot_y = pivot_y * dst_height;
        const float pixels_per_meter = 100.0f;

        const glm::vec2 final_rendering_position = glm::vec2(x, y) - camera_position;
        const float dst_x = final_rendering_position.x * pixels_per_meter
            + static_cast<float>(render_width) * 0.5f * (1.0f / camera_zoom_factor)
            - draw_pivot_x;
        const float dst_y = final_rendering_position.y * pixels_per_meter
            + static_cast<float>(render_height) * 0.5f * (1.0f / camera_zoom_factor)
            - draw_pivot_y;

        return intersectsScreenRect(dst_x, dst_y, dst_width, dst_height, static_cast<float>(render_width), static_cast<float>(render_height));
    }

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

    void renderImageRequest(const ImageDrawQueue& queue, size_t index, bool scene_space, int render_width, int render_height) {
        SDL_Texture* tex = loadImage(queue.image_name[index]);
        if (!tex) {
            return;
        }

        float tex_width = 0.0f;
        float tex_height = 0.0f;
        Helper::SDL_QueryTexture(tex, &tex_width, &tex_height);

        int flip_mode = SDL_FLIP_NONE;
        if (queue.scale_x[index] < 0.0f) {
            flip_mode |= SDL_FLIP_HORIZONTAL;
        }
        if (queue.scale_y[index] < 0.0f) {
            flip_mode |= SDL_FLIP_VERTICAL;
        }

        float x_scale = glm::abs(queue.scale_x[index]);
        float y_scale = glm::abs(queue.scale_y[index]);

        float dst_width = tex_width * x_scale;
        float dst_height = tex_height * y_scale;

        float pivot_x = queue.pivot_x[index] * dst_width;
        float pivot_y = queue.pivot_y[index] * dst_height;

        float dst_x = 0.0f;
        float dst_y = 0.0f;

        if (scene_space) {
            const float pixels_per_meter = 100.0f;
            glm::vec2 final_rendering_position = glm::vec2(queue.x[index], queue.y[index]) - camera_position;

            dst_x = final_rendering_position.x * pixels_per_meter
                + static_cast<float>(render_width) * 0.5f * (1.0f / camera_zoom_factor)
                - pivot_x;
            dst_y = final_rendering_position.y * pixels_per_meter
                + static_cast<float>(render_height) * 0.5f * (1.0f / camera_zoom_factor)
                - pivot_y;
        } else {
            dst_x = queue.x[index] - pivot_x;
            dst_y = queue.y[index] - pivot_y;
        }

        SDL_FRect tex_rect = { dst_x, dst_y, dst_width, dst_height };
        SDL_FPoint pivot_point = { pivot_x, pivot_y };

        SDL_SetTextureColorMod(tex, queue.r[index], queue.g[index], queue.b[index]);
        SDL_SetTextureAlphaMod(tex, queue.a[index]);

        Helper::SDL_RenderCopyEx(
            -1,
            "",
            ren,
            tex,
            nullptr,
            &tex_rect,
            static_cast<float>(queue.rotation_degrees[index]),
            &pivot_point,
            static_cast<SDL_RendererFlip>(flip_mode)
        );

        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    void renderQueuedImages(ImageDrawQueue& queue, bool scene_space) {
        std::vector<size_t> sorted_indices(queue.size());
        for (size_t i = 0; i < sorted_indices.size(); i++) {
            sorted_indices[i] = i;
        }

        std::stable_sort(sorted_indices.begin(), sorted_indices.end(), [&queue](size_t lhs, size_t rhs) {
            if (queue.sorting_order[lhs] != queue.sorting_order[rhs]) {
                return queue.sorting_order[lhs] < queue.sorting_order[rhs];
            }
            return queue.submit_order[lhs] < queue.submit_order[rhs];
        });

        int render_width = 0;
        int render_height = 0;

        if (scene_space) {
            SDL_GetRendererOutputSize(ren, &render_width, &render_height);
            SDL_RenderSetScale(ren, camera_zoom_factor, camera_zoom_factor);
        }

        for (size_t index : sorted_indices) {
            renderImageRequest(queue, index, scene_space, render_width, render_height);
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
        this->ui_image_draw_queue.push(request_counter++, image_name, x, y, 0, 1.0f, 1.0f, 0.0f, 0.0f, r, g, b, a, sorting_order);
    }

    void pushDrawUI(const std::string &image_name, float x, float y){
        this->pushDrawUIEx(image_name, x, y, 255, 255, 255, 255, 0);
    }

    void pushDrawEx(const std::string &image_name, float x, float y, int rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, int r, int g, int b, int a, int sorting_order){
        if (!shouldQueueSceneDraw(image_name, x, y, scale_x, scale_y, pivot_x, pivot_y)) {
            return;
        }
        this->image_draw_queue.push(request_counter++, image_name, x, y, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order);
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
