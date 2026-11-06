#include "../include/save_engine.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>

int main() {
    std::cout << "--- STARTING PHASE 6 TEST ---" << std::endl;

    const std::filesystem::path dataHome = "/tmp/obsfuria-phase6-test-data";
    std::filesystem::remove_all(dataHome);
    assert(setenv("XDG_DATA_HOME", dataHome.c_str(), 1) == 0);

    GameState originalState{5, 5, 10, 10, 10, 5, 50};
    assert(SaveEngine::saveState(originalState));
    assert(SaveEngine::hasValidSave());

    GameState loadedState{};
    assert(SaveEngine::loadState(loadedState));
    assert(originalState.hp == loadedState.hp);
    assert(originalState.maxHp == loadedState.maxHp);
    assert(originalState.x == loadedState.x);
    assert(originalState.y == loadedState.y);
    assert(originalState.atk == loadedState.atk);
    assert(originalState.def == loadedState.def);
    assert(originalState.chanceToRun == loadedState.chanceToRun);
    assert(originalState.difficulty == loadedState.difficulty);

    SaveEngine::deleteSave();
    assert(!SaveEngine::hasValidSave());
    std::filesystem::remove_all(dataHome);

    std::cout << "[INFO] Persistence systems verified." << std::endl;
    std::cout << "--- PHASE 6 TEST COMPLETE ---" << std::endl;
    return 0;
}
