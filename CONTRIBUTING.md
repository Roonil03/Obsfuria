# Contributing to Obsfuria

Thank you for your interest in contributing! This document outlines the process and guidelines.

## Getting Started

1. **Fork** the repository
2. **Clone** your fork: `git clone https://github.com/your-username/Obsfuria.git`
3. **Create a feature branch**: `git checkout -b feature/YourFeatureName`
4. **Build & test**: `bash ./scripts/add_commands.sh && make && make test`
5. **Make your changes**
6. **Commit**: `git commit -m 'Add YourFeatureName'`
7. **Push**: `git push origin feature/YourFeatureName`
8. **Open a Pull Request** against `main`

## Code Style

- C++17 with `-Wall -Wextra`
- Follow existing naming conventions (PascalCase for classes, camelCase for methods)
- No unnecessary comments; code should be self-documenting
- Header guards: `#ifndef FILENAME_HPP` / `#define FILENAME_HPP`
- One class per header/source pair in `include/` and `src/`

## Adding New Enemies

Enemies are command names from the arsenal. To add new enemies:

1. **Option A (Config file)**: Add entries to `assets/default_commands.txt` or `assets/crazy_commands.txt` using `name|command` format
2. **Option B (User config)**: Users can import custom lists via `Change Command List → Import Custom File`
3. **Formatting rules** (strict):
   - Enemy names displayed in-game must be **non-alphanumeric** or **C-error style** (e.g., "Segmentation Fault", "Stack Overflow")
   - Command output captured at build time must not contain the delimiter `)obsfuria"`
   - Use raw string literals `R"obsfuria(...)obsfuria"` in generated `arsenal.hpp`

## Adding New Phases/Tests

1. Create `tests/phaseN_test.cpp` following existing patterns
2. Add build rule to `Makefile` with correct object dependencies
3. Avoid `Terminal::readKey()` in tests (blocks CI)
4. Run `make test` to verify

## Architecture Notes

- `src/main.cpp` → `MenuMain` → `GameLoop` → `BattleLoop`
- `SpatialManager` wraps NASM (`nasm_hash.asm`, `nasm_crypto.asm`)
- `SaveEngine` uses XOR `0xAA` + CRC32, saves to XDG `~/.local/share/obsfuria/`
- `Arsenal` loads from generated `include/arsenal.hpp` (compile-time) or runtime config
- `StatCalc` generates distance-scaled stats; `RewardSystem` caps by difficulty

## Reporting Issues

- Use GitHub Issues with: steps to reproduce, expected vs actual, OS/terminal info
- Include save file (`~/.local/share/obsfuria/save.bin`) if relevant

## License

By contributing, you agree your contributions will be licensed under MPL-2.0 (same as project).