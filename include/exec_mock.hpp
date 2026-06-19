#ifndef EXEC_MOCK_HPP
#define EXEC_MOCK_HPP

#include <string>

class ExecMock {
public:
    static void simulateCommand(const std::string& commandName);
};

#endif // EXEC_MOCK_HPP
