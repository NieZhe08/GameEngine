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
        if (!game.IsObject()){
           exit(0);
        }

        // rendering.config may or may not exists
        if (!std::filesystem::exists("resources/rendering.config")){
            EngineUtils::ReadJsonFile("resources/rendering.config", rendering);
            if (!rendering.IsObject()){
                exit(0);
            }
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

    glm::ivec2 getResolution(){
        if (rendering.IsObject()){
            int x_res = (rendering.HasMember("x_resolution"))? rendering["x_resolution"].GetInt(): 13;
            int y_res = (rendering.HasMember("y_resolution"))? rendering["y_resolution"].GetInt(): 9;
            return glm::ivec2(x_res, y_res);
        } 
        return glm::ivec2(13, 9); // default resolution
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
};