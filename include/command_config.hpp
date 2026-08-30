#ifndef COMMAND_CONFIG_HPP
#define COMMAND_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

struct CommandEntry {
    std::string name;
    std::string executeCmd;
};

class CommandConfig {
public:
    static fs::path getConfigPath();
    static fs::path getDefaultAssetPath();
    static fs::path getCrazyAssetPath();
    
    static std::vector<CommandEntry> loadCommands(const fs::path& path);
    static bool saveCommands(const fs::path& path, const std::vector<CommandEntry>& commands);
    static std::vector<CommandEntry> getDefaultCommands();
    static std::vector<CommandEntry> getCrazyCommands();
    
    static bool regenerateArsenal(const std::vector<CommandEntry>& commands);
    static std::map<std::string, std::string> buildArsenalMap(const std::vector<CommandEntry>& commands);
};

#endif // COMMAND_CONFIG_HPP