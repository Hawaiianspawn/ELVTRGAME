#pragma once

#include "CoreMinimal.h"
#include "Mass/SwarmCombat.h" // EUnitType
#include "Mass/SwarmSubsystem.h" // USwarmSubsystem::NamedSoldiers

/**
 * The seven named soldiers — docs/design/castle-layout.md D1, §6.3, §8.
 *
 * WHAT THIS IS NOT. `docs/OPEN-DECISIONS.md` Q2 (composition of the seven — fixed roles,
 * chosen loadout, or survivors) and Q14 (is seven a cap, a floor, or a field size over a
 * bench) are both OPEN, and this table answers neither. It is a fixed authored roster
 * because a slice needs seven concrete bodies to point at, and "fixed roles" is the cheapest
 * of Q2's three options to build — NOT because fixed roles won the argument. If Q2 lands on
 * survivors or on loadout, this table is the thing that gets deleted, and nothing else here
 * assumes its shape.
 *
 * WHAT IT DOES CARRY, and where each column comes from:
 *
 *  - ARCHETYPE is Q29 = A, closed 2026-08-13: the four classes survive the pivot as the
 *    seven's archetypes. `CLASSES.md`'s four — Vanguard (the many), Relickeeper (the tough),
 *    Pathfinder (the few / range), Lampbearer (the light) — are drawn across seven slots at
 *    2/2/2/1. That ratio is a shape, not a decision; there is no canon for it.
 *
 *  - NO VERBS. Deliberately, and this is the single most important omission in the file.
 *    Q23 (the squad-channelled ability kit) is open, it is the largest piece of undesigned
 *    content in the project, and the register says in as many words that it "should not be
 *    settled by whatever gets prototyped first". Giving these seven abilities here would
 *    answer it by accident. The archetype is a LABEL and a LOOK and nothing else until
 *    task-144. What distinguishes them today is their body, their reach and their orders.
 *
 *  - VARIANT is the team-atlas WITHIN-BLOCK index (SwarmSheet::Team — 0-10 spearmen, 0-12
 *    archers), assigned through USwarmSubsystem::SetSquadRung, which is the existing
 *    adaptation handle and not a new mechanism. That one assignment moves the sprite, the
 *    formation detachment, the melee reach and the cleave together (SwarmRenderPack::
 *    VariantFor), so a named soldier cannot end up looking like something it does not fight
 *    like. Indices picked for silhouette separation off docs/data/art/team-variants.json's
 *    measured widths and masses — the heavy cloak against the braced staff against the
 *    hooded bow, not three knights in a row.
 *
 *  - TIER indexes Swarm.TierHP / Swarm.TierDPS (upgrades.json's tier_ladder: freed, militia,
 *    veteran, bannerman). All seven start at veteran, which is a starting position on a
 *    ladder that already exists — castle-layout.md §8's squad ratchet is "the 7 climb the
 *    evolution ladder already specced in adaptation.md", so this is using that spine, not
 *    inventing a second one. Climb one live with `Kindled.Adapt`.
 *
 *  - HPSCALE IS AN EXPEDIENT AND NOTHING ELSE. A veteran is 190 HP; seven of those inside a
 *    wave of 700 die before the player can finish reading their names, which makes it
 *    impossible to judge whether commanding them one at a time feels like anything — the
 *    question this slice exists to ask. There is no canon number for a named soldier's
 *    durability anywhere, because Q2/Q14 have not been answered. 4x is a legibility choice.
 */
namespace SevenRoster
{
	struct FSoldier
	{
		const TCHAR* Name;
		const TCHAR* Archetype;	// CLASSES.md class this slot draws from (Q29 = A)
		EUnitType Type;
		int32 Variant;			// team-atlas WITHIN-BLOCK index
		int32 Tier;				// Swarm.TierHP / Swarm.TierDPS row
		float HPScale;
	};

	inline constexpr int32 Num = USwarmSubsystem::NamedSoldiers;

	inline const FSoldier& Get(int32 Index)
	{
		static const FSoldier Table[Num] = {
			// Vanguard — the many. The line: closes, holds contact, takes the hits.
			{ TEXT("Ash"),   TEXT("VANGUARD"),    EUnitType::Spearmen, 7,  2, 4.f },	// v8_heavycloak — the widest, heaviest read
			{ TEXT("Rook"),  TEXT("VANGUARD"),    EUnitType::Spearmen, 3,  2, 4.f },	// v3_shieldbreak
			// Relickeeper — the tough. The guardian.
			{ TEXT("Cairn"), TEXT("RELICKEEPER"), EUnitType::Spearmen, 10, 2, 4.f },	// v13_maceraised
			{ TEXT("Slate"), TEXT("RELICKEEPER"), EUnitType::Spearmen, 8,  2, 4.f },	// v10_bracedstaff — light but a 46px footprint
			// Pathfinder — the few. The hunter: never closes, reaches 750uu.
			{ TEXT("Wren"),  TEXT("PATHFINDER"),  EUnitType::Archers,  2,  2, 4.f },	// ghilliecloak
			{ TEXT("Kite"),  TEXT("PATHFINDER"),  EUnitType::Archers,  0,  2, 4.f },	// hoodedbow
			// Lampbearer — the light. No healing exists in the sim, so today this slot is a
			// distinct body and a distinct name and no more. That gap is real and recorded.
			{ TEXT("Ember"), TEXT("LAMPBEARER"),  EUnitType::Archers,  5,  2, 4.f },	// crystalstaff
		};
		return Table[FMath::Clamp(Index, 0, Num - 1)];
	}
}
