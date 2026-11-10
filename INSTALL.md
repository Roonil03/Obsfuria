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

`make install` also installs the Safe and Crazy command lists under
`/usr/local/share/obsfuria/assets`. The executable locates these relative to
its own installation path, so both lists remain available when launched from
another working directory. Set `PREFIX` to choose an alternate install
prefix; package staging can use `DESTDIR`.

## Command Arsenal Configuration
The game's enemy command list can use:
- **Safe List**: `assets/default_commands.txt` - informational command entries
- **User List**: your editable `name|command` entries, with support for adding commands or importing a file from the in-game menu

Starting a game captures each selected command's output on the local machine, up to 25 lines per command. Encounters display that output; they do not run a command during combat. Commands in a user-provided list can perform any action allowed to your user account.

In-game: `Main Menu → Reload Enemy Commands` lets you choose the Safe List or User List. The User List can be edited, extended, or imported from that menu, and the main-menu **Add More Commands** option also edits it. Selecting a list persists across launches and reloads the enemy pool for the next game. Choosing a list does not run its commands.

The editable user list is `$XDG_CONFIG_HOME/obsfuria/commands.conf` when `XDG_CONFIG_HOME` is set, otherwise `~/.config/obsfuria/commands.conf`. The menu displays its current path as a clickable terminal link. To generate from a specific file, pass it explicitly: `bash ./scripts/add_commands.sh path/to/commands.conf`. An explicit path takes priority; without one, the generator uses the user list if present, then `assets/default_commands.txt`.

## Save Files
Game state saves to XDG path: `~/.local/share/obsfuria/save.bin`
Legacy `./save.bin` auto-migrates on first run.

## Troubleshooting
- `make` fails with `arsenal.hpp not found`: Run `bash ./scripts/add_commands.sh` first
- `nasm: command not found`: Install nasm package (`apt install nasm` / `brew install nasm`)
- Game won't start: Requires interactive TTY (not suitable for CI/headless)
- Save not found after install: Run from same user, saves are per-user in XDG dirs
