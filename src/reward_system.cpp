#include "reward_system.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include <iostream>
#include <algorithm>

int RewardSystem::calculateExp(int enemyLevel) {
    return enemyLevel * 15;
}

static int capStat(int value, int maxCap) {
    return std::min(value, maxCap);
}

void RewardSystem::stealStat(Combatant& player, const Combatant& enemy, Difficulty diff) {
    int maxHpCap = (diff == Difficulty::HARD) ? 999 : 50;
    int atkCap = (diff == Difficulty::HARD) ? 999 : 30;
    int defCap = (diff == Difficulty::HARD) ? 999 : 20;
    int runCap = (diff == Difficulty::HARD) ? 99 : 80;
    
    int stealHp = capStat(enemy.maxHp, maxHpCap);
    int stealAtk = capStat(enemy.atk, atkCap);
    int stealDef = capStat(enemy.def, defCap);
    int stealRun = capStat(enemy.chanceToRun, runCap);
    
    std::vector<std::string> opts = {
        "Replace Max HP (" + std::to_string(player.maxHp) + " -> " + std::to_string(stealHp) + ")",
        "Replace ATK (" + std::to_string(player.atk) + " -> " + std::to_string(stealAtk) + ")",
        "Replace DEF (" + std::to_string(player.def) + " -> " + std::to_string(stealDef) + ")",
        "Replace ChanceToRun (" + std::to_string(player.chanceToRun) + "% -> " + std::to_string(stealRun) + "%)"
    };
    int selected = 0;
    
    while (true) {
        Terminal::clearScreen();
        std::cout << "\x1b[35mEnemy Defeated! You feel their code flowing into you...\x1b[0m\r\n\n";
        UIRenderer::renderMenu("REWARD: STAT STEAL", opts, selected);
        
        int key = Terminal::readKey();
        if (key == Terminal::ARROW_UP) {
            selected = (selected - 1 + opts.size()) % opts.size();
        } else if (key == Terminal::ARROW_DOWN) {
            selected = (selected + 1) % opts.size();
        } else if (key == Terminal::ENTER) {
            if (selected == 0) {
                player.maxHp = stealHp;
                player.hp = stealHp;
            } else if (selected == 1) {
                player.atk = stealAtk;
            } else if (selected == 2) {
                player.def = stealDef;
            } else if (selected == 3) {
                player.chanceToRun = stealRun;
            }
            break;
        }
    }
}
