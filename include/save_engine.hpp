#ifndef SAVE_ENGINE_HPP
#define SAVE_ENGINE_HPP

#include <string>
#include <cstdint>

enum class Difficulty { EASY = 0, HARD = 1 };

struct GameState {
    int x = 1;
    int y = 1;
    int hp = 20;
    int maxHp = 20;
    int atk = 10;
    int def = 5;
    int chanceToRun = 75;
    Difficulty difficulty = Difficulty::EASY;
};

struct SaveFile {
    static constexpr uint32_t MAGIC = 0x4F425346; // 'OBSF'
    static constexpr uint16_t VERSION = 1;
    
    uint32_t magic;
    uint16_t version;
    GameState state;
    uint32_t checksum;
};

class SaveEngine {
public:
    static bool saveState(const GameState& state);
    static bool loadState(GameState& state);
    static bool hasValidSave();
    static void deleteSave();
    static std::string getSavePath();
    static std::string getLegacySavePath();
    
    static bool validateState(const GameState& state);
    static uint32_t computeChecksum(const GameState& state);
    
private:
    static bool loadLegacy(const std::string& path, GameState& state);
};

#endif // SAVE_ENGINE_HPP
