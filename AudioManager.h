#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include "SDL_mixer/SDL_mixer.h"
#include "AudioHelper.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

enum class AudioState {
    Not_Started,
    Playing,
    Stopped,
    None
};

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
            if (std::filesystem::exists(fullpath)){
                Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(fullpath.c_str());
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
        if (audio_number == -1) return -1; // invalid audio number
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
    private:
        bool channelBaseOnFrame = false; // swould not change
        bool hasNotLoaded = false; // change after loading

        void setAsDefault(){
            channel = 0;
            audio_number = -1;
            audio_path = "";
            does_loop = false;
            audio_state = AudioState::None;
        }

        void setByInfo(std::string path, int channel, bool does_loop, AudioManager* manager, 
            bool channelBaseOnFrame = false){
            audio_path = path;
            this->channel = channel;
            this->does_loop = does_loop;
            if (path.empty()){
                setAsDefault();
            } else {
                if (manager){
                    audio_number = manager->loadAudio(path);
                    if (audio_number == -1){
                        std::cout << "error: failed to load audio for path " << path << "\n";
                        setAsDefault();
                    }
                } else {
                    hasNotLoaded = true;
                }
                audio_state = AudioState::Not_Started;
            } 
        }

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

        // AudioInfo constructor with channel fixed
        AudioInfo(std::string _path, bool _does_loop, 
            AudioManager* manager = nullptr, int _channel = -1) : 
            channelBaseOnFrame(_channel == -1), 
            hasNotLoaded(manager == nullptr){
            setByInfo(_path, _channel, _does_loop, manager, channelBaseOnFrame);
        }
        
        void play(AudioManager* manager){
            if (hasNotLoaded){
                setByInfo(audio_path, channel, does_loop, manager, channelBaseOnFrame);
                hasNotLoaded = false;
            }
            if (audio_number == -1) {
                return;
            }
            if (audio_state == AudioState::Playing) return; // already playing

            if (channelBaseOnFrame){
                channel = Helper::GetFrameNumber() % 48 + 2;
            }
            int result = manager->PlayChannel(channel, audio_number, does_loop);
            if (result == -1){
                std::cout << "error: failed to play audio for path " << audio_path << "\n";
            } else {
                if (does_loop) audio_state = AudioState::Playing;
                else audio_state = AudioState::Stopped; // if not looping, consider it stopped after play
            }
        }

        void halt(AudioManager* manager){
            if (audio_number == -1) {
                return;
            }
            int result = 0;
            if (audio_state == AudioState::Playing) int result = manager->HaltChannel(channel);
            if (result == -1){
                std::cout << "error: failed to halt audio for path " << audio_path << "\n";
            } else {
                audio_state = AudioState::Stopped;
            }
        }

};

#endif