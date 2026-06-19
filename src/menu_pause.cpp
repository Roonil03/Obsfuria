#include "menu_pause.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include <iostream>
#include <vector>

bool MenuPause::displayPause() {
    std::vector<std::string> opts = {"Resume", "Quit to Main Menu"};
    int selected = 0;
    
    while (true) {
        Terminal::clearScreen();
        UIRenderer::renderMenu("PAUSED", opts, selected);
        
        int key = Terminal::readKey();
        if (key == Terminal::ARROW_UP) {
            selected = (selected - 1 + opts.size()) % opts.size();
        } else if (key == Terminal::ARROW_DOWN) {
            selected = (selected + 1) % opts.size();
        } else if (key == Terminal::ENTER) {
            return selected == 0; // True = Resume, False = Quit
        }
    }
}
