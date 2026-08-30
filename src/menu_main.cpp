#include "menu_main.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include "save_engine.hpp"
#include <iostream>
#include <vector>

MenuMain::Difficulty MenuMain::displayMenu() {
    bool canContinue = SaveEngine::hasValidSave();
    std::vector<std::string> opts;
    if (canContinue) {
        opts = {"Continue", "New Game (Easy Mode)", "New Game (Hard Mode)", "Contribute", "Exit"};
    } else {
        opts = {"Play (Easy Mode)", "Play (Hard Mode)", "Contribute", "Exit"};
    }
    
    int selected = 0;
    
    while (true) {
        Terminal::clearScreen();
        UIRenderer::renderMenu("OBSFURIA MAIN MENU", opts, selected);
        
        int key = Terminal::readKey();
        if (key == Terminal::ARROW_UP) {
            selected = (selected - 1 + opts.size()) % opts.size();
        } else if (key == Terminal::ARROW_DOWN) {
            selected = (selected + 1) % opts.size();
        } else if (key == Terminal::ENTER) {
            if (canContinue) {
                if (selected == 0) return Difficulty::CONTINUE;
                if (selected == 1) return Difficulty::EASY;
                if (selected == 2) return Difficulty::HARD;
                if (selected == 3) { displayContribute(); continue; }
                if (selected == 4) return Difficulty::EXIT;
            } else {
                if (selected == 0) return Difficulty::EASY;
                if (selected == 1) return Difficulty::HARD;
                if (selected == 2) { displayContribute(); continue; }
                if (selected == 3) return Difficulty::EXIT;
            }
        }
    }
}

void MenuMain::displayContribute() {
    Terminal::clearScreen();
    std::cout << "[INFO] Please view the Contribute.md file for details on contributing!\r\n";
    std::cout << "Press any key to return...\r\n";
    Terminal::readKey();
}
