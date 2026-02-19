#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include "engineUtils.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"

class JsonParser {
    rapidjson::Document game;
    rapidjson::Document rendering;

public:

    JsonParser () {
        if (!std::filesystem::exists("resources/")){
            std::cout<<"error: resources/ missing"; // no newline at end
            exit(0);
        }

        // game.config have to exist
        if (!std::filesystem::exists("resources/game.config")){
            std::cout<<"error: resources/game.config missing";// no newline at end
            exit(0);
        }
        EngineUtils::ReadJsonFile("resources/game.config", game);
        /*
        if (!game.IsObject()){
           exit(0);
        }
        */

        // rendering.config may or may not exists
        if (std::filesystem::exists("resources/rendering.config")){
            EngineUtils::ReadJsonFile("resources/rendering.config", rendering);
        }

    }

    std::string getGameStartMessage() {
        if (game.HasMember("game_start_message"))
            return game["game_start_message"].GetString();
        return "";
    }

    std::string getGameOverGoodMessage() {
        if (game.HasMember("game_over_good_message"))
            return game["game_over_good_message"].GetString();
        return "";
    }

    std::string getGameOverBadMessage() {
        if (game.HasMember("game_over_bad_message"))
            return game["game_over_bad_message"].GetString();
        return "";
    }

    std::string getGameTitle() {
        if (game.HasMember("game_title"))
            return game["game_title"].GetString();
        return "";
    }

    glm::ivec2 getResolution(){
        if (rendering.IsObject()){
            int x_res = (rendering.HasMember("x_resolution"))? rendering["x_resolution"].GetInt(): 640;
            int y_res = (rendering.HasMember("y_resolution"))? rendering["y_resolution"].GetInt(): 360;
            return glm::ivec2(x_res, y_res);
        } 
        return glm::ivec2(640, 360); // default resolution
    }

    glm::ivec3 getClearColor(){
        int r = 255; int g = 255; int b = 255; // default clear color is white
        if (rendering.IsObject()){
            r = (rendering.HasMember("clear_color_r"))? rendering["clear_color_r"].GetInt(): r;
            g = (rendering.HasMember("clear_color_g"))? rendering["clear_color_g"].GetInt(): g;
            b = (rendering.HasMember("clear_color_b"))? rendering["clear_color_b"].GetInt(): b;
        }
        return glm::ivec3(r, g, b);
    }

    std::string getInitialScene(){
        if (game.IsObject() && game.HasMember("initial_scene")){
            return game["initial_scene"].GetString();
        }
        else {
            std::cout<<"error: initial_scene unspecified";
            exit(0);
        }
    }

    std::string getHPimage(){ // only call when exist a player
        if (game.IsObject() && game.HasMember("hp_image")){
            return game["hp_image"].GetString();
        } else {
            std::cout<<"error: player actor requires an hp_image be defined"; 
            exit(0);
        }
    }

    glm::vec2 getCameraOffset(){
        if (rendering.IsObject()){
            float x_offset = (rendering.HasMember("cam_offset_x"))? rendering["cam_offset_x"].GetFloat(): 0.0f;
            float y_offset = (rendering.HasMember("cam_offset_y"))? rendering["cam_offset_y"].GetFloat(): 0.0f;
            return glm::vec2(x_offset, y_offset);
        } 
        return glm::vec2(0.0f, 0.0f); // default camera offset
    }

    float getZoomFactor(){
        if (rendering.IsObject() && rendering.HasMember("zoom_factor")){
            return rendering["zoom_factor"].GetFloat();
        } 
        return 1.0f; // default zoom factor
    }
    
};

#endif