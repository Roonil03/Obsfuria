#include "menu_main.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include "save_engine.hpp"
#include "command_config.hpp"
#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unistd.h>

static std::string readLine() {
    Terminal::disableRawMode();
    std::string value;
    std::getline(std::cin, value);
    Terminal::enableRawMode();
    return value;
}

MenuMain::Difficulty MenuMain::displayMenu() {
    bool canContinue = SaveEngine::hasValidSave();
    std::vector<std::string> opts;
    if (canContinue) {
        opts = {"Continue", "New Game (Easy Mode)", "New Game (Hard Mode)", "Change Command List", "Add More Commands", "Contribute", "Exit"};
    } else {
        opts = {"Play (Easy Mode)", "Play (Hard Mode)", "Change Command List", "Add More Commands", "Contribute", "Exit"};
    }
    
    int selected = 0;
    
    while (true) {
        Terminal::clearScreen();
        UIRenderer::renderMenu("OBSFURIA MAIN MENU", opts, selected);
        
        int key = Terminal::readKey();
        if (key == Terminal::ARROW_UP) {
            selected = (selected - 1 + opts.size()) % opts.size();
        } else if (key == Terminal::ARROW_DOWN) {
            selected = (selected + 1) % opts.size();
        } else if (key == Terminal::ENTER) {
            if (canContinue) {
                if (selected == 0) return Difficulty::CONTINUE;
                if (selected == 1) return Difficulty::EASY;
                if (selected == 2) return Difficulty::HARD;
                if (selected == 3) { displayChangeCommands(); continue; }
                if (selected == 4) { displayAddCommands(); continue; }
                if (selected == 5) { displayContribute(); continue; }
                if (selected == 6) return Difficulty::EXIT;
            } else {
                if (selected == 0) return Difficulty::EASY;
                if (selected == 1) return Difficulty::HARD;
                if (selected == 2) { displayChangeCommands(); continue; }
                if (selected == 3) { displayAddCommands(); continue; }
                if (selected == 4) { displayContribute(); continue; }
                if (selected == 5) return Difficulty::EXIT;
            }
        }
    }
}

void MenuMain::displayContribute() {
    Terminal::clearScreen();
    std::cout << "[INFO] See CONTRIBUTING.md for contribution guidelines.\r\n";
    std::cout << "[INFO] See INSTALL.md for installation help.\r\n";
    std::cout << "Press any key to return...\r\n";
    Terminal::readKey();
}

void MenuMain::displayChangeCommands() {
    using namespace std::filesystem;
    
    while (true) {
        Terminal::clearScreen();
        std::vector<std::string> opts = {
            "Use Safe List (default)",
            "Use Crazy List",
            "Import Custom File",
            "Edit Current List",
            "Reset to Default",
            "Back"
        };
        int selected = 0;
        
        while (true) {
            Terminal::clearScreen();
            std::cout << "\x1b[36m--- CHANGE COMMAND LIST ---\x1b[0m\r\n";
            std::cout << "Current config: " << CommandConfig::getConfigPath() << "\r\n";
            if (exists(CommandConfig::getConfigPath())) {
                auto cmds = CommandConfig::loadCommands(CommandConfig::getConfigPath());
                std::cout << "Commands loaded: " << cmds.size() << "\r\n";
            } else {
                std::cout << "Using built-in defaults (" << CommandConfig::getDefaultCommands().size() << " commands)\r\n";
            }
            std::cout << "\r\n";
            UIRenderer::renderMenu("COMMAND LIST OPTIONS", opts, selected);
            
            int key = Terminal::readKey();
            if (key == Terminal::ARROW_UP) {
                selected = (selected - 1 + opts.size()) % opts.size();
            } else if (key == Terminal::ARROW_DOWN) {
                selected = (selected + 1) % opts.size();
            } else if (key == Terminal::ENTER) {
                if (selected == 0) { // Safe List
                    auto cmds = CommandConfig::getDefaultCommands();
                    if (cmds.empty()) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mSafe list is empty; configuration was not changed.\x1b[0m\r\n";
                    } else if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds)) {
                        const bool regenerated = CommandConfig::regenerateArsenal(cmds);
                        Terminal::clearScreen();
                        std::cout << (regenerated ? "\x1b[32mSafe list applied and arsenal regenerated!\x1b[0m\r\n"
                                                 : "\x1b[31mSafe list saved, but arsenal regeneration failed.\x1b[0m\r\n");
                    } else {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mFailed to apply safe list.\x1b[0m\r\n";
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 1) { // Crazy List
                    auto cmds = CommandConfig::getCrazyCommands();
                    if (cmds.empty()) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mCrazy list is empty; configuration was not changed.\x1b[0m\r\n";
                    } else if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds)) {
                        const bool regenerated = CommandConfig::regenerateArsenal(cmds);
                        Terminal::clearScreen();
                        if (regenerated) {
                            std::cout << "\x1b[33mCrazy list applied and arsenal regenerated!\x1b[0m\r\n";
                            std::cout << "\x1b[33mCommands run while capturing output; encounters display the captured result.\x1b[0m\r\n";
                        } else {
                            std::cout << "\x1b[31mCrazy list saved, but arsenal regeneration failed.\x1b[0m\r\n";
                        }
                    } else {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mFailed to apply crazy list.\x1b[0m\r\n";
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 2) { // Import Custom File
                    Terminal::clearScreen();
                    std::cout << "Enter path to custom command file (format: name|command per line):\r\n";
                    std::cout << "> ";
                    std::string path;
                    path = readLine();
                    if (!path.empty()) {
                        auto cmds = CommandConfig::loadCommands(path);
                        if (!cmds.empty()) {
                            if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds)) {
                                Terminal::clearScreen();
                                if (CommandConfig::regenerateArsenal(cmds)) {
                                    std::cout << "\x1b[32mCustom list imported (" << cmds.size() << " commands)!\x1b[0m\r\n";
                                } else {
                                    std::cout << "\x1b[31mCustom list saved, but arsenal regeneration failed.\x1b[0m\r\n";
                                }
                            } else {
                                Terminal::clearScreen();
                                std::cout << "\x1b[31mFailed to save custom list.\x1b[0m\r\n";
                            }
                        } else {
                            Terminal::clearScreen();
                            std::cout << "\x1b[31mNo valid commands found in file.\x1b[0m\r\n";
                        }
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 3) { // Edit Current List
                    std::vector<CommandEntry> cmds;
                    if (exists(CommandConfig::getConfigPath())) {
                        cmds = CommandConfig::loadCommands(CommandConfig::getConfigPath());
                    } else {
                        cmds = CommandConfig::getDefaultCommands();
                    }

                    bool changed = false;
                    while (true) {
                        Terminal::clearScreen();
                        std::cout << "--- EDIT CURRENT COMMAND LIST ---\r\n";
                        for (std::size_t i = 0; i < cmds.size(); ++i) {
                            std::cout << (i + 1) << ") " << cmds[i].name << " | "
                                      << cmds[i].executeCmd << "\r\n";
                        }
                        std::cout << "\r\nEnter a command number to edit, or 0 to finish: ";
                        const std::string choice = readLine();
                        if (choice == "0" || choice.empty()) break;

                        std::size_t index = 0;
                        try {
                            std::size_t parsed = 0;
                            const unsigned long value = std::stoul(choice, &parsed);
                            if (parsed != choice.size() || value == 0 || value > cmds.size()) {
                                throw std::out_of_range("command index");
                            }
                            index = static_cast<std::size_t>(value - 1);
                        } catch (...) {
                            std::cout << "Invalid command number. Press any key to continue...\r\n";
                            Terminal::readKey();
                            continue;
                        }

                        std::cout << "New name: ";
                        const std::string name = readLine();
                        std::cout << "New command: ";
                        const std::string command = readLine();
                        if (name.empty() || command.empty()) {
                            std::cout << "Name and command must both be nonempty. Press any key...\r\n";
                            Terminal::readKey();
                            continue;
                        }
                        bool duplicate = false;
                        for (std::size_t i = 0; i < cmds.size(); ++i) {
                            if (i != index && cmds[i].name == name) duplicate = true;
                        }
                        if (duplicate) {
                            std::cout << "That command name already exists. Press any key...\r\n";
                            Terminal::readKey();
                            continue;
                        }
                        cmds[index] = {name, command};
                        changed = true;
                        std::cout << "Command updated. Press any key to continue...\r\n";
                        Terminal::readKey();
                    }

                    if (changed) {
                        const bool saved = CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds);
                        const bool regenerated = saved && CommandConfig::regenerateArsenal(cmds);
                        Terminal::clearScreen();
                        if (regenerated) {
                            std::cout << "\x1b[32mCommand list saved and arsenal regenerated.\x1b[0m\r\n";
                        } else if (saved) {
                            std::cout << "\x1b[31mCommand list saved, but arsenal regeneration failed.\x1b[0m\r\n";
                        } else {
                            std::cout << "\x1b[31mFailed to save command list; changes were not persisted.\x1b[0m\r\n";
                        }
                        std::cout << "Press any key to continue...\r\n";
                        Terminal::readKey();
                    }
                    break;
                } else if (selected == 4) { // Reset to Default
                    auto cmds = CommandConfig::getDefaultCommands();
                    Terminal::clearScreen();
                    if (cmds.empty()) {
                        std::cout << "\x1b[31mDefault command list is empty; configuration was not changed.\x1b[0m\r\n";
                    } else if (!CommandConfig::regenerateArsenal(cmds)) {
                        std::cout << "\x1b[31mFailed to regenerate default arsenal; configuration was not reset.\x1b[0m\r\n";
                    } else if (std::remove(CommandConfig::getConfigPath().c_str()) == 0 ||
                               !exists(CommandConfig::getConfigPath())) {
                        std::cout << "\x1b[32mReset to default command list!\x1b[0m\r\n";
                    } else {
                        std::cout << "\x1b[31mDefault arsenal regenerated, but failed to remove saved configuration.\x1b[0m\r\n";
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 5) { // Back
                    return;
                }
            }
        }
    }
}

void MenuMain::displayAddCommands() {
    using namespace std::filesystem;
    
    // Load current commands
    std::vector<CommandEntry> currentCmds;
    if (exists(CommandConfig::getConfigPath())) {
        currentCmds = CommandConfig::loadCommands(CommandConfig::getConfigPath());
    } else {
        currentCmds = CommandConfig::getDefaultCommands();
    }
    
    Terminal::clearScreen();
    std::cout << "\x1b[33m=== ADD MORE COMMANDS ===\x1b[0m\r\n";
    std::cout << "\x1b[33mNew commands will be available when you start a game.\x1b[0m\r\n";
    std::cout << "Continue? (y/N): ";
    
    std::string answer = readLine();
    char confirm = answer.empty() ? 'n' : answer.front();
    
    if (confirm != 'y' && confirm != 'Y') {
        return;
    }
    
    while (true) {
        Terminal::clearScreen();
        std::cout << "\x1b[36m--- ADD COMMAND ---\x1b[0m\r\n";
        std::cout << "Current commands: " << currentCmds.size() << "\r\n";
        std::cout << "Format: name|command (e.g., mycmd|echo hello)\r\n";
        std::cout << "Enter empty name to finish.\r\n\r\n";
        
        std::cout << "Command name: ";
        std::string name;
        name = readLine();
        
        if (name.empty()) {
            break;
        }
        
        std::cout << "Command to execute: ";
        std::string cmd;
        cmd = readLine();
        
        if (cmd.empty()) {
            std::cout << "\x1b[31mCommand cannot be empty.\x1b[0m\r\n";
            std::cout << "Press any key to continue...\r\n";
            Terminal::readKey();
            continue;
        }
        
        // Check for duplicate
        bool duplicate = false;
        for (const auto& c : currentCmds) {
            if (c.name == name) {
                duplicate = true;
                break;
            }
        }
        
        if (duplicate) {
            std::cout << "\x1b[31mCommand name already exists.\x1b[0m\r\n";
            std::cout << "Press any key to continue...\r\n";
            Terminal::readKey();
            continue;
        }
        
        currentCmds.push_back({name, cmd});
        std::cout << "\x1b[32mAdded: " << name << " | " << cmd << "\x1b[0m\r\n";
        std::cout << "Press any key to continue...\r\n";
        Terminal::readKey();
    }
    
    if (currentCmds.empty()) {
        Terminal::clearScreen();
        std::cout << "\x1b[31mNo commands to save.\x1b[0m\r\n";
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
        return;
    }
    
    // Save and regenerate
    if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), currentCmds)) {
        Terminal::clearScreen();
        if (CommandConfig::regenerateArsenal(currentCmds)) {
            std::cout << "\x1b[32mSaved " << currentCmds.size() << " commands.\x1b[0m\r\n";
        } else {
            std::cout << "\x1b[31mCommands saved, but arsenal regeneration failed.\x1b[0m\r\n";
        }
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
    } else {
        Terminal::clearScreen();
        std::cout << "\x1b[31mFailed to save commands.\x1b[0m\r\n";
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
    }
}
