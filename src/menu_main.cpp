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
    std::cout << "[INFO] Please view the Contribute.md file for details on contributing!\r\n";
    std::cout << "[INFO] Also see INSTALL.md for installation and CONTRIBUTING.md for guidelines.\r\n";
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
                    if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds)) {
                        CommandConfig::regenerateArsenal(cmds);
                        Terminal::clearScreen();
                        std::cout << "\x1b[32mSafe list applied and arsenal regenerated!\x1b[0m\r\n";
                    } else {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mFailed to apply safe list.\x1b[0m\r\n";
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 1) { // Crazy List
                    auto cmds = CommandConfig::getCrazyCommands();
                    if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds)) {
                        CommandConfig::regenerateArsenal(cmds);
                        Terminal::clearScreen();
                        std::cout << "\x1b[33mCrazy list applied and arsenal regenerated!\x1b[0m\r\n";
                        std::cout << "\x1b[33mNote: These are MOCKED commands - they display output but don't execute.\x1b[0m\r\n";
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
                    std::getline(std::cin, path);
                    if (!path.empty()) {
                        auto cmds = CommandConfig::loadCommands(path);
                        if (!cmds.empty()) {
                            if (CommandConfig::saveCommands(CommandConfig::getConfigPath(), cmds)) {
                                CommandConfig::regenerateArsenal(cmds);
                                Terminal::clearScreen();
                                std::cout << "\x1b[32mCustom list imported (" << cmds.size() << " commands)!\x1b[0m\r\n";
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
                    Terminal::clearScreen();
                    std::cout << "Current list will be opened for editing. Not implemented yet.\r\n";
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 4) { // Reset to Default
                    std::remove(CommandConfig::getConfigPath().c_str());
                    auto cmds = CommandConfig::getDefaultCommands();
                    CommandConfig::regenerateArsenal(cmds);
                    Terminal::clearScreen();
                    std::cout << "\x1b[32mReset to default command list!\x1b[0m\r\n";
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
    std::cout << "\x1b[31mWARNING: Adding commands will regenerate the arsenal and RESTART the game.\x1b[0m\r\n";
    std::cout << "\x1b[31mAny unsaved progress will be lost. Continue? (y/N): \x1b[0m";
    
    char confirm;
    std::cin >> confirm;
    std::cin.ignore(10000, '\n');
    
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
        std::getline(std::cin, name);
        
        if (name.empty()) {
            break;
        }
        
        std::cout << "Command to execute: ";
        std::string cmd;
        std::getline(std::cin, cmd);
        
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
        CommandConfig::regenerateArsenal(currentCmds);
        
        Terminal::clearScreen();
        std::cout << "\x1b[32mCommands saved! Arsenal regenerated with " << currentCmds.size() << " commands.\x1b[0m\r\n";
        std::cout << "\x1b[33mRestarting game...\x1b[0m\r\n";
        
        // Restart the game by exec'ing the binary
        Terminal::disableRawMode();
        
        // Get the binary path
        char exePath[1024];
        ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
        if (len != -1) {
            exePath[len] = '\0';
            execl(exePath, exePath, (char*)nullptr);
        } else {
            // Fallback
            execl("./bin/Obsfuria", "./bin/Obsfuria", (char*)nullptr);
        }
        
        // If exec fails
        std::cerr << "Failed to restart game. Please run manually.\r\n";
        std::exit(1);
    } else {
        Terminal::clearScreen();
        std::cout << "\x1b[31mFailed to save commands.\x1b[0m\r\n";
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
    }
}
