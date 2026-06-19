#include "reward_system.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include <iostream>

int RewardSystem::calculateExp(int enemyLevel) {
    return enemyLevel * 15;
}

void RewardSystem::stealStat(Combatant& player, const Combatant& enemy) {
    int stealHp = enemy.maxHp;
    int stealAtk = enemy.atk;
    int stealDef = enemy.def;
    int stealRun = enemy.chanceToRun;
    
    std::vector<std::string> opts = {
        "Replace Max HP (" + std::to_string(player.maxHp) + " -> " + std::to_string(enemy.maxHp) + ")",
        "Replace ATK (" + std::to_string(player.atk) + " -> " + std::to_string(enemy.atk) + ")",
        "Replace DEF (" + std::to_string(player.def) + " -> " + std::to_string(enemy.def) + ")",
        "Replace ChanceToRun (" + std::to_string(player.chanceToRun) + "% -> " + std::to_string(enemy.chanceToRun) + "%)"
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
                player.maxHp = enemy.maxHp;
                player.hp = enemy.maxHp;
            } else if (selected == 1) {
                player.atk = enemy.atk;
            } else if (selected == 2) {
                player.def = enemy.def;
            } else if (selected == 3) {
                player.chanceToRun = enemy.chanceToRun;
            }
            break;
        }
    }
}
