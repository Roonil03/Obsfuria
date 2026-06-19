#include "game_loop.hpp"
#include "terminal.hpp"
#include "spatial_manager.hpp"
#include "battle_loop.hpp"
#include "err_simulate.hpp"
#include "save_engine.hpp"
#include "menu_pause.hpp"
#include "stat_calc.hpp"
#include "arsenal.hpp"
#include "reward_system.hpp"
#include <iostream>
#include <string>
#include <random>

void GameLoop::start(MenuMain::Difficulty diff) {
    GameState state;
    
    if (diff == MenuMain::Difficulty::CONTINUE) {
        if (!SaveEngine::loadState("save.bin", state)) {
            diff = MenuMain::Difficulty::EASY; // Fallback if file vanishes
        }
    }
    
    if (diff == MenuMain::Difficulty::EASY) {
        state = {1, 1, 20, 20, 10, 5, 75}; // Start: Easy mode stats
        SaveEngine::saveState("save.bin", state);
    } else if (diff == MenuMain::Difficulty::HARD) {
        state = {1, 1, 10, 10, 5, 2, 25}; // Start: Hard mode stats
        SaveEngine::saveState("save.bin", state);
    }
    
    // Setup random generator for enemy selection
    std::random_device rd;
    std::mt19937 gen(rd());
    auto commands = Arsenal::getMockCommands();
    
    while (true) {
        Terminal::clearScreen();
        std::cout << "--- MAZE EXPLORATION ---\r\n";
        std::cout << "WASD to move, P or Q to pause/save.\r\n";
        std::cout << "Player Stats -> HP: " << state.hp << "/" << state.maxHp  
                  << " | ATK: " << state.atk << " | DEF: " << state.def << "\r\n";
        
        // Render local 5x5 view
        for (int y = state.y - 2; y <= state.y + 2; ++y) {
            for (int x = state.x - 2; x <= state.x + 2; ++x) {
                if (x == state.x && y == state.y) {
                    std::cout << "\x1b[32m@\x1b[0m"; // Player
                } else if (SpatialManager::isWall(x, y)) {
                    std::cout << "#"; // Wall
                } else {
                    std::cout << "."; // Floor
                }
            }
            std::cout << "\r\n";
        }
        
        int key = Terminal::readKey();
        int nx = state.x, ny = state.y;
        
        if (key == 'w') ny--;
        else if (key == 's') ny++;
        else if (key == 'a') nx--;
        else if (key == 'd') nx++;
        else if (key == 'p' || key == Terminal::ESC) {
            bool resume = MenuPause::displayPause();
            if (!resume) {
                SaveEngine::saveState("save.bin", state);
                break;
            }
        } else if (key == 'q') {
            SaveEngine::saveState("save.bin", state);
            break;
        }
        
        if (!SpatialManager::isWall(nx, ny)) {
            // Check if we actually moved to a new tile
            bool moved = (state.x != nx || state.y != ny);
            
            state.x = nx;
            state.y = ny;
            
            // Random encounter chance (10% per step) using an actual RNG instead of a fixed coordinate hash
            std::uniform_int_distribution<> encounterDist(1, 100);
            if (moved && encounterDist(gen) <= 10) {
                // Dynamically scale difficulty based on distance from origin
                int distance = std::abs(nx) + std::abs(ny);
                int enemyHp = StatCalc::generateStat(5 + (distance / 5), 1, 999);
                int enemyAtk = StatCalc::generateStat(3 + (distance / 10), 1, 999);
                int enemyDef = StatCalc::generateStat(1 + (distance / 15), 0, 999);
                int enemyRun = StatCalc::generateStat(20 + (distance / 5), 1, 99);
                
                // Pick random enemy name
                std::uniform_int_distribution<> dist(0, commands.size() - 1);
                auto it = commands.begin();
                std::advance(it, dist(gen));
                std::string enemyName = it->first; // E.g., "ls", "whoami"
                
                // 10% chance to be a fatal C-Error instead of a command
                if (dist(gen) % 10 == 0) enemyName = "Segmentation Fault";
                
                Combatant player = {"Player", state.hp, state.maxHp, state.atk, state.def, state.chanceToRun};
                Combatant enemy = {enemyName, enemyHp, enemyHp, enemyAtk, enemyDef, enemyRun};
                
                bool survived = BattleLoop::startBattle(player, enemy);
                
                if (survived) {
                    if (enemy.hp <= 0) {
                        RewardSystem::stealStat(player, enemy);
                    } else {
                        // The player successfully ran away
                        Terminal::clearScreen();
                        std::cout << "\x1b[33mYou successfully escaped!\x1b[0m\r\n";
                        std::cout << "Press any key to continue...\r\n";
                        Terminal::readKey();
                    }
                    
                    // Update state with modified stats
                    state.hp = player.hp;
                    state.maxHp = player.maxHp;
                    state.atk = player.atk;
                    state.def = player.def;
                    state.chanceToRun = player.chanceToRun;
                } else {
                    ErrSimulate::simulateCrash(enemy.name);
                }
            }
        }
    }
}
