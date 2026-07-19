#ifndef GUARD_PEEPO_HARDCORE_H
#define GUARD_PEEPO_HARDCORE_H

#include "global.h"

// Hardcore mode = an enforced nuzlocke, toggled at new game alongside the
// randomizer (RANDO_F_HARDCORE). Three rules:
//   * a fainted party Pokémon is permanently released after the battle,
//   * whiting out wipes the save and restarts at the title screen,
//   * you may only catch one Pokémon per route (region map section).

bool8 PeepoHardcore_IsEnabled(void);

// One-catch-per-route: has a Pokémon already been caught on the current map's
// route? Marked on a successful catch.
bool8 PeepoHardcore_RouteAlreadyCaught(void);
void PeepoHardcore_MarkRouteCaught(void);

// After a battle: permanently release any party Pokémon that fainted (HP == 0).
void PeepoHardcore_ReleaseFainted(void);

// Per-frame tick (from the main loop): fires ReleaseFainted on battle exit.
void PeepoHardcore_Tick(void);

// Hardcore game over. CB2_WhiteOut installs the field callback in place of the
// normal warp-exit fade when hardcore is on; it erases the save, shows the
// game-over message, and the script's callnative reboots to the title screen.
struct ScriptContext;
void PeepoHardcore_FieldCB_GameOver(void);
void PeepoHardcore_GameOverReset(struct ScriptContext *ctx);

#endif // GUARD_PEEPO_HARDCORE_H
