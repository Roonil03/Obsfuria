#include "ui_renderer.hpp"
#include "terminal_text.hpp"
#include <iostream>
#include <vector>

using TerminalText::escape;

void UIRenderer::renderEnemy(const std::string& enemyName, const std::string& sprite) {
    std::cout << "\x1b[31m"; // Red color
    std::cout << "====================" << std::endl;
    std::cout << " ENEMY: " << escape(enemyName) << std::endl;
    std::cout << "====================" << std::endl;
    std::cout << "\x1b[0m"; // Reset color
    std::cout << sprite << std::endl;
}

void UIRenderer::renderMenu(const std::string& title, const std::vector<std::string>& options, int selected) {
    std::cout << "\x1b[36m--- " << escape(title) << " ---\x1b[0m\r\n";
    for (size_t i = 0; i < options.size(); ++i) {
        if ((int)i == selected) {
            std::cout << "\x1b[32m> " << escape(options[i]) << " <\x1b[0m\r\n"; // Green selected
        } else {
            std::cout << "  " << escape(options[i]) << "\r\n";
        }
    }
}
