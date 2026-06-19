#include "../include/save_engine.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "--- STARTING PHASE 6 TEST ---" << std::endl;
    
    // Save Engine
    GameState originalState = {5, 5, 10, 10, 10, 5, 50};
    assert(SaveEngine::saveState("test_save.bin", originalState));
    
    GameState loadedState = {0, 0, 0, 0, 0, 0, 0};
    assert(SaveEngine::loadState("test_save.bin", loadedState));
    
    assert(originalState.hp == loadedState.hp);
    assert(originalState.maxHp == loadedState.maxHp);
    assert(originalState.x == loadedState.x);
    assert(originalState.y == loadedState.y);
    assert(originalState.atk == loadedState.atk);
    assert(originalState.def == loadedState.def);
    assert(originalState.chanceToRun == loadedState.chanceToRun);
    
    std::cout << "[INFO] Persistence systems verified." << std::endl;
    std::cout << "--- PHASE 6 TEST COMPLETE ---" << std::endl;
    return 0;
}
