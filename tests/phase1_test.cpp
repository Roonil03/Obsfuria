#include "../include/arsenal.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "--- STARTING PHASE 1 TEST ---" << std::endl;
    auto commands = Arsenal::getMockCommands();
    assert(!commands.empty());
    assert(commands.find("ls") != commands.end());
    std::cout << "Phase 1 Static Dictionary Verified." << std::endl;
    return 0;
}
