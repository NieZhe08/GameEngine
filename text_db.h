#include <iostream>
#include <filesystem>
#include <vector>
#include <string.h>
#include "Helper.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "unordered_map"

class TextDB {
    rapidjson::Document game;
    rapidjson::Document rendering;
    std::unique_ptr<std::vector<std::string>> intro_text;
    SDL_Renderer* renderer_;
    TTF_Font* font;
    bool do_text_rendering;
    //std::unordered_map<std::string, int> image_index_map; // Map from image path to index in cache
    //std::vector<SDL_Texture*> cache; // Cache of loaded textures

public:
    TextDB(SDL_Renderer* renderer, bool _do_text_rendering) : renderer_(renderer), do_text_rendering(_do_text_rendering){
        //std::cout << "TextDB constructor called with do_text_rendering = " << do_text_rendering << std::endl;
        if (do_text_rendering){
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
                        std::cout<<"error: font "<<font_path<<" missing\n";// no newline at end
                        exit(0);
                    }
                    font = TTF_OpenFont(("resources/fonts/" + font_path + ".ttf").c_str(), 16);
                    if (!font) {
                        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;// should not happen
                        exit(0);
                    }
                } else {
                   std::cout << "error: text render failed. No font configured\n";
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

    void drawText(const std::string& text, float x, float y, SDL_Color color = {255, 255, 255, 255}) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (!surface) {
            std::cout << "Failed to create text surface: " << TTF_GetError() << std::endl; // should not happen
            return;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        if (!texture) {
            std::cout << "Failed to create text texture: " << SDL_GetError() << std::endl; // should not happen
            SDL_FreeSurface(surface);
            return;
        }
        SDL_FRect dstRect = { x, y, static_cast<float>(surface->w), static_cast<float>(surface->h) };
        Helper::SDL_RenderCopy(renderer_, texture, nullptr, &dstRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
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


