#include "game_engine.h"
#include "SDL2/SDL.h"
#include "Helper.h"
#include <chrono>
#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {
    //std::cout<< "Starting game..." << std::endl;
    // using chorono to measure runtime
    //auto start = std::chrono::high_resolution_clock::now();
    GameEngine game;
    game.gameLoop();
    //auto end = std::chrono::high_resolution_clock::now();
    //std::cout<< "Exiting game..." << std::endl;
    //std::cout<<"Run time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
    return 0;
}