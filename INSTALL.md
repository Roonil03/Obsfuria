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
make                                    # Generates arsenal.hpp and builds bin/Obsfuria
./bin/Obsfuria                          # Run game (requires TTY)
```

## System Install
```bash
make                                    # Build as your regular user
sudo make install                       # Install the already-built binary to /usr/local
obsfuria                                # Run from anywhere
```

For a user-local install, no `sudo` is needed:

```bash
make
make install PREFIX="$HOME/.local"
```

`make install` requires a current `bin/Obsfuria` build and only copies the
binary and command data into `$(DESTDIR)$(PREFIX)`. It does not build the game
or remove files from your home directory. Run `make` as your regular user
first; if sources have changed since the build, rebuild before installing.
Set `PREFIX` to choose an install prefix. Package staging can use `DESTDIR`,
for example `make install DESTDIR="$PWD/stage" PREFIX=/usr`.
`make uninstall` uses the same `PREFIX` and `DESTDIR` values and removes only
the files installed by this project.

## Command Arsenal Configuration
The game provides a curated **Safe List** with fixed mock output and an editable **User List**. The
User List contains `name|command` entries and supports adding commands or
importing a file from the in-game menu.

Commands in a user-provided list can perform actions allowed to your user
account when the game captures their output. Only add commands you trust.
Capture uses `/bin/sh -c`, with a one-second limit per command, a 40-second
session budget, 16 KiB of output and 25 lines per command. These limits and
I/O redirection are not a sandbox for hostile shell commands.
Encounters display captured output and do not run commands during combat.

In-game: `Main Menu → Reload Enemy Commands` lets you choose the Safe List or User List. The User List can be edited, extended, or imported from that menu, and the main-menu **Add More Commands** option also edits it. Selecting a list persists across launches and reloads the enemy pool for the next game. Choosing a list does not run its commands.

The editable user list is `$XDG_CONFIG_HOME/obsfuria/commands.conf` when
`XDG_CONFIG_HOME` is set, otherwise `~/.config/obsfuria/commands.conf`. The
menu displays its current path as a clickable terminal link. Build generation
uses the bundled default command data, not a user-provided list.

## Save Files
Game state saves to the XDG path: `~/.local/share/obsfuria/save.bin`
Legacy `./save.bin` auto-migrates on first run.
Saves use fixed, versioned fields. CRC detects accidental changes; XOR only
obfuscates bytes. Neither encrypts or authenticates save data.
Existing version-1 saves migrate when loaded. Concurrent writers publish
complete files atomically; the last successful rename wins. If a directory
sync fails after rename, the save is installed but its durability is uncertain.

## Troubleshooting
- `make` fails with `arsenal.hpp not found`: Run `bash ./scripts/add_commands.sh` first
- `nasm: command not found`: Install nasm package (`apt install nasm` / `brew install nasm`)
- Game won't start: Requires interactive TTY (not suitable for CI/headless)
- Save not found after install: Run from same user, saves are per-user in XDG dirs
