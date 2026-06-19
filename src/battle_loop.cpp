#include "battle_loop.hpp"
#include "terminal.hpp"
#include "ui_renderer.hpp"
#include "sprite_gen.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <random>

bool BattleLoop::startBattle(Combatant& player, Combatant& enemy) {
    std::string sprite = SpriteGen::generate();
    std::vector<std::string> opts = {"Attack", "Run"};
    int selected = 0;
    
    while (player.hp > 0 && enemy.hp > 0) {
        Terminal::clearScreen();
        UIRenderer::renderEnemy(enemy.name, sprite);
        std::cout << "\nPlayer HP: " << player.hp << "/" << player.maxHp << "\n";
        std::cout << "Enemy HP: " << enemy.hp << "/" << enemy.maxHp << "\n\n";
        UIRenderer::renderMenu("COMBAT", opts, selected);
        
        int key = Terminal::readKey();
        if (key == Terminal::ARROW_UP) {
            selected = (selected - 1 + opts.size()) % opts.size();
        } else if (key == Terminal::ARROW_DOWN) {
            selected = (selected + 1) % opts.size();
        } else if (key == Terminal::ENTER) {
            if (selected == 0) { // Attack
                int pDamage = std::max(1, player.atk - enemy.def);
                enemy.hp -= pDamage;
                if (enemy.hp <= 0) break;
                
                // Enemy turn
                int eDamage = std::max(1, enemy.atk - player.def);
                player.hp -= eDamage;
            } else if (selected == 1) { // Run
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dist(1, 100);
                
                if (dist(gen) <= player.chanceToRun) {
                    return true; // Successfully escaped
                } else {
                    std::cout << "\x1b[31mFailed to run away!\x1b[0m\r\n";
                    int eDamage = std::max(1, enemy.atk - player.def);
                    player.hp -= eDamage;
                    
                    // Pause briefly to show the message
                    usleep(1000000); 
                }
            }
        }
    }
    
    return player.hp > 0;
}
