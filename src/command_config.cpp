#include "command_config.hpp"
#include "spatial_manager.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

fs::path CommandConfig::getConfigPath() {
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig && *xdgConfig) {
        return fs::path(xdgConfig) / "obsfuria" / "commands.conf";
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return fs::path(home) / ".config" / "obsfuria" / "commands.conf";
    }
    return "./obsfuria_commands.conf";
}

fs::path CommandConfig::getDefaultAssetPath() {
    return "assets/default_commands.txt";
}

fs::path CommandConfig::getCrazyAssetPath() {
    return "assets/crazy_commands.txt";
}

std::vector<CommandEntry> CommandConfig::loadCommands(const fs::path& path) {
    std::vector<CommandEntry> commands;
    std::ifstream in(path);
    if (!in) return commands;
    
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find('|');
        if (pos != std::string::npos) {
            CommandEntry e;
            e.name = line.substr(0, pos);
            e.executeCmd = line.substr(pos + 1);
            // Trim whitespace
            auto trim = [](std::string& s) {
                size_t start = s.find_first_not_of(" \t\r\n");
                size_t end = s.find_last_not_of(" \t\r\n");
                if (start == std::string::npos) { s.clear(); return; }
                s = s.substr(start, end - start + 1);
            };
            trim(e.name);
            trim(e.executeCmd);
            if (!e.name.empty()) commands.push_back(std::move(e));
        }
    }
    return commands;
}

bool CommandConfig::saveCommands(const fs::path& path, const std::vector<CommandEntry>& commands) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    if (!out) return false;
    for (const auto& e : commands) {
        out << e.name << "|" << e.executeCmd << "\n";
    }
    return out.good();
}

std::vector<CommandEntry> CommandConfig::getDefaultCommands() {
    return loadCommands(getDefaultAssetPath());
}

std::vector<CommandEntry> CommandConfig::getCrazyCommands() {
    return loadCommands(getCrazyAssetPath());
}

std::map<std::string, std::string> CommandConfig::buildArsenalMap(const std::vector<CommandEntry>& commands) {
    std::map<std::string, std::string> arsenal;
    for (const auto& e : commands) {
        std::string output;
        FILE* fp = popen(e.executeCmd.c_str(), "r");
        if (fp) {
            char buf[256];
            while (fgets(buf, sizeof(buf), fp)) output += buf;
            pclose(fp);
        }
        if (output.empty()) {
            output = "bash: " + e.executeCmd.substr(0, e.executeCmd.find(' ')) + ": command not found or output restricted.";
        }
        // Limit to 25 lines
        size_t lines = 0;
        std::string limited;
        for (char c : output) {
            limited += c;
            if (c == '\n' && ++lines >= 25) break;
        }
        arsenal[e.name] = limited;
    }
    arsenal["obsfuria_version"] = "1.0.0";
    return arsenal;
}

bool CommandConfig::regenerateArsenal(const std::vector<CommandEntry>& commands) {
    auto arsenal = buildArsenalMap(commands);
    
    std::string outFile = "include/arsenal.hpp";
    std::ofstream out(outFile);
    if (!out) return false;
    
    out << "#ifndef ARSENAL_HPP\n";
    out << "#define ARSENAL_HPP\n\n";
    out << "#include <string>\n";
    out << "#include <map>\n\n";
    out << "class Arsenal {\n";
    out << "public:\n";
    out << "    static const std::map<std::string, std::string>& getMockCommands() {\n";
    out << "        static const std::map<std::string, std::string> commands = {\n";
    
    for (const auto& [name, output] : arsenal) {
        out << "            {\"" << name << "\", R\"obsfuria(";
        out << output;
        out << ")obsfuria\"},\n";
    }
    
    out << "        };\n";
    out << "        return commands;\n";
    out << "    };\n";
    out << "};\n\n";
    out << "#endif\n";
    
    return out.good();
}