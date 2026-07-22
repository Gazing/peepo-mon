# peepo-mon

Multiplayer Pokémon Emerald romhack base: [rh-hideout/pokeemerald-expansion](https://github.com/rh-hideout/pokeemerald-expansion) **1.14.0** plus ~4,000 lines of custom code. Working branch: `peepo`. Upstream master has moved far ahead (1.15-dev, breaking changes) — this base pins 1.14 deliberately; don't "helpfully" rebase.

## Build

`make MODERN=1` on ubuntu-22.04 + gcc-arm-none-eabi (10.x; newer GCCs also build cleanly). CI (`.github/workflows/build.yml`) mirrors this. Building a tree without `.git` (zip/`git archive`) requires `touch .histignore` first — the tools stage fatals without git history otherwise.

## Custom modules (all peepo-prefixed, all in `src/`)

| File | What it is |
|---|---|
| `peepo_overworld.c` | Multiplayer netcode: remote avatars + followers, map-seam translation, admin commands (force encounter, teleport-to-player), roster/timeout lifecycle |
| `peepo_net.c` | Packet transport under the overworld sync |
| `peepo_mapedit.c` | In-game map editor: tile brush + placed objects (`sObjs` logical records → `sPoolObj` render pool → object events at localIds 0xE0–0xE7; cursor 0xEF; remotes 0xF0+) |
| `peepo_rando.c` | Randomizer: **deterministic per-save hash remap** (seeded by Trainer ID, zero `Random()` calls — keep it that way) |
| `peepo_qol.c` | SELECT-button QOL menu (auto-run, super repel, heal, DexNav open) |
| `peepo_hardcore.c` | Enforced nuzlocke: faint = release, whiteout = save wipe, one catch per route |
| `bw_summary_screen.c` | Vendored ravepossum BW summary screen (third-party, integrated over 1.14) |

`main_menu.c` adds the randomizer/hardcore new-game screens as a contiguous ~313-line append plus three redirect lines (`gTasks[taskId].func = …`), not a rewrite of the stock flow. Most hooks into stock files carry a `peepo` comment, but a few don't (evo-stone mart pricing in `item.c` + `data/maps/RustboroCity_Mart/scripts.inc`, `include/config/dexnav.h`, `src/battle_controller_player.c`) — so `grep -i peepo` is not exhaustive; the authoritative list of peepo changes is `git diff da5a2ca7 -- <path>` (the 1.14.0 base).

## Invariants that have already caused real bugs — check these before changing related code

- **The wild randomizer remaps exactly once, inside `CreateWildMon()`.** Anything passing an *already-remapped* species must use `CreateWildMonExact()` instead, or the species double-remaps and the battle won't match what was displayed (that was DexNav bug PR #1).
- **`RunScriptImmediately` must never run a script containing `waitmessage`/`waitbuttonpress`** (or any command that needs task frames): it spins synchronously and those commands only advance via `RunTasks()` — instant hard freeze. Schedule via `ScriptContext_SetupScript` from a task or field callback instead; note `CB2_WhiteOut` calls `ScriptContext_Init` after `DoWhiteOut`, wiping anything scheduled earlier.
- **DexNav search lifecycle**: `DN_FLAG_SEARCHING` is set in `Task_SetUpDexNavSearch` for both search modes and must be cleared on every end path; the search task's function pointer changes during hidden-mon reveal, so anything that looks tasks up by function must handle both `Task_DexNavSearch` and `Task_RevealHiddenMon`.
- **Map-editor render bookkeeping is keyed by `sObjs` index only.** Reusing an index for a logically different/moved object without a `ReconcileObjects()` in between leaves the on-field object event stale (the Strength-push desync).
- **`SetWarpDestination`'s x/y parameters are s8** even though `WarpData` stores s16 — map-local coords ≥ 128 truncate. Use `SetWarpDestinationXY16` (PR #3) for arbitrary positions.

## Before an upstream (pokeemerald-expansion) bump — hand-audit gate

The randomizer/hardcore save data repurposes upstream **filler bytes**: `peepoRandoFlags` at SaveBlock2 `0x90` (from `filler_90`) and `peepoCaughtRoutes` at SaveBlock1 `0x988` (from `filler1`). Upstream reclaims filler routinely. If a new release allocates those same bytes, git merges both **silently** — no conflict marker, no compile error — and corrupts saves. Unlike every other hook (which fails loudly on a bump), this one is invisible. **On any upstream bump, hand-check both offsets in `include/global.h` before shipping.** Merge-labor ranking otherwise: `bw_summary_screen.c` (vendored, pinned to 1.14 APIs, merges clean and breaks at runtime — revalidate it) and `src/dexnav.c` (~46 hunks) are the real work; the battle-file hooks are tiny and re-apply trivially.

## Known debt

- Littleroot test-starter hook in `wild_encounter.c` (`sPeepoTestStarters`) ships ungated — its own comment says remove for production.

## Contributing (human or AI-assisted)

One defect per branch per PR — never bundle unrelated changes, so a regression bisects to one small diff. PR bodies state the failure scenario and how the fix was verified (compile-only vs play-tested — say which honestly). AI-authored commits carry `Co-Authored-By: Claude <noreply@anthropic.com>` (or the equivalent for the tool used). Fork note: GitHub disables Actions on forks until enabled once in the Actions tab.

Claude Code auto-loads this file; Codex auto-loads `AGENTS.md`, which is a thin pointer here — keep instructions in this one document.
