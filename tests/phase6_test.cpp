#include "../include/save_engine.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "--- STARTING PHASE 6 TEST ---" << std::endl;
    
    GameState originalState = {5, 5, 10, 10, 10, 5, 50, Difficulty::EASY};
    assert(SaveEngine::saveState(originalState));
    
    GameState loadedState = {0, 0, 0, 0, 0, 0, 0, Difficulty::EASY};
    assert(SaveEngine::loadState(loadedState));
    
    assert(originalState.hp == loadedState.hp);
    assert(originalState.maxHp == loadedState.maxHp);
    assert(originalState.x == loadedState.x);
    assert(originalState.y == loadedState.y);
    assert(originalState.atk == loadedState.atk);
    assert(originalState.def == loadedState.def);
    assert(originalState.chanceToRun == loadedState.chanceToRun);
    assert(originalState.difficulty == loadedState.difficulty);
    
    assert(SaveEngine::hasValidSave());
    SaveEngine::deleteSave();
    assert(!SaveEngine::hasValidSave());
    
    std::cout << "[INFO] Persistence systems verified." << std::endl;
    std::cout << "--- PHASE 6 TEST COMPLETE ---" << std::endl;
    return 0;
}
