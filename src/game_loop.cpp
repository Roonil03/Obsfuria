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
        if (!SaveEngine::loadState(state)) {
            diff = MenuMain::Difficulty::EASY;
        }
    }
    
    if (diff == MenuMain::Difficulty::EASY) {
        state = {1, 1, 20, 20, 10, 5, 75, Difficulty::EASY};
        SaveEngine::saveState(state);
    } else if (diff == MenuMain::Difficulty::HARD) {
        state = {1, 1, 10, 10, 5, 2, 25, Difficulty::HARD};
        SaveEngine::saveState(state);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    auto commands = Arsenal::getMockCommands();
    
    while (true) {
        Terminal::clearScreen();
        std::cout << "--- MAZE EXPLORATION ---\r\n";
        std::cout << "WASD to move, P or Q to pause/save.\r\n";
        std::cout << "Player Stats -> HP: " << state.hp << "/" << state.maxHp  
                  << " | ATK: " << state.atk << " | DEF: " << state.def << "\r\n";
        
        for (int y = state.y - 2; y <= state.y + 2; ++y) {
            for (int x = state.x - 2; x <= state.x + 2; ++x) {
                if (x == state.x && y == state.y) {
                    std::cout << "\x1b[32m@\x1b[0m";
                } else if (SpatialManager::isWall(x, y)) {
                    std::cout << "#";
                } else {
                    std::cout << ".";
                }
            }
            std::cout << "\r\n";
        }
        
        int key = Terminal::readKey();
        int nx = state.x, ny = state.y;
        
        if (key == 'w' || key == 'W') ny--;
        else if (key == 's' || key == 'S') ny++;
        else if (key == 'a' || key == 'A') nx--;
        else if (key == 'd' || key == 'D') nx++;
        else if (key == 'p' || key == 'P' || key == Terminal::ESC) {
            bool resume = MenuPause::displayPause();
            if (!resume) {
                SaveEngine::saveState(state);
                break;
            }
        } else if (key == 'q' || key == 'Q') {
            SaveEngine::saveState(state);
            break;
        }
        
        if (!SpatialManager::isWall(nx, ny)) {
            bool moved = (state.x != nx || state.y != ny);
            
            state.x = nx;
            state.y = ny;
            
            std::uniform_int_distribution<> encounterDist(1, 100);
            if (moved && encounterDist(gen) <= 10) {
                int distance = std::abs(nx) + std::abs(ny);
                double diffMult = (state.difficulty == Difficulty::HARD) ? 1.5 : 1.0;
                
                int enemyHp = StatCalc::generateStat(static_cast<int>((5 + (distance / 5)) * diffMult), 1, 999);
                int enemyAtk = StatCalc::generateStat(static_cast<int>((3 + (distance / 10)) * diffMult), 1, 999);
                int enemyDef = StatCalc::generateStat(static_cast<int>((1 + (distance / 15)) * diffMult), 0, 999);
                int enemyRun = StatCalc::generateStat(static_cast<int>((20 + (distance / 5)) * diffMult), 1, 99);
                
                std::uniform_int_distribution<> dist(0, commands.size() - 1);
                auto it = commands.begin();
                std::advance(it, dist(gen));
                std::string enemyName = it->first;
                
                if (dist(gen) % 10 == 0) enemyName = "Segmentation Fault";
                
                Combatant player = {"Player", state.hp, state.maxHp, state.atk, state.def, state.chanceToRun};
                Combatant enemy = {enemyName, enemyHp, enemyHp, enemyAtk, enemyDef, enemyRun};
                
                bool survived = BattleLoop::startBattle(player, enemy);
                
                if (survived) {
                    if (enemy.hp <= 0) {
                        RewardSystem::stealStat(player, enemy);
                    } else {
                        Terminal::clearScreen();
                        std::cout << "\x1b[33mYou successfully escaped!\x1b[0m\r\n";
                        std::cout << "Press any key to continue...\r\n";
                        Terminal::readKey();
                    }
                    
                    state.hp = player.hp;
                    state.maxHp = player.maxHp;
                    state.atk = player.atk;
                    state.def = player.def;
                    state.chanceToRun = player.chanceToRun;
                    
                    SaveEngine::saveState(state);
                } else {
                    ErrSimulate::simulateCrash(enemy.name);
                }
            }
        }
    }
}
