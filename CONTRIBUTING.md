# Contributing to Obsfuria

Thank you for your interest in contributing! This document outlines the process and guidelines.

## Getting Started

1. **Fork** the repository
2. **Clone** your fork: `git clone https://github.com/your-username/Obsfuria.git`
3. **Create a feature branch**: `git checkout -b feature/YourFeatureName`
4. **Build and verify**: Follow the verification steps below.
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

1. **Option A (Bundled data)**: Add entries to the project's command data using `name|command` format. Safe List names and mock output are curated in `src/command_config.cpp`.
2. **Option B (User config)**: Users can import custom lists via `Change Command List → Import Custom File`.
3. **Execution behavior**:
   - Build generation is deterministic and does not run commands from the input data.
   - User-provided commands may be executed when the game captures their output. Treat imported command lists as executable input and only use trusted lists.
   - During encounters, enemies show captured output; encounters do not execute commands.
4. **Formatting**:
   - Follow the game's enemy naming convention: non-alphanumeric names or C-error style names (for example, "Segmentation Fault" or "Stack Overflow").
   - The generator chooses safe C++ string delimiters automatically; do not edit generated `include/arsenal.hpp` by hand.

## Verifying a change

Use an external scratch directory to build the current tracked files, including
local edits. Stage any new source files first so Git includes them in the copy. The phase
test sources are maintained locally. If you have them, copy the local
`tests/` directory beside the scratch Makefile before running `make test`.

```bash
scratch=$(mktemp -d)
git ls-files --cached --deduplicate -z | tar --null -T - -cf - | tar -xf - -C "$scratch"
if [ -d tests ]; then cp -R tests "$scratch/"; fi
mkdir "$scratch/home"
(cd "$scratch" && bash scripts/add_commands.sh && make -j4)
(cd "$scratch" && HOME="$scratch/home" XDG_CONFIG_HOME="$scratch/home/config" XDG_DATA_HOME="$scratch/home/data" ./bin/Obsfuria)
```

Use a terminal for the game. Start an Easy game, quit to the menu with Q and
select Continue. With the local suite available, also run
`(cd "$scratch" && HOME="$scratch/home" XDG_CONFIG_HOME="$scratch/home/config" XDG_DATA_HOME="$scratch/home/data" make test)`.

## Architecture Notes

- `src/main.cpp` → `MenuMain` → `GameLoop` → `BattleLoop`
- `SpatialManager` wraps NASM (`nasm_hash.asm`, `nasm_crypto.asm`)
- `SaveEngine` writes versioned little-endian fields with a CRC32 error check and XOR obfuscation to XDG `~/.local/share/obsfuria/`; neither mechanism provides encryption or authentication
- `Arsenal` loads bundled display text from generated `include/arsenal.hpp`; the editable user command list is stored in the user's config directory
- `StatCalc` generates distance-scaled stats; `RewardSystem` caps by difficulty

## Reporting Issues

- Use GitHub Issues with: steps to reproduce, expected vs actual, OS/terminal info
- Include save file (`~/.local/share/obsfuria/save.bin`) if relevant

## License

By contributing, you agree your contributions will be licensed under MPL-2.0 (same as project).
