#include "sprite_gen.hpp"
#include <random>

std::string SpriteGen::generate() {
    const std::string symbols = "[]|_*~%&?@$!^()";
    std::string result;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, symbols.size() - 1);

    for (int y = 0; y < 6; ++y) {
        for (int x = 0; x < 6; ++x) {
            result += symbols[dist(gen)];
        }
        result += '\n';
    }
    return result;
}
