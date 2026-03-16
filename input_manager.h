#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "SDL2/SDL.h"
#include "glm/glm.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

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
	static glm::vec2 GetMousePosition();
	static bool GetMouseButton(int button);
	static bool GetMouseButtonDown(int button);
	static bool GetMouseButtonUp(int button);
	static float GetMouseScrollDelta();
	static void HideCursor();
	static void ShowCursor();
	static bool GetQuit(); // Returns true if SDL_QUIT event was received

protected:
	static void ResetQuit(); // Reset the quit flag

private:
	static inline std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states;
	static inline bool should_quit = false;
	static inline std::vector<SDL_Scancode> just_became_down_scancodes;
	static inline std::vector<SDL_Scancode> just_became_up_scancodes;

	static inline glm::vec2 mouse_position = glm::vec2(0.0f, 0.0f);
	static inline std::unordered_map<int, INPUT_STATE> mouse_button_states;
	static inline std::vector<int> just_became_down_buttons;
	static inline std::vector<int> just_became_up_buttons;
	static inline float mouse_scroll_this_frame = 0.0f;

	static SDL_Scancode KeycodeFromString(const std::string& keycode);
	static bool IsValidScancode(SDL_Scancode keycode);
	static bool IsValidMouseButton(int button);
};

// Implementation

inline void Input::Init() {
	keyboard_states.clear();
	just_became_down_scancodes.clear();
	just_became_up_scancodes.clear();
	mouse_button_states.clear();
	just_became_down_buttons.clear();
	just_became_up_buttons.clear();
	mouse_scroll_this_frame = 0.0f;
	mouse_position = glm::vec2(0.0f, 0.0f);
	should_quit = false;
}

inline void Input::ProcessEvent(const SDL_Event & e) {
	if (e.type == SDL_QUIT) {
		should_quit = true;
		return;
	}
	if (e.type == SDL_KEYDOWN ) {
		SDL_Scancode scancode = e.key.keysym.scancode;
		if (!IsValidScancode(scancode)) {
			return;
		}
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
		if (!IsValidScancode(scancode)) {
			return;
		}
		auto it = keyboard_states.find(scancode);
		
		// Only transition to JUST_BECAME_UP if the key was tracked and currently down
		if (it != keyboard_states.end() && (it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN)) {
			keyboard_states[scancode] = INPUT_STATE_JUST_BECAME_UP;
			just_became_up_scancodes.push_back(scancode);
		}
	}
	else if (e.type == SDL_MOUSEMOTION) {
		mouse_position = glm::vec2(static_cast<float>(e.motion.x), static_cast<float>(e.motion.y));
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN) {
		int button = static_cast<int>(e.button.button);
		if (!IsValidMouseButton(button)) {
			return;
		}
		auto it = mouse_button_states.find(button);
		INPUT_STATE current_state = (it != mouse_button_states.end()) ? it->second : INPUT_STATE_UP;
		if (current_state == INPUT_STATE_UP) {
			mouse_button_states[button] = INPUT_STATE_JUST_BECAME_DOWN;
			just_became_down_buttons.push_back(button);
		}
	}
	else if (e.type == SDL_MOUSEBUTTONUP) {
		int button = static_cast<int>(e.button.button);
		if (!IsValidMouseButton(button)) {
			return;
		}
		auto it = mouse_button_states.find(button);
		if (it != mouse_button_states.end() && (it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN)) {
			mouse_button_states[button] = INPUT_STATE_JUST_BECAME_UP;
			just_became_up_buttons.push_back(button);
		}
	}
	else if (e.type == SDL_MOUSEWHEEL) {
		mouse_scroll_this_frame += e.wheel.preciseY;
	}
}

inline void Input::LateUpdate() {
	// Update states from JUST_BECAME_DOWN to DOWN
	for (SDL_Scancode scancode : just_became_down_scancodes) {
		auto it = keyboard_states.find(scancode);
		if (it != keyboard_states.end() && it->second == INPUT_STATE_JUST_BECAME_DOWN) {
			it->second = INPUT_STATE_DOWN;
		}
	}
	just_became_down_scancodes.clear();
	
	// Update states from JUST_BECAME_UP to UP
	for (SDL_Scancode scancode : just_became_up_scancodes) {
		auto it = keyboard_states.find(scancode);
		if (it != keyboard_states.end() && it->second == INPUT_STATE_JUST_BECAME_UP) {
			it->second = INPUT_STATE_UP;
		}
	}
	just_became_up_scancodes.clear();

	for (int button : just_became_down_buttons) {
		auto it = mouse_button_states.find(button);
		if (it != mouse_button_states.end() && it->second == INPUT_STATE_JUST_BECAME_DOWN) {
			it->second = INPUT_STATE_DOWN;
		}
	}
	just_became_down_buttons.clear();

	for (int button : just_became_up_buttons) {
		auto it = mouse_button_states.find(button);
		if (it != mouse_button_states.end() && it->second == INPUT_STATE_JUST_BECAME_UP) {
			it->second = INPUT_STATE_UP;
		}
	}
	just_became_up_buttons.clear();

	mouse_scroll_this_frame = 0.0f;
}

inline bool Input::GetKey(SDL_Scancode keycode) {
	if (!IsValidScancode(keycode)) return false;
	auto it = keyboard_states.find(keycode);
	if (it == keyboard_states.end()) {return false;}
	else {return it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN;}
}

inline bool Input::GetKeyDown(SDL_Scancode keycode) {
	if (!IsValidScancode(keycode)) return false;
	auto it = keyboard_states.find(keycode);
	if (it == keyboard_states.end()) {return false;}
	else {return it->second == INPUT_STATE_JUST_BECAME_DOWN;}
}

inline bool Input::GetKeyUp(SDL_Scancode keycode) {
	if (!IsValidScancode(keycode)) return false;
	auto it = keyboard_states.find(keycode);
	if (it == keyboard_states.end()) {return false;}
	else {return it->second == INPUT_STATE_JUST_BECAME_UP;}
}

inline SDL_Scancode Input::KeycodeFromString(const std::string& keycode) {
	if (keycode.empty()) return SDL_SCANCODE_UNKNOWN;
	std::string normalized = keycode;
	// CSV 里有部分键名前后带空格，这里先做修剪再做大小写归一化。
	while (!normalized.empty() && std::isspace(static_cast<unsigned char>(normalized.front()))) {
		normalized.erase(normalized.begin());
	}
	while (!normalized.empty() && std::isspace(static_cast<unsigned char>(normalized.back()))) {
		normalized.pop_back();
	}
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

	static const std::unordered_map<std::string, SDL_Scancode> map = {
		{"up", SDL_SCANCODE_UP}, {"down", SDL_SCANCODE_DOWN}, {"left", SDL_SCANCODE_LEFT}, {"right", SDL_SCANCODE_RIGHT},
		{"space", SDL_SCANCODE_SPACE}, {"enter", SDL_SCANCODE_RETURN}, {"return", SDL_SCANCODE_RETURN},
		{"escape", SDL_SCANCODE_ESCAPE}, {"esc", SDL_SCANCODE_ESCAPE}, {"tab", SDL_SCANCODE_TAB},
		{"backspace", SDL_SCANCODE_BACKSPACE}, {"delete", SDL_SCANCODE_DELETE}, {"insert", SDL_SCANCODE_INSERT},
		{"lshift", SDL_SCANCODE_LSHIFT}, {"rshift", SDL_SCANCODE_RSHIFT},
		{"lctrl", SDL_SCANCODE_LCTRL}, {"rctrl", SDL_SCANCODE_RCTRL},
		{"lalt", SDL_SCANCODE_LALT}, {"ralt", SDL_SCANCODE_RALT},
		{"0", SDL_SCANCODE_0}, {"1", SDL_SCANCODE_1}, {"2", SDL_SCANCODE_2}, {"3", SDL_SCANCODE_3}, {"4", SDL_SCANCODE_4},
		{"5", SDL_SCANCODE_5}, {"6", SDL_SCANCODE_6}, {"7", SDL_SCANCODE_7}, {"8", SDL_SCANCODE_8}, {"9", SDL_SCANCODE_9},
		{"/", SDL_SCANCODE_SLASH}, {";", SDL_SCANCODE_SEMICOLON}, {"=", SDL_SCANCODE_EQUALS}, {"-", SDL_SCANCODE_MINUS},
		{".", SDL_SCANCODE_PERIOD}, {",", SDL_SCANCODE_COMMA}, {"[", SDL_SCANCODE_LEFTBRACKET}, {"]", SDL_SCANCODE_RIGHTBRACKET},
		{"\\", SDL_SCANCODE_BACKSLASH}, {"'", SDL_SCANCODE_APOSTROPHE},
		{"a", SDL_SCANCODE_A}, {"b", SDL_SCANCODE_B}, {"c", SDL_SCANCODE_C}, {"d", SDL_SCANCODE_D}, {"e", SDL_SCANCODE_E},
		{"f", SDL_SCANCODE_F}, {"g", SDL_SCANCODE_G}, {"h", SDL_SCANCODE_H}, {"i", SDL_SCANCODE_I}, {"j", SDL_SCANCODE_J},
		{"k", SDL_SCANCODE_K}, {"l", SDL_SCANCODE_L}, {"m", SDL_SCANCODE_M}, {"n", SDL_SCANCODE_N}, {"o", SDL_SCANCODE_O},
		{"p", SDL_SCANCODE_P}, {"q", SDL_SCANCODE_Q}, {"r", SDL_SCANCODE_R}, {"s", SDL_SCANCODE_S}, {"t", SDL_SCANCODE_T},
		{"u", SDL_SCANCODE_U}, {"v", SDL_SCANCODE_V}, {"w", SDL_SCANCODE_W}, {"x", SDL_SCANCODE_X}, {"y", SDL_SCANCODE_Y},
		{"z", SDL_SCANCODE_Z}
	};

	auto it = map.find(normalized);
	if (it == map.end()) return SDL_SCANCODE_UNKNOWN;
	return it->second;
}

inline glm::vec2 Input::GetMousePosition() {
	return mouse_position;
}

inline bool Input::GetMouseButton(int button) {
	if (!IsValidMouseButton(button)) return false;
	auto it = mouse_button_states.find(button);
	if (it == mouse_button_states.end()) return false;
	return it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN;
}

inline bool Input::GetMouseButtonDown(int button) {
	if (!IsValidMouseButton(button)) return false;
	auto it = mouse_button_states.find(button);
	if (it == mouse_button_states.end()) return false;
	return it->second == INPUT_STATE_JUST_BECAME_DOWN;
}

inline bool Input::GetMouseButtonUp(int button) {
	if (!IsValidMouseButton(button)) return false;
	auto it = mouse_button_states.find(button);
	if (it == mouse_button_states.end()) return false;
	return it->second == INPUT_STATE_JUST_BECAME_UP;
}

inline bool Input::IsValidScancode(SDL_Scancode keycode) {
	int code = static_cast<int>(keycode);
	return code > static_cast<int>(SDL_SCANCODE_UNKNOWN) && code < static_cast<int>(SDL_NUM_SCANCODES);
}

inline bool Input::IsValidMouseButton(int button) {
	return button >= static_cast<int>(SDL_BUTTON_LEFT) && button <= static_cast<int>(SDL_BUTTON_X2);
}

inline float Input::GetMouseScrollDelta() {
	return mouse_scroll_this_frame;
}

inline void Input::HideCursor() {
	SDL_ShowCursor(SDL_DISABLE);
}

inline void Input::ShowCursor() {
	SDL_ShowCursor(SDL_ENABLE);
}

inline bool Input::GetQuit() {
	return should_quit;
}

inline void Input::ResetQuit() {
	should_quit = false;
}

#endif
