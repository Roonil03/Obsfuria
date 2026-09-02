# Installation Instructions

## Prerequisites
- `g++` (C++17 support, GCC 9+ or Clang 10+)
- `nasm` (Netwide Assembler, 2.14+)
- `make` (GNU Make)
- Optional: `x-terminal-emulator` / `xterm` / `gnome-terminal` (for crash simulation), `tree`, `cowsay`, `fortune` (for full command arsenal)

## Quick Start (Development)
```bash
git clone <repo-url>
cd Obsfuria
bash ./scripts/add_commands.sh          # Generate arsenal.hpp (REQUIRED before make)
make                                    # Builds bin/Obsfuria
./bin/Obsfuria                          # Run game (requires TTY)
```

## System Install
```bash
bash ./scripts/add_commands.sh          # Generate arsenal from default assets
make                                    # Build binary
sudo make install                       # Installs to /usr/local/bin/obsfuria
obsfuria                                # Run from anywhere
```

## Command Arsenal Configuration
The game's enemy command list is configurable:
- **Safe List** (default): `assets/default_commands.txt` - 46 informational commands
- **Crazy List**: `assets/crazy_commands.txt` - 21 "dangerous-looking" mocked commands
- **Custom**: Import your own `name|command` file via in-game menu

In-game: `Main Menu → Change Command List`

Config persists at `~/.config/obsfuria/commands.conf`

## Save Files
Game state saves to XDG path: `~/.local/share/obsfuria/save.bin`
Legacy `./save.bin` auto-migrates on first run.

## Troubleshooting
- `make` fails with `arsenal.hpp not found`: Run `bash ./scripts/add_commands.sh` first
- `nasm: command not found`: Install nasm package (`apt install nasm` / `brew install nasm`)
- Game won't start: Requires interactive TTY (not suitable for CI/headless)
- Save not found after install: Run from same user, saves are per-user in XDG dirs
