#ifndef REWARD_SYSTEM_HPP
#define REWARD_SYSTEM_HPP

#include "battle_loop.hpp" // For Combatant struct
#include "save_engine.hpp" // For Difficulty enum

class RewardSystem {
public:
    static int calculateExp(int enemyLevel);
    /*
     * Interactive stat stealing menu post-combat.
     */
    static void stealStat(Combatant& player, const Combatant& enemy, Difficulty diff);
};

#endif // REWARD_SYSTEM_HPP
