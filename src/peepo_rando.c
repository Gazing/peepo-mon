#include "global.h"
#include "peepo_rando.h"
#include "pokemon.h"
#include "data.h"
#include "battle_transition.h" // enum MugshotColor (boss-trainer marker)
#include "constants/species.h"
#include "constants/abilities.h"
#include "constants/trainers.h" // trainer classes (boss detection)

// A scaled-mode replacement's base-stat total must be within this of the original.
#define PEEPO_RANDO_BST_BAND   70
// Rejection-sampling attempt cap before giving up and keeping the original.
#define PEEPO_RANDO_MAX_TRIES  64

// Integer hash (Murmur-style finalizer). Fully deterministic: identical inputs
// always yield the same output, which is what makes a remap stable.
static u32 PeepoRandoHash(u32 seed, u32 key, u32 salt)
{
    u32 x = seed ^ (key * 2654435761u) ^ (salt * 40503u);
    x = (x ^ (x >> 16)) * 2246822519u;
    x = (x ^ (x >> 13)) * 3266489917u;
    x =  x ^ (x >> 16);
    return x;
}

// The per-save seed. The Trainer ID is generated once at new game and is unique
// per save, so two saves get two different (but internally consistent) worlds.
static u32 PeepoRandoSeed(void)
{
    const u8 *tid = gSaveBlock2Ptr->playerTrainerId;
    return (u32)tid[0] | ((u32)tid[1] << 8) | ((u32)tid[2] << 16) | ((u32)tid[3] << 24);
}

static bool8 IsLegendaryLike(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return info->isLegendary || info->isMythical || info->isUltraBeast;
}

// Rejects gap/undefined dex slots and battle-only forms that must not spawn in
// the overworld (Mega/Primal/Ultra Burst/Gigantamax/Tera/Totem). Regional forms
// are intentionally allowed.
static bool8 IsSpawnableSpecies(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    if (info->baseHP == 0)
        return FALSE;
    if (info->isMegaEvolution || info->isPrimalReversion || info->isUltraBurst
        || info->isGigantamax || info->isTeraForm || info->isTotem)
        return FALSE;
    return TRUE;
}

static u32 SpeciesBST(u16 species)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    return info->baseHP + info->baseAttack + info->baseDefense
         + info->baseSpeed + info->baseSpAttack + info->baseSpDefense;
}

u16 PeepoRando_MapSpecies(u16 species, u8 kind)
{
    u8 flags = gSaveBlock2Ptr->peepoRandoFlags;
    bool8 scaled, legendaryAware, srcLegend;
    u32 srcBst, seed;
    u16 i;

    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return species;
    if (kind == RANDO_KIND_TRAINER)
    {
        if (!(flags & RANDO_F_TRAINER))
            return species;
    }
    else // WILD / GIFT share the wild toggle + mode
    {
        if (!(flags & RANDO_F_WILD))
            return species;
    }

    scaled = (flags & RANDO_F_SCALED) != 0;
    legendaryAware = (flags & RANDO_F_LEGENDARY_AWARE) != 0;

    srcLegend = IsLegendaryLike(species);
    srcBst = scaled ? SpeciesBST(species) : 0;
    seed = PeepoRandoSeed();

    for (i = 0; i < PEEPO_RANDO_MAX_TRIES; i++)
    {
        u16 cand = 1 + (PeepoRandoHash(seed, species, i) % (NUM_SPECIES - 1));
        bool8 candLegend;

        if (!IsSpawnableSpecies(cand))
            continue;

        candLegend = IsLegendaryLike(cand);
        if (legendaryAware)
        {
            if (candLegend != srcLegend) // keep legendary status on both sides
                continue;
        }
        else if (candLegend)             // never turn anything into a legendary
        {
            continue;
        }

        if (scaled)
        {
            u32 candBst = SpeciesBST(cand);
            u32 diff = (candBst > srcBst) ? (candBst - srcBst) : (srcBst - candBst);
            if (diff > PEEPO_RANDO_BST_BAND)
                continue;
        }

        return cand;
    }

    return species; // nothing acceptable found within the attempt cap
}

// Curated ability pool. Excludes abilities that are broken (Wonder Guard),
// unpredictable/useless (Color Change, Klutz), or tied to a specific species'
// form/transform mechanic (Stance Change, Multitype, Disguise, ...), which would
// do nothing or misbehave on a random mon. Everything else is fair game.
static bool8 IsAllowedRandomAbility(u16 ability)
{
    switch (ability)
    {
    case ABILITY_NONE:
    case ABILITY_WONDER_GUARD:
    case ABILITY_COLOR_CHANGE:
    case ABILITY_KLUTZ:
    case ABILITY_FORECAST:
    case ABILITY_FLOWER_GIFT:
    case ABILITY_MULTITYPE:
    case ABILITY_ZEN_MODE:
    case ABILITY_STANCE_CHANGE:
    case ABILITY_SHIELDS_DOWN:
    case ABILITY_SCHOOLING:
    case ABILITY_DISGUISE:
    case ABILITY_BATTLE_BOND:
    case ABILITY_POWER_CONSTRUCT:
    case ABILITY_RKS_SYSTEM:
    case ABILITY_COMATOSE:
    case ABILITY_GULP_MISSILE:
    case ABILITY_ICE_FACE:
    case ABILITY_HUNGER_SWITCH:
    case ABILITY_ZERO_TO_HERO:
    case ABILITY_COMMANDER:
    case ABILITY_TERA_SHIFT:
    case ABILITY_TERA_SHELL:
    case ABILITY_TERAFORM_ZERO:
        return FALSE;
    default:
        return TRUE;
    }
}

u16 PeepoRando_RandomAbility(u16 species, u8 abilityNum)
{
    u32 seed = PeepoRandoSeed();
    u16 i;

    for (i = 0; i < PEEPO_RANDO_MAX_TRIES; i++)
    {
        // Key on species AND slot so normal vs hidden ability differ but stay stable.
        u16 ability = 1 + (PeepoRandoHash(seed, (u32)species * 4u + abilityNum, i) % (ABILITIES_COUNT - 1));
        if (IsAllowedRandomAbility(ability))
            return ability;
    }

    return gSpeciesInfo[species].abilities[0]; // fallback: the species' own first ability
}

bool8 PeepoRando_IsBossTrainer(const struct Trainer *trainer)
{
    if (trainer == NULL)
        return FALSE;
    // "Boss" = story-significant trainer CLASS. The old mugshot-only check
    // covered exactly the five trainers with mugshots in trainers.party (the
    // Elite Four + Wallace), so "No Bosses" still randomized every Gym Leader,
    // the team leaders/admins, and the rival. Class membership is robust across
    // difficulty variants; the mugshot stays as an opt-in fallback so a custom
    // trainer can be marked a boss by giving it one.
    switch (trainer->trainerClass)
    {
    case TRAINER_CLASS_LEADER:
    case TRAINER_CLASS_ELITE_FOUR:
    case TRAINER_CLASS_CHAMPION:
    case TRAINER_CLASS_MAGMA_LEADER:
    case TRAINER_CLASS_MAGMA_ADMIN:
    case TRAINER_CLASS_AQUA_LEADER:
    case TRAINER_CLASS_AQUA_ADMIN:
    case TRAINER_CLASS_RIVAL: // debatable for the early low-stakes fights — drop this line to randomize rivals
        return TRUE;
    default:
        break;
    }
    return trainer->mugshotColor != MUGSHOT_COLOR_NONE;
}

bool8 PeepoRando_ShouldRandomizeTrainer(const struct Trainer *trainer)
{
    u8 flags = gSaveBlock2Ptr->peepoRandoFlags;

    if (!(flags & RANDO_F_TRAINER))                                          // Trainers: Off
        return FALSE;
    if (!(flags & RANDO_F_TRAINER_BOSSES) && PeepoRando_IsBossTrainer(trainer)) // No Bosses
        return FALSE;
    return TRUE;                                                             // All (or a non-boss)
}
