#ifndef GUARD_PEEPO_QOL_H
#define GUARD_PEEPO_QOL_H

#include "global.h"

// Peepo QOL menu — a compact left-side overworld menu bound to SELECT. Offers
// auto-run + follower toggles, a DexNav shortcut, and the normal registered-item
// use (SELECT's usual job, preserved here since SELECT now opens this menu).

// Persistent per-save toggle flags (from the FLAG_UNUSED_0x02x pool).
#define FLAG_PEEPO_AUTORUN   0x23   // set = auto-run ON (run without holding B).
// Follower show/hide reuses the expansion's own B_FLAG_FOLLOWERS_DISABLED (0x24).
#define FLAG_PEEPO_SUPER_REPEL 0x25 // set = perma-repel ON (wild encounters below the lead mon's level are always blocked, no repel item needed).

// Open the QOL menu (called from the field SELECT handler).
void PeepoQol_Open(void);

// Auto-run resolver used by the field run check: returns whether the player
// should run given the held keys (auto-run inverts the hold-B-to-run rule).
bool8 PeepoQol_WantsRun(u16 heldKeys);

#endif // GUARD_PEEPO_QOL_H
