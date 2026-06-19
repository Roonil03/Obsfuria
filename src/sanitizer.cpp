#include "sanitizer.hpp"
#include <cctype>

bool Sanitizer::isClean(const std::string& sprite) {
    for (char c : sprite) {
        if (std::isalnum(c)) {
            return false;
        }
    }
    return true;
}
