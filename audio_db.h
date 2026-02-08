#ifndef AUDIO_DB_H
#define AUDIO_DB_H

#include <iostream>
#include <filesystem>
#include <string.h>
#include <vector>
#include "AudioHelper.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_mixer.h"
#include "engineUtils.h"
#include "rapidjson/document.h"


class AudioDB {
    rapidjson::Document game;
    rapidjson::Document rendering;
    bool has_intro_bgm;
    bool has_gameplay_bgm;

public:
    AudioDB(): has_intro_bgm(false), has_gameplay_bgm(false) {
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

    Mix_Chunk* readBGM(std::string bgm_type){
        if (game.HasMember(bgm_type.c_str())) {
            if (bgm_type == "intro_bgm"){
                has_intro_bgm = true;  
            } else if (bgm_type == "gameplay_audio"){
                has_gameplay_bgm = true;
            }
            const rapidjson::Value& bgm = game[bgm_type.c_str()];
            std::string intro_bgm = bgm.GetString();

            static bool audio_initialized = false;
            if (!audio_initialized) {
                if (AudioHelper::Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
                    std::cout << "error: AudioHelper::Mix_OpenAudio failed: " << Mix_GetError() << "\n"; // should not happen
                    return nullptr;
                }
                audio_initialized = true;
            }

            std::string wav_path = "resources/audio/" + intro_bgm + ".wav";
            std::string ogg_path = "resources/audio/" + intro_bgm + ".ogg";
            if (std::filesystem::exists(wav_path)) {
                Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(wav_path.c_str());
                if (!chunk) {
                    std::cout << "error: AudioHelper::Mix_LoadWAV failed for " << wav_path << ": " << Mix_GetError() << "\n"; // should not happen
                }
                return chunk;
            } else if (std::filesystem::exists(ogg_path)) {
                Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(ogg_path.c_str());
                if (!chunk) {
                    std::cout << "error: AudioHelper::Mix_LoadWAV failed for " << ogg_path << ": " << Mix_GetError() << "\n"; // should not happen
                }
                return chunk;
            } else {
                std::cout << "error: failed to play audio clip " << intro_bgm ;
                exit(0);
            }
        } else {
            return nullptr;
        }
    }

    AudioState hasIntroBGM() const {
        return has_intro_bgm ? AudioState::Not_Started : AudioState::Stopped;
    }

    AudioState hasGameplayBGM() const {
        return has_gameplay_bgm ? AudioState::Not_Started : AudioState::Stopped;
    }

};

#endif

