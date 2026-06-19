#ifndef REWARD_SYSTEM_HPP
#define REWARD_SYSTEM_HPP

#include "battle_loop.hpp" // For Combatant struct

class RewardSystem {
public:
    static int calculateExp(int enemyLevel);
    /*
     * Interactive stat stealing menu post-combat.
     */
    static void stealStat(Combatant& player, const Combatant& enemy);
};

#endif // REWARD_SYSTEM_HPP
