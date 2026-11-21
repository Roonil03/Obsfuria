#include "game_loop.hpp"
#include "terminal.hpp"
#include "spatial_manager.hpp"
#include "battle_loop.hpp"
#include "err_simulate.hpp"
#include "save_engine.hpp"
#include "menu_pause.hpp"
#include "stat_calc.hpp"
#include "arsenal.hpp"
#include "command_config.hpp"
#include "reward_system.hpp"
#include <iostream>
#include <string>
#include <cstddef>
#include <map>
#include <random>
#include <utility>
#include <vector>

namespace {
using CommandMap = std::map<std::string, std::string>;

CommandMap loadSessionCommands(CommandConfig::List list) {
    if (list == CommandConfig::List::USER) {
        auto entries = CommandConfig::loadCommands(CommandConfig::getUserListPath());
        if (entries.empty()) return {};
        return CommandConfig::buildEncounterArsenalMap(std::move(entries));
    }
    CommandMap commands;
    for (auto& entry : CommandConfig::getDefaultCommands()) {
        commands.emplace(std::move(entry.name), "Obsfuria node active\n");
    }
    return commands;
}

bool saveOrQuit(const GameState& state) {
    while (true) {
        if (SaveEngine::saveState(state)) return true;
        Terminal::clearScreen();
        if (SaveEngine::lastSaveOutcome() == SaveEngine::SaveOutcome::INSTALLED_DURABILITY_UNCERTAIN) {
            std::cout << "Save installed, but storage durability is uncertain. Press any key to continue.\r\n";
            Terminal::readKey();
            return true;
        }
        std::cout << "Save failed. Press R to retry or Q to quit without saving.\r\n";
        const int key = Terminal::readKey();
        if (key == 'r' || key == 'R') continue;
        if (key == 'q' || key == 'Q' || key < 0) return false;
    }
}
}

void GameLoop::start(MenuMain::Difficulty diff) {
    GameState state;
    
    if (diff == MenuMain::Difficulty::CONTINUE) {
        if (!SaveEngine::loadState(state)) {
            Terminal::clearScreen();
            const auto reason = SaveEngine::lastLoadOutcome();
            std::cout << "Unable to load the saved game ("
                      << (reason == SaveEngine::LoadOutcome::INVALID ? "invalid" :
                          reason == SaveEngine::LoadOutcome::INACCESSIBLE ? "inaccessible" :
                          reason == SaveEngine::LoadOutcome::MIGRATION_FAILED ? "migration failed" : "missing")
                      << "). Press any key to return.\r\n";
            Terminal::readKey();
            return;
        }
        if (SaveEngine::lastLoadOutcome() == SaveEngine::LoadOutcome::LOADED_DURABILITY_UNCERTAIN) {
            Terminal::clearScreen();
            std::cout << "Save migrated, but storage durability is uncertain. Press any key to continue.\r\n";
            Terminal::readKey();
        }
    }
    
    if (diff == MenuMain::Difficulty::EASY) {
        state = {1, 1, 20, 20, 10, 5, 75, Difficulty::EASY};
        if (!saveOrQuit(state)) return;
    } else if (diff == MenuMain::Difficulty::HARD) {
        state = {1, 1, 10, 10, 5, 2, 25, Difficulty::HARD};
        if (!saveOrQuit(state)) return;
    } else if (diff == MenuMain::Difficulty::CHANGE_COMMANDS) {
        MenuMain::displayChangeCommands();
        return;
    } else if (diff == MenuMain::Difficulty::ADD_COMMANDS) {
        MenuMain::displayAddCommands();
        return;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    const auto list = CommandConfig::getActiveList();
    const auto sessionCommands = loadSessionCommands(list);
    const auto& commands = list == CommandConfig::List::USER && sessionCommands.empty()
        ? Arsenal::getMockCommands() : sessionCommands;
    // Both maps outlive the index. Sampling only needs a pointer per command.
    std::vector<const CommandMap::value_type*> enemyPool;
    enemyPool.reserve(commands.size());
    for (const auto& entry : commands) {
        if (entry.first != "obsfuria_version") enemyPool.push_back(&entry);
    }

    // Pacing is local to this exploration session; saves keep their existing format.
    bool hunting = false;
    int clearMoves = 0;
    
    while (true) {
        Terminal::clearScreen();
        std::cout << "--- MAZE EXPLORATION ---\r\n";
        std::cout << "WASD to move, P/ESC to pause, Q to save and quit.\r\n";
        std::cout << "C to change pace: "
                  << (hunting ? "HUNT (25% per move; more stat-steal chances and HP risk)"
                               : "NORMAL (10% per move)") << "\r\n";
        std::cout << "After each fight: 3 clear moves. Protection left: " << clearMoves
                  << " move(s). Pace resets to Normal when loading.\r\n";
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
        if (key < 0) { saveOrQuit(state); return; }
        int nx = state.x, ny = state.y;
        
        if (key == 'w' || key == 'W') ny--;
        else if (key == 's' || key == 'S') ny++;
        else if (key == 'a' || key == 'A') nx--;
        else if (key == 'd' || key == 'D') nx++;
        else if (key == 'c' || key == 'C') {
            hunting = !hunting;
            continue;
        }
        else if (key == 'p' || key == 'P' || key == Terminal::ESC) {
            bool resume = MenuPause::displayPause();
            if (!resume) {
                saveOrQuit(state);
                break;
            }
        } else if (key == 'q' || key == 'Q') {
            saveOrQuit(state);
            break;
        }
        
        if (!SpatialManager::isWall(nx, ny)) {
            bool moved = (state.x != nx || state.y != ny);
            
            state.x = nx;
            state.y = ny;

            if (!moved) continue;
            if (clearMoves > 0) {
                --clearMoves;
                continue;
            }
            
            std::uniform_int_distribution<> encounterDist(1, 100);
            if (encounterDist(gen) <= (hunting ? 25 : 10)) {
                int distance = std::abs(nx) + std::abs(ny);
                double diffMult = (state.difficulty == Difficulty::HARD) ? 1.5 : 1.0;
                
                int enemyHp = StatCalc::generateStat(static_cast<int>((5 + (distance / 5)) * diffMult), 1, 999);
                int enemyAtk = StatCalc::generateStat(static_cast<int>((3 + (distance / 10)) * diffMult), 1, 999);
                int enemyDef = StatCalc::generateStat(static_cast<int>((1 + (distance / 15)) * diffMult), 0, 999);
                int enemyRun = StatCalc::generateStat(static_cast<int>((20 + (distance / 5)) * diffMult), 1, 99);
                
                Combatant player = {"Player", state.hp, state.maxHp, state.atk, state.def, state.chanceToRun, {}};
                Combatant enemy = {"Unknown Command", enemyHp, enemyHp, enemyAtk, enemyDef, enemyRun, {}};
                if (std::uniform_int_distribution<int>(1, 10)(gen) == 1) {
                    enemy.name = "Segmentation Fault";
                    enemy.output = "Segmentation fault (core dumped)";
                } else if (!enemyPool.empty()) {
                    std::uniform_int_distribution<std::size_t> pick(0, enemyPool.size() - 1);
                    const auto& entry = *enemyPool[pick(gen)];
                    enemy.name = entry.first;
                    enemy.output = entry.second;
                }

                bool survived = BattleLoop::startBattle(player, enemy);
                
                if (survived) {
                    clearMoves = 3;
                    if (enemy.hp <= 0) {
                        RewardSystem::stealStat(player, enemy, state.difficulty);
                    } else if (!Terminal::inputClosed()) {
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
                    
                    if (!saveOrQuit(state)) return;
                } else {
                    ErrSimulate::simulateCrash(enemy.name);
                }
            }
        }
    }
}
