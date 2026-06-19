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
};

class BattleLoop {
public:
    /*
     * Engages the interactive battle loop with the terminal.
     * Returns true if player won/escaped, false if player died.
     */
    static bool startBattle(Combatant& player, Combatant& enemy);
};

#endif // BATTLE_LOOP_HPP
