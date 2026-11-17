#include "exec_mock.hpp"
#include "arsenal.hpp"
#include "terminal.hpp"
#include "terminal_text.hpp"
#include <iostream>

namespace {
void printEscaped(const std::string& text, bool multiline = false) {
    std::cout << TerminalText::escape(text, multiline);
}
}

void ExecMock::simulateCommand(const std::string& commandName) {
    Terminal::clearScreen();
    const auto& commands = Arsenal::getMockCommands();
    auto it = commands.find(commandName);
    
    std::cout << "[MOCK EXEC] obsfuria_user@local:~$ ";
    printEscaped(commandName);
    std::cout << "\r\n";
    if (it != commands.end()) {
        if (it->second.empty()) {
            std::cout << "(no output)\r\n";
        } else {
            printEscaped(it->second, true);
            std::cout << "\r\n";
        }
    } else {
        std::cout << "bash: ";
        printEscaped(commandName);
        std::cout << ": command not found\r\n";
    }
}
