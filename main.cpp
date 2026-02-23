#include "game_engine.h"
#include "SDL2/SDL.h"
#include "Helper.h"
#include <chrono>
#include <iostream>

int main(int argc, char* argv[]) {
    //auto start = std::chrono::high_resolution_clock::now();
    
    GameEngine game;
    game.gameLoop();
    
    //auto end = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    //std::cout << "Game runtime: " << duration.count() << " ms (" << duration.count() / 1000.0 << " s)" << std::endl;
    
    return 0;
}

/*
for (int i=0; i<9; i++){
        int row = 15 - 4 + i;
        for (int j=0; j<13; j++){
            int col = 19 - 6 + j;
            std::cout<<hardcoded_map[row][col];        
        }
        std::cout<<"\n";
    }
*/    