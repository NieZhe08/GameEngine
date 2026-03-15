#include "game_engine.h"
#include "SDL2/SDL.h"
#include "Helper.h"
#include <chrono>
#include <iostream>

int main(int argc, char* argv[]) {
    GameEngine game;
    game.gameLoop();
    return 0;
}