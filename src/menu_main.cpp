#include "menu_main.hpp"
#include "terminal.hpp"
#include "terminal_text.hpp"
#include "ui_renderer.hpp"
#include "save_engine.hpp"
#include "command_config.hpp"
#include <iostream>
#include <vector>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

static bool readLineOrEscape(std::string& value, const std::string& prompt) {
    value.clear();
    const auto redraw = [&] {
        winsize window{};
        const std::size_t columns = ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == 0 && window.ws_col
            ? window.ws_col : 80;
        // A byte budget is conservative for UTF-8 column widths. Keep input on
        // one row, showing its tail when a long command would otherwise wrap.
        const std::size_t budget = columns - 1;
        const std::string label = prompt.substr(0, budget > 8 ? budget - 8 : budget / 2);
        std::size_t available = budget - label.size();
        std::string display = CommandConfig::escapeForDisplay(value);
        std::string more;
        if (display.size() > available) {
            if (available >= 3) { more = "..."; available -= 3; }
            std::size_t start = display.size() - available;
            while (start < display.size() && (static_cast<unsigned char>(display[start]) & 0xc0) == 0x80) ++start;
            display.erase(0, start);
        }
        std::cout << "\r\x1b[K" << label << more << display << std::flush;
    };
    redraw();
    while (true) {
        const int key = Terminal::readKey();
        if (key < 0) return false;
        if (key == Terminal::ESC) return false;
        if (key == Terminal::ENTER || key == '\n') {
            std::cout << "\r\n";
            return true;
        }
        if (key == 8 || key == 127) {
            if (!value.empty()) {
                TerminalText::popCharacter(value);
                redraw();
            }
            continue;
        }
        if (key >= 32 && key <= 255) {
            value.push_back(static_cast<char>(key));
            redraw();
        }
    }
}

static std::string clickableUserListPath() {
    namespace fs = std::filesystem;
    const fs::path userPath = CommandConfig::getUserListPath();
    std::error_code error;
    const fs::path absolute = fs::absolute(userPath, error).lexically_normal();
    if (error) return CommandConfig::escapeForDisplay(userPath.string());
    const fs::path workingDirectory = fs::current_path(error);
    const fs::path relative = error ? fs::path{} : absolute.lexically_relative(workingDirectory);
    const std::string label = CommandConfig::escapeForDisplay((relative.empty() ? absolute : relative).string());

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

static bool loadEditableCommands(std::vector<CommandEntry>& commands) {
    const auto outcome = CommandConfig::loadCommands(CommandConfig::getUserListPath(), commands);
    if (outcome == CommandConfig::LoadOutcome::MISSING) {
        commands = CommandConfig::getDefaultCommands();
    } else if (outcome != CommandConfig::LoadOutcome::OK) {
        std::cout << (outcome == CommandConfig::LoadOutcome::INVALID
            ? "User List contains malformed entries; the file was not changed. "
            : "User List is inaccessible; the file was not changed. ");
        std::cout << "Press any key to return...\r\n";
        Terminal::readKey();
        return false;
    }
    return true;
}

MenuMain::Difficulty MenuMain::displayMenu() {
    bool canContinue = SaveEngine::hasValidSave();
    const auto saveStatus = SaveEngine::lastLoadOutcome();
    std::vector<std::string> opts;
    if (canContinue) {
        opts = {"Continue", "New Game (Easy Mode)", "New Game (Hard Mode)", "Reload Enemy Commands", "Add More Commands", "Contribute", "Exit"};
    } else {
        opts = {"Play (Easy Mode)", "Play (Hard Mode)", "Reload Enemy Commands", "Add More Commands", "Contribute", "Exit"};
    }
    
    int selected = 0;
    
    while (true) {
        if (Terminal::inputClosed()) return Difficulty::EXIT;
        Terminal::clearScreen();
        if (!canContinue && saveStatus != SaveEngine::LoadOutcome::MISSING) {
            std::cout << "Saved game is "
                      << (saveStatus == SaveEngine::LoadOutcome::INVALID ? "invalid" : "inaccessible")
                      << "; it was not reset.\r\n";
        }
        UIRenderer::renderMenu("OBSFURIA MAIN MENU", opts, selected);
        
        int key = Terminal::readKey();
        if (key < 0) return Difficulty::EXIT;
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
        if (Terminal::inputClosed()) return;
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
            if (Terminal::inputClosed()) return;
            Terminal::clearScreen();
            std::cout << "\x1b[36m--- RELOAD ENEMY COMMANDS ---\x1b[0m\r\n";
            std::cout << "Active list: "
                      << (CommandConfig::getActiveList() == CommandConfig::List::SAFE ? "Safe List" : "User List")
                      << "\r\n";
            std::cout << "Editable user list: " << clickableUserListPath() << "\r\n";
            std::vector<CommandEntry> userCommands;
            const auto userOutcome = CommandConfig::loadCommands(CommandConfig::getUserListPath(), userCommands);
            std::cout << "User commands: ";
            if (userOutcome == CommandConfig::LoadOutcome::INVALID) std::cout << "malformed file";
            else if (userOutcome == CommandConfig::LoadOutcome::INACCESSIBLE) std::cout << "inaccessible file";
            else std::cout << userCommands.size();
            std::cout << "\r\n\r\n";
            std::cout << "\r\n";
            UIRenderer::renderMenu("COMMAND LIST OPTIONS", opts, selected);
            
            int key = Terminal::readKey();
            if (key < 0) return;
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
                    std::vector<CommandEntry> cmds;
                    const auto outcome = CommandConfig::loadCommands(CommandConfig::getUserListPath(), cmds);
                    if (outcome != CommandConfig::LoadOutcome::OK || cmds.empty()) {
                        Terminal::clearScreen();
                        std::cout << "\x1b[31mUser List must be readable and contain valid commands before selecting it.\x1b[0m\r\n";
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
                    std::string path;
                    if (!readLineOrEscape(path, "> ")) break;
                    if (!path.empty()) {
                        std::vector<CommandEntry> cmds;
                        const auto outcome = CommandConfig::loadCommands(path, cmds);
                        if (outcome == CommandConfig::LoadOutcome::OK && !cmds.empty()) {
                            if (CommandConfig::saveCommands(CommandConfig::getUserListPath(), cmds)) {
                                Terminal::clearScreen();
                                std::cout << "\x1b[32mImported " << cmds.size() << " commands into the User List.\x1b[0m\r\n";
                            } else {
                                Terminal::clearScreen();
                                std::cout << "\x1b[31mFailed to save custom list.\x1b[0m\r\n";
                            }
                        } else {
                            Terminal::clearScreen();
                            std::cout << "\x1b[31mImport requires a readable file containing only valid entries; User List was not changed.\x1b[0m\r\n";
                        }
                    }
                    std::cout << "Press any key to continue...\r\n";
                    Terminal::readKey();
                    break;
                } else if (selected == 3) { // Edit Current List
                    std::vector<CommandEntry> cmds;
                    if (!loadEditableCommands(cmds)) break;

                    bool changed = false;
                    bool canceled = false;
                    while (true) {
                        Terminal::clearScreen();
                        std::cout << "--- EDIT CURRENT COMMAND LIST ---\r\n";
                        for (std::size_t i = 0; i < cmds.size(); ++i) {
                            std::cout << (i + 1) << ") " << CommandConfig::escapeForDisplay(cmds[i].name) << " | "
                                      << CommandConfig::escapeForDisplay(cmds[i].executeCmd) << "\r\n";
                        }
                        std::cout << "\r\n";
                        std::string choice;
                        if (!readLineOrEscape(choice, "Enter a command number to edit, or 0 to finish: ")) {
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

                        std::string name;
                        if (!readLineOrEscape(name, "New name: ")) {
                            canceled = true;
                            break;
                        }
                        std::string command;
                        if (!readLineOrEscape(command, "New command: ")) {
                            canceled = true;
                            break;
                        }
                        if (name.empty() || name.front() == '#' || name.find('|') != std::string::npos || command.empty()) {
                            std::cout << "Use a nonempty name without # at the start or |, and a nonempty command. Press any key...\r\n";
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
    if (!loadEditableCommands(currentCmds)) return;
    
    Terminal::clearScreen();
    std::cout << "\x1b[33m=== ADD MORE COMMANDS ===\x1b[0m\r\n";
    std::cout << "\x1b[33mNew commands will be available when you start a game.\x1b[0m\r\n";
    std::cout << "Editable user list: " << clickableUserListPath() << "\r\n";
    
    std::string answer;
    if (!readLineOrEscape(answer, "Continue? (y/N): ")) return;
    char confirm = answer.empty() ? 'n' : answer.front();
    
    if (confirm != 'y' && confirm != 'Y') {
        return;
    }
    
    bool changed = false;
    while (true) {
        if (Terminal::inputClosed()) return;
        Terminal::clearScreen();
        std::cout << "\x1b[36m--- ADD COMMAND ---\x1b[0m\r\n";
        std::cout << "Current commands: " << currentCmds.size() << "\r\n";
        std::cout << "Format: name|command (e.g., mycmd|echo hello)\r\n";
        std::cout << "Enter empty name to finish.\r\n\r\n";
        
        std::string name;
        if (!readLineOrEscape(name, "Command name: ")) return;
        
        if (name.empty()) {
            break;
        }
        if (name.front() == '#' || name.find('|') != std::string::npos) {
            std::cout << "Name cannot start with # or contain |. Press any key...\r\n";
            Terminal::readKey();
            continue;
        }
        
        std::string cmd;
        if (!readLineOrEscape(cmd, "Command to execute: ")) return;
        
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
        changed = true;
        std::cout << "\x1b[32mAdded: " << CommandConfig::escapeForDisplay(name) << " | "
                  << CommandConfig::escapeForDisplay(cmd) << "\x1b[0m\r\n";
        std::cout << "Press any key to continue...\r\n";
        Terminal::readKey();
    }
    
    if (!changed) return;

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
