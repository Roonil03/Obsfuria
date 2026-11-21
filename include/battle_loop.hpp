#ifndef BATTLE_LOOP_HPP
#define BATTLE_LOOP_HPP

#include <string>

struct Combatant {
    std::string name;
    int hp;
    int maxHp;
    int atk;
    int def;
    int chanceToRun;
    std::string output;
};

class BattleLoop {
public:
    /*
     * Engages the interactive battle loop with the terminal.
     * Returns false if the player died; true for victory, escape or closed input.
     * On closed input, callers can check Terminal::inputClosed() before rewards.
     */
    static bool startBattle(Combatant& player, Combatant& enemy);
};

#endif // BATTLE_LOOP_HPP
