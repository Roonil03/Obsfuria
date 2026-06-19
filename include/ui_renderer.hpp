#ifndef UI_RENDERER_HPP
#define UI_RENDERER_HPP

#include <string>
#include <vector>

class UIRenderer {
public:
    static void renderEnemy(const std::string& enemyName, const std::string& sprite);
    static void renderMenu(const std::string& title, const std::vector<std::string>& options, int selected);
};

#endif // UI_RENDERER_HPP
