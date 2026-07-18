#include "global.h"
#include "peepo_net.h"

// Custom MMIO mailbox registers implemented by the patched mGBA fork. They sit
// in the emulator's unused 0x04FFF8xx region, alongside mGBA's debug/printf
// block (0x04FFF6xx), and are reached with plain LDRH/STRH/STRB.
#define REG_NET_ENABLE  (*(vu16 *)0x4FFF800) // W magic 0x504E -> R 0x1DEA if present
#define REG_NET_TX_SEND (*(vu16 *)0x4FFF802) // W(len): doorbell, emit TX buffer
#define REG_NET_RX_LEN  (*(vu16 *)0x4FFF804) // R: pending inbox length; W0: ack
#define NET_TX_BUF      ((vu8 *)0x4FFF900)    // outgoing packet bytes (byte writes OK)
// The emulator serves the RX buffer via 16-bit GBAIORead, so read it as u16
// words and unpack (byte reads of this MMIO region are not synthesized).
#define NET_RX_BUF16    ((vu16 *)0x4FFFA00)   // incoming packet, 16-bit reads

#define NET_MAGIC 0x504E // 'PN'
#define NET_ACK   0x1DEA

bool32 PeepoNet_Open(void)
{
    REG_NET_ENABLE = NET_MAGIC;
    return REG_NET_ENABLE == NET_ACK;
}

void PeepoNet_Send(const u8 *data, u32 len)
{
    u32 i;
    if (len > NET_MAX)
        len = NET_MAX;
    for (i = 0; i < len; i++)
        NET_TX_BUF[i] = data[i];
    REG_NET_TX_SEND = len; // doorbell
}

u32 PeepoNet_Poll(u8 *out)
{
    u32 i;
    u32 len = REG_NET_RX_LEN;
    if (len == 0)
        return 0;
    if (len > NET_MAX)
        len = NET_MAX;
    for (i = 0; i < len; i += 2)
    {
        u16 w = NET_RX_BUF16[i >> 1];
        out[i] = w & 0xFF;
        if (i + 1 < len)
            out[i + 1] = w >> 8;
    }
    REG_NET_RX_LEN = 0; // acknowledge
    return len;
}
