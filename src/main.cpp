#include "terminal.hpp"
#include "menu_main.hpp"
#include "game_loop.hpp"
#include <iostream>

int main() {
    Terminal::enableRawMode();
    
    while (true) {
        MenuMain::Difficulty diff = MenuMain::displayMenu();
        if (diff == MenuMain::Difficulty::EXIT) {
            break;
        } else {
            GameLoop::start(diff);
        }
    }
    
    Terminal::clearScreen();
    Terminal::disableRawMode();
    return 0;
}
