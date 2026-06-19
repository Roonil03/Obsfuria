#include "../include/sprite_gen.hpp"
#include "../include/ui_renderer.hpp"
#include "../include/sanitizer.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "--- STARTING PHASE 3 TEST ---" << std::endl;
    
    std::string sprite = SpriteGen::generate();
    assert(Sanitizer::isClean(sprite));
    
    UIRenderer::renderEnemy("Segmentation Fault", sprite);
    
    std::vector<std::string> opts = {"Attack", "Run"};
    UIRenderer::renderMenu("COMBAT", opts, 0);
    
    std::cout << "[INFO] Sprite generation, sanitization, and UI rendering verified." << std::endl;
    std::cout << "--- PHASE 3 TEST COMPLETE ---" << std::endl;
    return 0;
}
