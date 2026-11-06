#include "menu_main.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include "save_engine.hpp"
#include "command_config.hpp"
#include <iostream>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <sstream>

static bool readLineOrEscape(std::string& value, bool escapeCancels = true) {
    value.clear();
    while (true) {
        const int key = Terminal::readKey();
        if (key == Terminal::ESC) {
            if (escapeCancels) return false;
            continue;
        }
        if (key == Terminal::ENTER || key == '\n') {
            std::cout << "\r\n";
            return true;
        }
        if (key == 8 || key == 127) {
            if (!value.empty()) {
                value.pop_back();
                std::cout << "\b \b";
            }
            continue;
        }
        if (key >= 32 && key <= 255) {
            value.push_back(static_cast<char>(key));
            std::cout << static_cast<char>(key);
        }
    }
}

static std::string clickableUserListPath() {
    namespace fs = std::filesystem;
    const fs::path absolute = fs::absolute(CommandConfig::getUserListPath()).lexically_normal();
    const fs::path relative = absolute.lexically_relative(fs::current_path());
    const std::string label = (relative.empty() ? absolute : relative).string();

    std::ostringstream uri;
    uri << "file://";
    for (unsigned char ch : absolute.string()) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '/' || ch == '-' || ch == '_' ||
            ch == '.' || ch == '~') {
            uri << static_cast<char>(ch);
        } else {
            uri << '%' << std::uppercase << std::hex << std::setw(2)
                << std::setfill('0') << static_cast<int>(ch) << std::nouppercase << std::dec;
        }
    }
    return "\x1b]8;;" + uri.str() + "\x1b\\" + label + "\x1b]8;;\x1b\\";
}

MenuMain::Difficulty MenuMain::displayMenu() {
    bool canContinue = SaveEngine::hasValidSave();
    std::vector<std::string> opts;
    if (canContinue) {
        opts = {"Continue", "New Game (Easy Mode)", "New Game (Hard Mode)", "Reload Enemy Commands", "Add More Commands", "Contribute", "Exit"};
    } else {
        opts = {"Play (Easy Mode)", "Play (Hard Mode)", "Reload Enemy Commands", "Add More Commands", "Contribute", "Exit"};
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
            "Safe List",
            "User List",
            "Import into User List",
            "Edit User List",
            "Reset User List to Safe List",
            "Back"
        };
        int selected = 0;
        
        while (true) {
            Terminal::clearScreen();
            std::cout << "\x1b[36m--- RELOAD ENEMY COMMANDS ---\x1b[0m\r\n";
            std::cout << "Active list: "
                      << (CommandConfig::getActiveList() == CommandConfig::List::SAFE ? "Safe List" : "User List")
                      << "\r\n";
            std::cout << "Editable user list: " << clickableUserListPath() << "\r\n";
            const auto userCommands = CommandConfig::loadCommands(CommandConfig::getUserListPath());
            std::cout << "User commands: " << userCommands.size() << "\r\n\r\n";
            std::cout << "\r\n";
            UIRenderer::renderMenu("COMMAND LIST OPTIONS", opts, selected);
            
            int key = Terminal::readKey();
            if (key == Terminal::ARROW_UP) {
                selected = (selected - 1 + opts.size()) % opts.size();
            } else if (key == Terminal::ARROW_DOWN) {
                selected = (selected + 1) % opts.size();
            } else if (key == Terminal::ESC) {
                return;
            } else if (key == Terminal::ENTER) {
                if (selected == 0) { // Safe List
                    auto cmds = CommandConfig::getDefaultCommands();
                    if (cmds.empty()) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mSafe list is empty; configuration was not changed.\x1b[0m\r\n";
                    } else if (CommandConfig::setActiveList(CommandConfig::List::SAFE)) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[32mSafe List selected. It will be used in the next game.\x1b[0m\r\n";
                    } else {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mFailed to select Safe List.\x1b[0m\r\n";
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    return;
                } else if (selected == 1) { // User List
                    const auto cmds = CommandConfig::loadCommands(CommandConfig::getUserListPath());
                    if (cmds.empty()) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mUser List is empty; add or import commands before selecting it.\x1b[0m\r\n";
                        std::cout << "Editable user list: " << clickableUserListPath() << "\r\n";
                        std::cout << "Press any key to continue...\r\n";
                        Terminal::readKey();
                        break;
                    } else if (CommandConfig::setActiveList(CommandConfig::List::USER)) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[32mUser List selected. It will be used in the next game.\x1b[0m\r\n";
                    } else {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mFailed to select User List.\x1b[0m\r\n";
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    return;
                } else if (selected == 2) { // Import Custom File
                    Terminal::clearScreen();
                    std::cout << "Enter path to custom command file (format: name|command per line):\r\n";
                    std::cout << "> ";
                    std::string path;
                    if (!readLineOrEscape(path)) break;
                    if (!path.empty()) {
                        auto cmds = CommandConfig::loadCommands(path);
                        if (!cmds.empty()) {
                            if (CommandConfig::saveCommands(CommandConfig::getUserListPath(), cmds)) {
                                Terminal::clearScreen();
                                std::cout << "\x1b[32mImported " << cmds.size() << " commands into the User List.\x1b[0m\r\n";
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
                    if (exists(CommandConfig::getUserListPath())) {
                        cmds = CommandConfig::loadCommands(CommandConfig::getUserListPath());
                    } else {
                        cmds = CommandConfig::getDefaultCommands();
                    }

                    bool changed = false;
                    bool canceled = false;
                    while (true) {
                        Terminal::clearScreen();
                        std::cout << "--- EDIT CURRENT COMMAND LIST ---\r\n";
                        for (std::size_t i = 0; i < cmds.size(); ++i) {
                            std::cout << (i + 1) << ") " << cmds[i].name << " | "
                                      << cmds[i].executeCmd << "\r\n";
                        }
                        std::cout << "\r\nEnter a command number to edit, or 0 to finish: ";
                        std::string choice;
                        if (!readLineOrEscape(choice)) {
                            canceled = true;
                            break;
                        }
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
                        std::string name;
                        if (!readLineOrEscape(name)) {
                            canceled = true;
                            break;
                        }
                        std::cout << "New command: ";
                        std::string command;
                        if (!readLineOrEscape(command)) {
                            canceled = true;
                            break;
                        }
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

                    if (canceled) break;

                    if (changed) {
                        const bool saved = CommandConfig::saveCommands(CommandConfig::getUserListPath(), cmds);
                        Terminal::clearScreen();
                        if (saved) {
                            std::cout << "\x1b[32mUser List saved. It will be used when selected.\x1b[0m\r\n";
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
                        std::cout << "\x1b[31mSafe List is empty; User List was not changed.\x1b[0m\r\n";
                    } else if (CommandConfig::saveCommands(CommandConfig::getUserListPath(), cmds)) {
                        std::cout << "\x1b[32mUser List reset to the Safe List contents.\x1b[0m\r\n";
                    } else {
                        std::cout << "\x1b[31mFailed to reset the User List.\x1b[0m\r\n";
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
    if (exists(CommandConfig::getUserListPath())) {
        currentCmds = CommandConfig::loadCommands(CommandConfig::getUserListPath());
    } else {
        currentCmds = CommandConfig::getDefaultCommands();
    }
    
    Terminal::clearScreen();
    std::cout << "\x1b[33m=== ADD MORE COMMANDS ===\x1b[0m\r\n";
    std::cout << "\x1b[33mNew commands will be available when you start a game.\x1b[0m\r\n";
    std::cout << "Editable user list: " << clickableUserListPath() << "\r\n";
    std::cout << "Continue? (y/N): ";
    
    std::string answer;
    readLineOrEscape(answer, false);
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
        readLineOrEscape(name, false);
        
        if (name.empty()) {
            break;
        }
        
        std::cout << "Command to execute: ";
        std::string cmd;
        readLineOrEscape(cmd, false);
        
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
    
    // Save the user list; choosing it as active is a separate menu action.
    if (CommandConfig::saveCommands(CommandConfig::getUserListPath(), currentCmds)) {
        Terminal::clearScreen();
        std::cout << "\x1b[32mSaved " << currentCmds.size() << " commands to the User List.\x1b[0m\r\n";
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
    } else {
        Terminal::clearScreen();
        std::cout << "\x1b[31mFailed to save commands.\x1b[0m\r\n";
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
    }
}
