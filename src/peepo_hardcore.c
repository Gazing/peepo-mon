#include "global.h"
#include "peepo_hardcore.h"
#include "peepo_rando.h"
#include "pokemon.h"
#include "pokemon_storage_system.h" // CompactPartySlots
#include "overworld.h"
#include "fieldmap.h"
#include "field_screen_effect.h" // FadeInFromBlack
#include "palette.h"             // gPaletteFade
#include "save.h"
#include "main.h"
#include "script.h"
#include "task.h"
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

// ---- Hardcore game over -----------------------------------------------------
// The game-over message must NOT run synchronously from DoWhiteOut:
// RunScriptImmediately spins the script context in a tight C loop, but msgbox's
// waitmessage only advances via tasks that loop never lets run — a hard freeze
// before the save was ever erased. Instead the whiteout warps home normally and
// CB2_WhiteOut installs this field callback, which erases the save while the
// screen is still black, fades in, and schedules the message script; the
// script's final callnative reboots. Erase-before-message makes game over
// irreversible (power-cycling at the message can't rescue the save) — to get
// the lenient order instead, move ClearSaveData into PeepoHardcore_GameOverReset.

static void Task_PeepoHardcoreGameOver(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        ScriptContext_SetupScript(EventScript_PeepoHardcoreGameOver);
        DestroyTask(taskId);
    }
}

void PeepoHardcore_FieldCB_GameOver(void)
{
    ClearSaveData(); // erase every save sector (screen is still black)
    FadeInFromBlack();
    CreateTask(Task_PeepoHardcoreGameOver, 10);
    LockPlayerFieldControls();
}

// callnative target at the end of EventScript_PeepoHardcoreGameOver.
void PeepoHardcore_GameOverReset(struct ScriptContext *ctx)
{
    DoSoftReset(); // no save left -> title screen offers only New Game
}
