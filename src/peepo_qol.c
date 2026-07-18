#include "global.h"
#include "peepo_qol.h"
#include "menu.h"
#include "window.h"
#include "text.h"
#include "string_util.h"
#include "sound.h"
#include "task.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "event_object_movement.h"
#include "event_object_lock.h"
#include "overworld.h"
#include "script.h"
#include "item_menu.h"
#include "dexnav.h"
#include "peepo_mapedit.h"
#include "script_pokemon_util.h"
#include "main.h"
#include "region_map.h"
#include "field_move.h"
#include "constants/field_move.h"
#include "constants/songs.h"
#include "constants/flags.h"

extern const u8 PeepoHealScript[]; // data/event_scripts.s — Poke Vial full-heal + dialog

// Left-side field menu opened by SELECT. Modeled on the start menu's field flow:
// freeze the field, draw a std-framed window + cursor, run a task for input, then
// unfreeze + unlock on close.

enum {
    QOL_ITEM_POKEVIAL,
    QOL_ITEM_AUTORUN,
    QOL_ITEM_FOLLOWER,
    QOL_ITEM_SUPERREPEL,
    QOL_ITEM_USEITEM,
    QOL_ITEM_EDITMAP,
    QOL_ITEM_EXIT,
    QOL_ITEM_COUNT
};

#define QOL_ROW_TOP     8    // pixel y of the first row (also the cursor top)
#define QOL_WIN_BASEBLOCK 0x139  // shared with the (mutually exclusive) start menu

// {bg, fg, shadow}. A toggled-on option is shown in green (like the randomizer's
// active choice); everything else uses the normal dark text.
static const u8 sQolColorNormal[] = { TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY };
static const u8 sQolColorGreen[]  = { TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_LIGHT_GREEN };

static const struct WindowTemplate sQolWindowTemplate = {
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = 11,
    .height = (QOL_ITEM_COUNT * 2) + 2,
    .paletteNum = 15,
    .baseBlock = QOL_WIN_BASEBLOCK,
};

static EWRAM_DATA u8 sQolWindowId = 0;
static EWRAM_DATA s8 sQolCursorPos = 0;

static const u8 sText_PokeVial[]   = _("Poke Vial");
static const u8 sText_AutoRun[]    = _("Auto Run");
static const u8 sText_Follower[]   = _("Follower");
static const u8 sText_SuperRepel[] = _("Super Repel");
static const u8 sText_UseItem[]    = _("Use Item");
static const u8 sText_EditMap[]    = _("Edit Map");
static const u8 sText_Exit[]       = _("Exit");

static void Task_PeepoQol(u8 taskId);

// A toggle option prints green when on; action rows and off-toggles print normal.
static void PrintRow(u8 i, const u8 *label, bool8 on)
{
    u8 y = (i * 16) + QOL_ROW_TOP;
    AddTextPrinterParameterized3(sQolWindowId, FONT_NORMAL, 8, y,
        on ? sQolColorGreen : sQolColorNormal, TEXT_SKIP_DRAW, label);
}

static void PeepoQol_PrintItems(void)
{
    PrintRow(QOL_ITEM_POKEVIAL,   sText_PokeVial,   FALSE);
    PrintRow(QOL_ITEM_AUTORUN,    sText_AutoRun,    FlagGet(FLAG_PEEPO_AUTORUN));
    PrintRow(QOL_ITEM_FOLLOWER,   sText_Follower,   !FlagGet(B_FLAG_FOLLOWERS_DISABLED));
    PrintRow(QOL_ITEM_SUPERREPEL, sText_SuperRepel, FlagGet(FLAG_PEEPO_SUPER_REPEL));
    PrintRow(QOL_ITEM_USEITEM,    sText_UseItem,    FALSE);
    PrintRow(QOL_ITEM_EDITMAP,    sText_EditMap,    FALSE);
    PrintRow(QOL_ITEM_EXIT,       sText_Exit,       FALSE);
}

// Re-fill the content, reprint the rows and re-place the cursor. Used both for
// the initial draw and after a toggle changes an On/Off state.
static void PeepoQol_Refresh(void)
{
    FillWindowPixelBuffer(sQolWindowId, PIXEL_FILL(1));
    PeepoQol_PrintItems();
    sQolCursorPos = InitMenuNormal(sQolWindowId, FONT_NORMAL, 0, QOL_ROW_TOP, 16, QOL_ITEM_COUNT, sQolCursorPos);
}

static void PeepoQol_DrawWindow(void)
{
    sQolWindowId = AddWindow(&sQolWindowTemplate);
    PutWindowTilemap(sQolWindowId);
    DrawStdWindowFrame(sQolWindowId, FALSE);
    PeepoQol_Refresh();
    CopyWindowToVram(sQolWindowId, COPYWIN_MAP);
}

static void PeepoQol_CloseWindow(void)
{
    ClearStdWindowAndFrame(sQolWindowId, TRUE);
    RemoveWindow(sQolWindowId);
}

// Close the menu and hand control back to the overworld.
static void PeepoQol_Close(u8 taskId)
{
    PeepoQol_CloseWindow();
    ScriptUnfreezeObjectEvents();
    UnlockPlayerFieldControls();
    DestroyTask(taskId);
}

void PeepoQol_Open(void)
{
    PlaySE(SE_WIN_OPEN);
    FreezeObjectEvents();
    PlayerFreeze();
    StopPlayerAvatar();
    LockPlayerFieldControls();
    // The window is drawn in the task's first step (next frame) to avoid touching
    // window VRAM in the middle of field input processing.
    CreateTask(Task_PeepoQol, 0x50);
}

static void PeepoQol_OnSelect(u8 taskId)
{
    switch (sQolCursorPos)
    {
    case QOL_ITEM_POKEVIAL:
        PlaySE(SE_SELECT);
        // Close the menu and hand off to a field script that full-heals the party and
        // shows a "healed" dialog (like a Poke Center), then returns to the overworld.
        PeepoQol_CloseWindow();
        DestroyTask(taskId);
        ScriptContext_SetupScript(PeepoHealScript);
        break;
    case QOL_ITEM_AUTORUN:
        PlaySE(SE_SELECT);
        FlagToggle(FLAG_PEEPO_AUTORUN);
        PeepoQol_Refresh();
        CopyWindowToVram(sQolWindowId, COPYWIN_GFX);
        break;
    case QOL_ITEM_FOLLOWER:
        PlaySE(SE_SELECT);
        FlagToggle(B_FLAG_FOLLOWERS_DISABLED);
        UpdateFollowingPokemon(); // spawns or removes per the flag
        PeepoQol_Refresh();
        CopyWindowToVram(sQolWindowId, COPYWIN_GFX);
        break;
    case QOL_ITEM_SUPERREPEL:
        PlaySE(SE_SELECT);
        FlagToggle(FLAG_PEEPO_SUPER_REPEL);
        PeepoQol_Refresh();
        CopyWindowToVram(sQolWindowId, COPYWIN_GFX);
        break;
    case QOL_ITEM_USEITEM:
        PlaySE(SE_SELECT);
        PeepoQol_CloseWindow();
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
        DestroyTask(taskId);
        UseRegisteredKeyItemOnField(); // SELECT's normal behavior, on demand
        break;
    case QOL_ITEM_EDITMAP:
        PlaySE(SE_SELECT);
        // Hand off to the map editor. Keep the field frozen/locked (do NOT
        // unfreeze) — the editor takes over input and unlocks on its own exit.
        PeepoQol_CloseWindow();
        DestroyTask(taskId);
        PeepoMapEdit_Enter();
        break;
    case QOL_ITEM_EXIT:
    default:
        PlaySE(SE_SELECT);
        PeepoQol_Close(taskId);
        break;
    }
}

static void PeepoQol_HandleInput(u8 taskId)
{
    if (JOY_NEW(DPAD_UP))
    {
        PlaySE(SE_SELECT);
        sQolCursorPos = Menu_MoveCursor(-1);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        PlaySE(SE_SELECT);
        sQolCursorPos = Menu_MoveCursor(1);
    }
    else if (JOY_NEW(A_BUTTON))
    {
        PeepoQol_OnSelect(taskId);
    }
    else if (JOY_NEW(B_BUTTON) || JOY_NEW(SELECT_BUTTON))
    {
        PlaySE(SE_SELECT);
        PeepoQol_Close(taskId);
    }
}

static void Task_PeepoQol(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    switch (data[0])
    {
    case 0:
        PeepoQol_DrawWindow();
        data[0] = 1;
        break;
    case 1:
        PeepoQol_HandleInput(taskId);
        break;
    }
}

bool8 PeepoQol_WantsRun(u16 heldKeys)
{
    bool8 bHeld = (heldKeys & B_BUTTON) != 0;
    if (FlagGet(FLAG_PEEPO_AUTORUN))
        return !bHeld; // auto-run: run by default, hold B to walk
    return bHeld;      // normal: hold B to run
}
