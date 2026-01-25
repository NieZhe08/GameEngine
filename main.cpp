#include "game_engine.cpp"
#include "MapHelper.h"

int main() {
    GameEngine game;
    game.gameLoop();
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