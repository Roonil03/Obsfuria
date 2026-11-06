#include "command_config.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>
#include <unistd.h>

int main() {
    namespace fs = std::filesystem;
    const fs::path original = fs::current_path();
    std::string pattern = (fs::temp_directory_path() / "obsfuria-command-config-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    char* scratchName = mkdtemp(writable.data());
    if (!scratchName) return 1;
    const fs::path scratch = scratchName;
    if (chdir(scratch.c_str()) != 0) {
        std::cerr << "Could not change to isolated working directory\n";
        return 1;
    }

    const auto safePath = CommandConfig::getDefaultAssetPath();
    const auto crazyPath = CommandConfig::getCrazyAssetPath();
    const auto safe = CommandConfig::getDefaultCommands();
    const auto crazy = CommandConfig::getCrazyCommands();
    chdir(original.c_str());
    fs::remove_all(scratch);

    if (safe.empty() || crazy.empty() || !fs::is_regular_file(safePath) || !fs::is_regular_file(crazyPath)) {
        std::cerr << "Command assets were not found outside the repository working directory\n";
        return 1;
    }
    std::cout << "Command assets resolve outside the repository working directory\n";
    return 0;
}
