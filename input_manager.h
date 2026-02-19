#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "SDL2/SDL.h"
#include <unordered_map>
#include <vector>

enum INPUT_STATE { INPUT_STATE_UP, INPUT_STATE_JUST_BECAME_DOWN, INPUT_STATE_DOWN, INPUT_STATE_JUST_BECAME_UP };

class Input
{
public:
	static void Init(); // Call before main loop begins.
	static void ProcessEvent(const SDL_Event & e); // Call every frame at start of event loop.
	static void LateUpdate(); // Call at end of frame to transition states.

	static bool GetKey(SDL_Scancode keycode); // Returns true if key is held down (includes just became down)
	static bool GetKeyDown(SDL_Scancode keycode); // Returns true only on the frame the key was pressed
	static bool GetKeyUp(SDL_Scancode keycode); // Returns true only on the frame the key was released
	static bool GetQuit(); // Returns true if SDL_QUIT event was received

protected:
	static void ResetQuit(); // Reset the quit flag

private:
	static inline std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states;
	static inline bool should_quit = false;
	static inline std::vector<SDL_Scancode> just_became_down_scancodes;
	static inline std::vector<SDL_Scancode> just_became_up_scancodes;
};

// Implementation

inline void Input::Init() {
	keyboard_states.clear();
	just_became_down_scancodes.clear();
	just_became_up_scancodes.clear();
	should_quit = false;
}

inline void Input::ProcessEvent(const SDL_Event & e) {
	if (e.type == SDL_QUIT) {
		should_quit = true;
		return;
	}
	if (e.type == SDL_KEYDOWN && !e.key.repeat) {
		SDL_Scancode scancode = e.key.keysym.scancode;
		auto it = keyboard_states.find(scancode);
		INPUT_STATE current_state = (it != keyboard_states.end()) ? it->second : INPUT_STATE_UP;
		
		// Only transition to JUST_BECAME_DOWN if the key was up or not tracked yet
		if (current_state == INPUT_STATE_UP) {
			keyboard_states[scancode] = INPUT_STATE_JUST_BECAME_DOWN;
			just_became_down_scancodes.push_back(scancode);
		}
	}
	else if (e.type == SDL_KEYUP) {
		SDL_Scancode scancode = e.key.keysym.scancode;
		auto it = keyboard_states.find(scancode);
		
		// Only transition to JUST_BECAME_UP if the key was tracked and currently down
		if (it != keyboard_states.end() && (it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN)) {
			keyboard_states[scancode] = INPUT_STATE_JUST_BECAME_UP;
			just_became_up_scancodes.push_back(scancode);
		}
	}
}

inline void Input::LateUpdate() {
	// Update states from JUST_BECAME_DOWN to DOWN
	for (SDL_Scancode scancode : just_became_down_scancodes) {
		if (keyboard_states[scancode] == INPUT_STATE_JUST_BECAME_DOWN) {
			keyboard_states[scancode] = INPUT_STATE_DOWN;
		}
	}
	just_became_down_scancodes.clear();
	
	// Update states from JUST_BECAME_UP to UP
	for (SDL_Scancode scancode : just_became_up_scancodes) {
		if (keyboard_states[scancode] == INPUT_STATE_JUST_BECAME_UP) {
			keyboard_states[scancode] = INPUT_STATE_UP;
		}
	}
	just_became_up_scancodes.clear();
}

inline bool Input::GetKey(SDL_Scancode keycode) {
	auto it = keyboard_states.find(keycode);
	if (it == keyboard_states.end()) return false;
	return it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN;
}

inline bool Input::GetKeyDown(SDL_Scancode keycode) {
	auto it = keyboard_states.find(keycode);
	if (it == keyboard_states.end()) return false;
	return it->second == INPUT_STATE_JUST_BECAME_DOWN;
}

inline bool Input::GetKeyUp(SDL_Scancode keycode) {
	auto it = keyboard_states.find(keycode);
	if (it == keyboard_states.end()) return false;
	return it->second == INPUT_STATE_JUST_BECAME_UP;
}

inline bool Input::GetQuit() {
	return should_quit;
}

inline void Input::ResetQuit() {
	should_quit = false;
}

#endif
