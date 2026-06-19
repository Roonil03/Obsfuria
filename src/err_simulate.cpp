#include "err_simulate.hpp"
#include "terminal.hpp"
#include "save_engine.hpp"
#include <iostream>
#include <cstdlib>

void ErrSimulate::simulateCrash(const std::string& errorType) {
    Terminal::clearScreen();
    if (errorType == "Segmentation Fault") {
        std::cout << "Segmentation fault (core dumped)\r\n";
    } else if (errorType == "Stack Overflow") {
        std::cout << "Fatal error: stack overflow\r\n";
    } else {
        std::cout << "Aborted (core dumped)\r\n";
        
        // Execute the command in a new terminal window
        std::string sysCmd = "x-terminal-emulator -e \"bash -c '" + errorType + "; echo \\\"\\n[OBSFURIA GAME OVER] Command executed. Press any key to close.\\\"; read -n 1'\" &";
        std::system(sysCmd.c_str());
    }
    SaveEngine::deleteSave("save.bin");
    Terminal::disableRawMode();
    std::exit(1);
}
