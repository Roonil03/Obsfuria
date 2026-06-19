#include "exec_mock.hpp"
#include "arsenal.hpp"
#include "terminal.hpp"
#include <iostream>

void ExecMock::simulateCommand(const std::string& commandName) {
    Terminal::clearScreen();
    auto commands = Arsenal::getMockCommands();
    auto it = commands.find(commandName);
    
    std::cout << "[MOCK EXEC] obsfuria_user@local:~$ " << commandName << "\r\n";
    if (it != commands.end()) {
        std::cout << it->second << "\r\n";
    } else {
        std::cout << "bash: " << commandName << ": command not found\r\n";
    }
}
