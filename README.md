# Obsfuria

Obsfuria is a terminal-based, turn-based roguelike RPG.
- Explore procedurally generated grids, encounter non-alphanumeric entities, and try to survive the simulated penalties.
- The curated Safe List provides fixed mock output; custom User List commands may run locally when their output is captured. Use only trusted lists.
- Encounters display captured output as a mock and do not run commands.
- Explore through a tiny maze, and steal the stat of the enemy you defeat. **WARNING:** Be careful of the enemies you defeat, since the stats that you steal will affect your player character.

## Playing

Use WASD to move. Press **C** to switch from Normal (a 10% encounter chance per successful move) to Hunt (25%) when you want more chances to steal a stat. After a win or escape, your next three successful moves are clear. Walking into a wall, switching pace, and pausing all preserve those moves. Continuing a save starts you in Normal with zero protected moves.

In battle, each command name has a consistent attack pattern. Read **NEXT** for the incoming move and damage. Press **A** to attack, **G** to guard, **B** to break, or **R** to attempt escape. You can also select an action with the arrows and Enter. Press **O** to switch between the captured output and the sprite without spending a turn.

Watch for a Burst enemy charging up or a Bulwark bracing behind its armor. A Swarm can hit three times in one turn. A Siphon heals from the damage it deals unless you guard. All four have a recovery turn when your attack deals double damage.


## Documentation:
- For [installation, please check here!](./INSTALL.md)
- For helping out with the game, [check out here!](./CONTRIBUTING.md)
