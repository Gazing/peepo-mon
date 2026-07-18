#ifndef GUARD_PEEPO_MAPEDIT_H
#define GUARD_PEEPO_MAPEDIT_H

#include "global.h"

// PEEPO MAP EDIT — the collaborative, server-persisted "build mode". Players
// place/replace/delete metatiles on any overworld map; the edits live purely in
// the global MapWorld Durable Object (never in the ROM save) and are shared with
// every other player in real time. Warps and scripted tiles are protected.
//
// Runs on the PeepoNet mailbox, sharing the transport with the presence netcode.
// Map-plane opcodes are 0x10+ to stay clear of the presence packets.

// Per-frame tick from the main loop. Detects when the local player enters a new
// map and requests that map's persisted edits, so the shared world always
// renders — even when the editor UI is closed.
void PeepoMapEdit_Update(void);

// Feed an inbound network packet. Handles the map plane (MAP_SNAP / MAP_EDIT)
// and ignores anything else. Called from the PeepoNet drain in peepo_overworld.
void PeepoMapEdit_OnPacket(const u8 *p, u32 n);

// Open the in-game tile editor (called from the QOL menu). The caller must have
// already frozen/locked the field; the editor unlocks it again on exit.
void PeepoMapEdit_Enter(void);

// HM field moves on editor-placed objects. The Cut/Rock Smash/Strength setup
// functions call PeepoMapEdit_PlacedFieldMoveTarget(gfx) to claim a matching placed
// object in front of the player (returns TRUE and stashes it); if so they set
// PeepoMapEdit_DoFieldMovePlaced as gPostMenuFieldCallback, which applies it —
// cut/smash delete the object, Strength shoves a boulder — all networked. Real
// map obstacles aren't in our list, so the target check returns FALSE for them and
// the vanilla field-move handles them.
bool8 PeepoMapEdit_PlacedFieldMoveTarget(u16 gfx);
void PeepoMapEdit_DoFieldMovePlaced(void);

// A-press parity: pressing A facing a placed tree/rock/boulder runs the same networked
// cut/smash/push as the field move, but ONLY if the player can use that HM (badge + a
// party mon that knows it) — so placed objects respond to A like real obstacles do.
// Returns TRUE if it acted (caller then runs no dialog). Called from the interaction
// resolver in field_control_avatar.c.
bool8 PeepoMapEdit_TryAPressFieldMove(u8 localId);

#endif // GUARD_PEEPO_MAPEDIT_H
