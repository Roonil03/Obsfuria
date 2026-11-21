#include "battle_loop.hpp"
#include "terminal.hpp"
#include "terminal_text.hpp"
#include "ui_renderer.hpp"
#include "sprite_gen.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

namespace {
enum class Pattern { BURST, BULWARK, SWARM, SIPHON };

Pattern patternFor(const std::string& name) {
    // A stable byte hash also gives imported names a pattern without executing them.
    std::uint32_t hash = 2166136261u;
    for (unsigned char ch : name) hash = (hash ^ ch) * 16777619u;
    return static_cast<Pattern>(hash % 4);
}

const char* patternName(Pattern pattern) {
    switch (pattern) {
        case Pattern::BURST: return "Burst";
        case Pattern::BULWARK: return "Bulwark";
        case Pattern::SWARM: return "Swarm";
        case Pattern::SIPHON: return "Siphon";
    }
    return "Burst";
}

struct Intent {
    const char* name;
    const char* counter;
    int damage = 0;
    int hits = 1;
    bool charge = false;
    bool armored = false;
    bool drain = false;
    bool recovering = false;
};

Intent nextIntent(Pattern pattern, int phase, int damage, bool chargeBroken) {
    switch (pattern) {
        case Pattern::BURST:
            if (phase == 0) return {"CHARGE", "Break now: a weak counter replaces its next heavy strike.", 0, 1, true};
            if (phase == 1 && chargeBroken) return {"COUNTER", "Charge broken. Guard reduces this weak counter to one third.", std::max(1, damage / 2)};
            if (phase == 1) return {"RUPTURE", "Guard reduces the heavy strike to one third.", damage * 2};
            break;
        case Pattern::BULWARK:
            if (phase == 0) return {"BRACE", "Attack is halved. Break bypasses armor; shield bash still follows.", 0, 1, false, true};
            if (phase == 1) return {"SHIELD BASH", "Guard reduces the bash to one third.", damage + (damage + 1) / 2};
            break;
        case Pattern::SWARM:
            if (phase == 0) return {"PROBE", "A single hit; attack now or prepare to guard its volley.", damage};
            if (phase == 1) return {"VOLLEY", "Three hits. Guard reduces each hit to one third.", std::max(1, (damage + 1) / 2), 3};
            break;
        case Pattern::SIPHON:
            if (phase == 0) return {"DRAIN", "Guard reduces damage and prevents all enemy healing.", damage, 1, false, false, true};
            if (phase == 1) return {"ECHO DRAIN", "Guard prevents healing; its recovery is next.", std::max(1, damage / 2), 1, false, false, true};
            break;
    }
    return {"RECOVER", "Attack deals double damage. It cannot retaliate this turn.", 0, 1, false, false, false, true};
}

int guardedDamage(int damage) {
    return (damage + 2) / 3;
}

void renderEnemyOutput(const std::string& output) {
    constexpr std::size_t maxLines = 6;
    constexpr std::size_t maxLineLength = 100;
    std::cout << "Captured output (O: sprite):\r\n";
    if (output.empty()) {
        std::cout << "  (no captured output)\r\n";
        return;
    }
    std::size_t offset = 0;
    for (std::size_t line = 0; line < maxLines && offset < output.size(); ++line) {
        std::string preview;
        while (offset < output.size() && output[offset] != '\n') {
            unsigned int codepoint = 0;
            const auto length = TerminalText::characterLength(output, offset, codepoint);
            const auto bytes = length == 0 ? 1 : length;
            const auto escaped = TerminalText::escape(output.substr(offset, bytes));
            if (preview.size() + escaped.size() > maxLineLength) {
                std::cout << "  " << preview << "... [output truncated]\r\n";
                return;
            }
            preview += escaped;
            offset += bytes;
        }
        std::cout << "  " << preview << "\r\n";
        if (offset < output.size()) ++offset;
    }
    if (offset < output.size()) std::cout << "  ... [output truncated]\r\n";
}
}

bool BattleLoop::startBattle(Combatant& player, Combatant& enemy) {
    const std::string sprite = SpriteGen::generate();
    const Pattern pattern = patternFor(enemy.name);
    const int runChance = std::clamp(player.chanceToRun, 0, 100);
    const std::vector<std::string> options = {
        "Attack [A]", "Guard [G]", "Break [B]", "Run [R] (" + std::to_string(runChance) + "%)"
    };
    std::mt19937 generator(std::random_device{}());
    int selected = 0;
    int phase = 0;
    bool chargeBroken = false;
    bool showOutput = true;
    std::string playerLog = "Choose an action. Navigation and O do not spend a turn.";
    std::string enemyLog = "Patterns repeat every three turns; recovery rewards an attack.";

    while (player.hp > 0 && enemy.hp > 0) {
        const int normalDamage = std::max(1, player.atk - enemy.def);
        const int baseEnemyDamage = std::max(1, enemy.atk - player.def);
        const Intent intent = nextIntent(pattern, phase, baseEnemyDamage, chargeBroken);
        const int attackDamage = intent.recovering ? normalDamage * 2
            : intent.armored ? std::max(1, normalDamage / 2) : normalDamage;
        const int breakDamage = intent.armored ? normalDamage : std::max(1, normalDamage / 2);
        Terminal::clearScreen();
        std::cout << "ENEMY: " << TerminalText::escape(enemy.name) << " | " << patternName(pattern) << " pattern\r\n";
        if (showOutput) renderEnemyOutput(enemy.output);
        else {
            std::cout << "Command sprite (O: captured output):\r\n";
            for (char ch : sprite) {
                if (ch == '\n') std::cout << "\r\n";
                else std::cout << ch;
            }
        }
        std::cout << "HP " << player.hp << "/" << player.maxHp
                  << " | Enemy " << enemy.hp << "/" << enemy.maxHp << "\r\n";
        std::cout << "NEXT: " << intent.name << " | " << intent.hits << " x " << intent.damage
                  << " damage (Guard: " << intent.hits << " x " << guardedDamage(intent.damage) << ")\r\n";
        std::cout << intent.counter << "\r\n";
        std::cout << "Attack: " << attackDamage << " damage | Break: " << breakDamage << " damage\r\n";
        std::cout << playerLog << "\r\n" << enemyLog << "\r\n";
        UIRenderer::renderMenu("COMBAT", options, selected);
        std::cout << "Arrows/W/S + Enter | Esc: Run | O: output/sprite\r\n";

        const int key = Terminal::readKey();
        if (key < 0) return true;
        if (key == Terminal::ARROW_UP || key == 'w' || key == 'W') {
            selected = (selected + static_cast<int>(options.size()) - 1) % options.size();
            continue;
        }
        if (key == Terminal::ARROW_DOWN || key == 's' || key == 'S') {
            selected = (selected + 1) % options.size();
            continue;
        }
        if (key == 'o' || key == 'O') {
            showOutput = !showOutput;
            continue;
        }
        int action = -1;
        if (key == Terminal::ENTER) action = selected;
        else if (key == 'a' || key == 'A') action = 0;
        else if (key == 'g' || key == 'G') action = 1;
        else if (key == 'b' || key == 'B') action = 2;
        else if (key == 'r' || key == 'R' || key == Terminal::ESC) action = 3;
        if (action < 0) continue;
        selected = action;

        bool breakCharge = false;
        const bool guarding = action == 1;
        if (action == 0 || action == 2) {
            const int damage = std::min(enemy.hp, action == 0 ? attackDamage : breakDamage);
            enemy.hp -= damage;
            playerLog = std::string(action == 0 ? "You attack for " : "You break for ") + std::to_string(damage) + " damage.";
            if (enemy.hp <= 0) break;
            if (action == 2 && intent.charge) {
                breakCharge = true;
                playerLog += " Charge broken: weak counter next.";
            } else if (action == 2 && intent.armored) {
                playerLog += " Armor breached; shield bash still follows.";
            }
        } else if (guarding) {
            playerLog = "You guard: damage reduced; siphon healing blocked.";
        } else {
            if (std::uniform_int_distribution<int>(1, 100)(generator) <= runChance) return true;
            playerLog = "Escape failed! The enemy carries out its announced move.";
        }

        int damageTaken = 0;
        for (int hit = 0; hit < intent.hits && player.hp > 0; ++hit) {
            const int damage = std::min(player.hp, guarding ? guardedDamage(intent.damage) : intent.damage);
            player.hp -= damage;
            damageTaken += damage;
        }
        int healed = 0;
        if (intent.drain && !guarding) {
            healed = std::min(damageTaken, std::max(0, enemy.maxHp - enemy.hp));
            enemy.hp += healed;
        }
        enemyLog = std::string(intent.name) + ": " + std::to_string(damageTaken) + " damage taken.";
        if (intent.drain) enemyLog += " Enemy healed " + std::to_string(healed) + " HP.";
        chargeBroken = breakCharge;
        phase = (phase + 1) % 3;
    }
    return player.hp > 0;
}
