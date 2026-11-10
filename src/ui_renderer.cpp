#include "ui_renderer.hpp"
#include <iostream>
#include <vector>

namespace {
std::string escapeForDisplay(const std::string& value) {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string escaped;
    for (unsigned char ch : value) {
        if (ch < 0x20 || ch == 0x7f || (ch >= 0x80 && ch <= 0x9f)) {
            escaped += "\\x";
            escaped += hex[ch >> 4];
            escaped += hex[ch & 15];
        } else {
            escaped += static_cast<char>(ch);
        }
    }
    return escaped;
}
}

void UIRenderer::renderEnemy(const std::string& enemyName, const std::string& sprite) {
    std::cout << "\x1b[31m"; // Red color
    std::cout << "====================" << std::endl;
    std::cout << " ENEMY: " << escapeForDisplay(enemyName) << std::endl;
    std::cout << "====================" << std::endl;
    std::cout << "\x1b[0m"; // Reset color
    std::cout << sprite << std::endl;
}

void UIRenderer::renderMenu(const std::string& title, const std::vector<std::string>& options, int selected) {
    std::cout << "\x1b[36m--- " << title << " ---\x1b[0m\r\n";
    for (size_t i = 0; i < options.size(); ++i) {
        if ((int)i == selected) {
            std::cout << "\x1b[32m> " << options[i] << " <\x1b[0m\r\n"; // Green selected
        } else {
            std::cout << "  " << options[i] << "\r\n";
        }
    }
}
