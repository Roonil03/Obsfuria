#include "terminal.hpp"
#include "menu_main.hpp"
#include "game_loop.hpp"
#include <iostream>
#include <exception>

int main() {
    Terminal::installSignalHandlers();
    if (!Terminal::enableRawMode()) {
        std::cerr << "Obsfuria requires an interactive terminal.\n";
        return 1;
    }
    struct RestoreTerminal {
        ~RestoreTerminal() { Terminal::disableRawMode(); }
    } restoreTerminal;
    
    try {
        while (true) {
            MenuMain::Difficulty diff = MenuMain::displayMenu();
            if (diff == MenuMain::Difficulty::EXIT) {
                break;
            } else {
                GameLoop::start(diff);
            }
        }
    } catch (const std::exception&) {
        std::cerr << "\r\nObsfuria could not continue.\r\n";
        return 1;
    }
    
    Terminal::clearScreen();
    return 0;
}
