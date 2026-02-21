#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "SDL_mixer/SDL_mixer.h"
#include "AudioHelper.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "game_utils.h"

class AudioManager {
    static inline std::unordered_map<int, Mix_Chunk*> audio_cache; // Map from audio path hash to loaded Mix_Chunk*
    static inline std::unordered_map<std::string, int> loaded_audio_paths; // Set of loaded audio paths to avoid duplicate loading
    static inline int audio_count = 0;
public:

    AudioManager() {};

    static void Init(){
        if (AudioHelper::Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cout << "error: AudioHelper::Mix_OpenAudio failed: " << Mix_GetError() << "\n";
            exit(0);
        }
        AudioHelper::Mix_AllocateChannels(50);
    }
    static int loadAudio(const std::string& path){
        if (loaded_audio_paths.find(path) != loaded_audio_paths.end()) {
            return loaded_audio_paths[path];
        }
        
        for (std::string afterfix : {".wav",  ".ogg"}){
            std::string fullpath = "resources/audio/" + path + afterfix;
            if (std::filesystem::exists(path)){
                Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(path.c_str());
                if (!chunk) {
                    std::cout << "error: AudioHelper::Mix_LoadWAV failed for " << path << ": " << Mix_GetError() << "\n";
                    return -1;
                }
                audio_cache[audio_count] = chunk;
                loaded_audio_paths[path] = audio_count;
                return audio_count++;
            }
        }
            
        return -1; // audio file not found
    }
    static int PlayChannel(int channel, int audio_number, bool does_loop){
        if (audio_cache.find(audio_number) == audio_cache.end()) {
            std::cout << "error: PlayChannel failed, audio number " << audio_number << " not found in cache\n";
            return -1;
        }
        Mix_Chunk* chunk = audio_cache[audio_number];
        int loops = does_loop ? -1 : 0;
        return AudioHelper::Mix_PlayChannel(channel, chunk, loops);
    }

    static int HaltChannel(int channel){
        return AudioHelper::Mix_HaltChannel(channel);
    }
};

class AudioInfo{
    public:
        int channel;
        int audio_number;
        std::string audio_path;
        bool does_loop;
        AudioState audio_state;

        // default constructor with default values
        AudioInfo() {
            setAsDefault();
        }

        AudioInfo(std::string path, int channel, bool does_loop, AudioManager* manager):
        channel(channel), audio_number(-1), audio_path(path),
        does_loop(does_loop), audio_state(AudioState::Not_Started){
            if (path.empty()){
                setAsDefault();
            } else {
                audio_number = manager->loadAudio(path);
                if (audio_number == -1){
                    std::cout << "error: failed to load audio for path " << path << "\n";
                    setAsDefault();
                }
            }        
        };

        void setAsDefault(){
            channel = 0;
            audio_number = -1;
            audio_path = "";
            does_loop = false;
            audio_state = AudioState::None;
        }
};

#endif