#ifndef GUARD_PEEPO_NET_H
#define GUARD_PEEPO_NET_H

// PEEPO NET — application-level packet mailbox between this ROM and the
// (patched) mGBA emulator, which bridges packets to the network. This is NOT
// the GBA link cable / SIO lockstep path; it is a latency-tolerant byte pipe
// over a custom MMIO block in the emulator's unused 0x04FFF8xx region (see
// GBA_REG_NET_* in the mGBA fork's io.h).

#include "global.h"

#define NET_MAX 256 // max packet size (matches the emulator mailbox buffers)

// Object-event localIds owned by the PeepoNet layer — transient, networked/placed
// object events (placed objects 0xE0-0xE7, editor cursor 0xEF, remote players
// 0xF0-0xF3, their followers 0xF4-0xF7). These are re-derived from the network
// every map load, so they must NEVER be written to / restored from the save file
// (see load_save.c). Kept clear of the engine's reserved ids (0x7F camera,
// 0xFD/0xFE/0xFF npc-follower/follower/player).
#define PEEPO_NET_LOCALID_MIN 0xE0
#define PEEPO_NET_LOCALID_MAX 0xF7
#define PeepoNet_IsNetworkedLocalId(id) ((id) >= PEEPO_NET_LOCALID_MIN && (id) <= PEEPO_NET_LOCALID_MAX)

// Handshake with the emulator bridge. Returns TRUE if the mailbox is present.
bool32 PeepoNet_Open(void);

// Queue an outgoing packet (<=256 bytes) and ring the send doorbell.
void PeepoNet_Send(const u8 *data, u32 len);

// Drain one inbound packet into `out` (<=256 bytes); returns its length (0 if
// none pending) and acknowledges it to the emulator.
u32 PeepoNet_Poll(u8 *out);

#endif // GUARD_PEEPO_NET_H
