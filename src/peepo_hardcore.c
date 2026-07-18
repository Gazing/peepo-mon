#include "global.h"
#include "peepo_hardcore.h"
#include "peepo_rando.h"
#include "pokemon.h"
#include "pokemon_storage_system.h" // CompactPartySlots
#include "overworld.h"
#include "fieldmap.h"
#include "save.h"
#include "main.h"
#include "script.h"
#include "constants/species.h"

// Game-over message shown on a hardcore whiteout (data/event_scripts.s).
extern const u8 EventScript_PeepoHardcoreGameOver[];

bool8 PeepoHardcore_IsEnabled(void)
{
    return (gSaveBlock2Ptr->peepoRandoFlags & RANDO_F_HARDCORE) != 0;
}

// The current "route" is the region map section of the map we're on. Its id is a
// u8, so the 0x20-byte (256-bit) bitset in SaveBlock1 covers every possible value.
static u8 CurrentMapSec(void)
{
    return gMapHeader.regionMapSectionId;
}

bool8 PeepoHardcore_RouteAlreadyCaught(void)
{
    u8 sec = CurrentMapSec();
    return (gSaveBlock1Ptr->peepoCaughtRoutes[sec >> 3] & (1 << (sec & 7))) != 0;
}

void PeepoHardcore_MarkRouteCaught(void)
{
    u8 sec = CurrentMapSec();
    gSaveBlock1Ptr->peepoCaughtRoutes[sec >> 3] |= (1 << (sec & 7));
}

void PeepoHardcore_ReleaseFainted(void)
{
    u32 i;
    bool8 released = FALSE;

    if (!PeepoHardcore_IsEnabled())
        return;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            continue;
        if (GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL))
            continue; // an egg hasn't fainted, it just has 0 "HP"
        if (GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) == 0)
        {
            ZeroMonData(&gPlayerParty[i]); // release it — gone for good
            released = TRUE;
        }
    }

    if (released)
    {
        CompactPartySlots();
        CalculatePlayerPartyCount();
    }
}

// Called every frame from the main loop. The instant a battle ends (inBattle
// 1->0) we release any mon that fainted — one hook covering every battle type.
void PeepoHardcore_Tick(void)
{
    static bool8 sWasInBattle = FALSE;
    if (sWasInBattle && !gMain.inBattle && PeepoHardcore_IsEnabled())
        PeepoHardcore_ReleaseFainted();
    sWasInBattle = gMain.inBattle;
}

void PeepoHardcore_OnWhiteOut(void)
{
    RunScriptImmediately(EventScript_PeepoHardcoreGameOver);
    ClearSaveData(); // erase every save sector
    DoSoftReset();   // reboot straight to the title screen (no save -> new game)
}
