#include "../include/terminal.hpp"
#include <iostream>

/*
 * Tests terminal clear screen and cursor logic. Cannot easily test readKey blocking.
 */
int main() {
    std::cout << "--- STARTING PHASE 0 TEST ---" << std::endl;
    // We won't block execution for unit tests, just ensure link is valid.
    Terminal::clearScreen();
    Terminal::moveCursor(1, 1);
    std::cout << "Phase 0 Terminal Library Linked Successfully." << std::endl;
    return 0;
}
