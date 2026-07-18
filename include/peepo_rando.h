#ifndef GUARD_PEEPO_RANDO_H
#define GUARD_PEEPO_RANDO_H

// Which kind of species lookup is being remapped. All three are gated on the
// species option (RANDO_F_SPECIES) and share the same mode logic.
#define RANDO_KIND_WILD     0
#define RANDO_KIND_TRAINER  1
#define RANDO_KIND_GIFT     2  // script-given mons: starters, gift Pokémon

// Bits stored in gSaveBlock2Ptr->peepoRandoFlags. Chosen once in the new-game
// setup menu and then baked into the save (the seed is the player's Trainer ID).
#define RANDO_F_WILD            (1 << 0)  // randomize wild + gift/starter species
#define RANDO_F_SCALED          (1 << 1)  // replacement must have a similar base-stat total (shared mode)
#define RANDO_F_LEGENDARY_AWARE (1 << 2)  // legendaries stay legendary (shared mode)
#define RANDO_F_ABILITY         (1 << 3)  // randomize abilities from a curated pool
#define RANDO_F_TRAINER         (1 << 4)  // randomize trainer parties
#define RANDO_F_TRAINER_BOSSES  (1 << 5)  // ...including gym leaders / boss trainers
#define RANDO_F_HARDCORE        (1 << 6)  // enforced nuzlocke (see peepo_hardcore)

struct Trainer;

// Deterministic per-save species remap. Returns the species unchanged when the
// species option is off. The same input species always maps to the same output
// within a save, so a given wild/trainer/gift mon is stable across encounters.
u16 PeepoRando_MapSpecies(u16 species, u8 kind);

// Deterministic per-save ability roll from the curated allowed pool. Keyed on
// (species, abilityNum) so a mon's normal and hidden slots differ but are stable.
u16 PeepoRando_RandomAbility(u16 species, u8 abilityNum);

// TRUE for "boss" trainers (gym leaders / Elite Four / Champion).
bool8 PeepoRando_IsBossTrainer(const struct Trainer *trainer);

// TRUE if this trainer's party should be randomized, per the Trainers setting
// (Off / All / No Bosses).
bool8 PeepoRando_ShouldRandomizeTrainer(const struct Trainer *trainer);

#endif // GUARD_PEEPO_RANDO_H
