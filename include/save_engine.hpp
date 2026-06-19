#ifndef SAVE_ENGINE_HPP
#define SAVE_ENGINE_HPP

#include <string>

struct GameState {
    int x;
    int y;
    int hp;
    int maxHp;
    int atk;
    int def;
    int chanceToRun;
};

class SaveEngine {
public:
    static bool saveState(const std::string& filename, const GameState& state);
    static bool loadState(const std::string& filename, GameState& state);
    static bool hasValidSave(const std::string& filename);
    static void deleteSave(const std::string& filename);
};

#endif // SAVE_ENGINE_HPP
