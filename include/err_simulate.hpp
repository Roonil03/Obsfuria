#ifndef ERR_SIMULATE_HPP
#define ERR_SIMULATE_HPP

#include <string>

class ErrSimulate {
public:
    static void simulateCrash(const std::string& errorType);
};

#endif // ERR_SIMULATE_HPP
