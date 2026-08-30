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
    }
    SaveEngine::deleteSave();
    Terminal::disableRawMode();
    std::exit(1);
}
