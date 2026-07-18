#ifndef GUARD_PEEPO_OVERWORLD_H
#define GUARD_PEEPO_OVERWORLD_H

#include "global.h"

// Multiplayer overworld presence built on the PeepoNet mailbox transport.
// Broadcasts the local player's map + position + facing, and renders remote
// players as overworld object events when they are on the same map. Called once
// per frame from the main loop.
void PeepoOverworld_Update(void);

// If `localId` is a remote player's avatar, buffers their name into gStringVar1
// and returns the greeting field script; otherwise NULL. Called from the engine's
// object-interaction resolver so pressing A on a remote greets them in-game.
const u8 *PeepoOverworld_GetRemoteInteractScript(u8 localId);

#endif // GUARD_PEEPO_OVERWORLD_H
