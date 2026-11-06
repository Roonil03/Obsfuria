#include "../include/stat_calc.hpp"
#include "../include/reward_system.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "--- STARTING PHASE 4 TEST ---" << std::endl;
    
    int stat = StatCalc::generateStat(100, 0, 255);
    assert(stat >= 50 && stat <= 150);
    
    assert(RewardSystem::calculateExp(2) == 30);
    
    // We omit BattleLoop test here to avoid blocking on Terminal::readKey()
    std::cout << "[INFO] Deviation stats and reward system validated." << std::endl;
    std::cout << "--- PHASE 4 TEST COMPLETE ---" << std::endl;
    return 0;
}
