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
 *  - VERB is task-144's addition, and it is LIVE ONLY UNDER Kindled.Ability.Mode 1 (Q23 = B).
 *    Under Mode 0 (Q23 = A) this column is inert: all four verbs belong to the bearer and
 *    apply to whichever of the seven are in range, so which soldier "carries" what does not
 *    exist as a concept. Both modes flip inside one session, on purpose.
 *
 *    STILL NOT AN ANSWER TO Q23. The register says the kit "should not be settled by whatever
 *    gets prototyped first", so this table implements ONE of the two options rather than
 *    choosing it, and the binding itself is ability-kit.md §2 B's own suggested one, not a new
 *    proposal: line/Vanguard, guardian/Relickeeper, hunter/Pathfinder, light/Guided each take
 *    the verb that section pairs them with. Vanguard takes Banner Slam rather than §2's
 *    "reposition" because the shipped stance set already IS reposition (ESwarmStance::Hold
 *    anchors a unit on the ground it was called on, which is Shield Wall's mechanical text),
 *    and the four verbs the task names are focus / screen / raise / rally.
 *
 *    THE SHAPE OF THIS COLUMN IS ITSELF EVIDENCE. Seven soldiers over four verbs means three
 *    verbs are doubled and one (Kindle, on Ember) is held by exactly one body. So losing Ember
 *    loses raise for the run, while losing Wren still leaves Kite able to mark — which is
 *    precisely the "is the choice hollow?" question ability-kit.md §5 says to watch under
 *    Q23 = B, made concrete instead of argued. It is not balanced and is not meant to be.
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
		ESquadVerb Verb;		// Q23 = B only — inert under Kindled.Ability.Mode 0
	};

	inline constexpr int32 Num = USwarmSubsystem::NamedSoldiers;

	inline const FSoldier& Get(int32 Index)
	{
		static const FSoldier Table[Num] = {
			// Vanguard — the many. The line: closes, holds contact, takes the hits.
			// Banner Slam: haste + leash exemption on the ground it is planted on.
			{ TEXT("Ash"),   TEXT("VANGUARD"),    EUnitType::Spearmen, 7,  2, 4.f, ESquadVerb::Rally },	// v8_heavycloak — the widest, heaviest read
			{ TEXT("Rook"),  TEXT("VANGUARD"),    EUnitType::Spearmen, 3,  2, 4.f, ESquadVerb::Rally },	// v3_shieldbreak
			// Relickeeper — the tough. The guardian. Ward Circle: damage taken cut inside it.
			{ TEXT("Cairn"), TEXT("RELICKEEPER"), EUnitType::Spearmen, 10, 2, 4.f, ESquadVerb::Screen },	// v13_maceraised
			{ TEXT("Slate"), TEXT("RELICKEEPER"), EUnitType::Spearmen, 8,  2, 4.f, ESquadVerb::Screen },	// v10_bracedstaff — light but a 46px footprint
			// Pathfinder — the few. The hunter: never closes, reaches 750uu.
			// Mark Quarry: the pack's blows reach the marked boss through the wave in the way.
			{ TEXT("Wren"),  TEXT("PATHFINDER"),  EUnitType::Archers,  2,  2, 4.f, ESquadVerb::Focus },	// ghilliecloak
			{ TEXT("Kite"),  TEXT("PATHFINDER"),  EUnitType::Archers,  0,  2, 4.f, ESquadVerb::Focus },	// hoodedbow
			// Lampbearer — the light. Kindle now has a real effect (heal over time on one of
			// the seven), but only its first half: no overheal shield, and nothing revives —
			// Q15 is open. The one body holding raise, which is the point. See the header.
			{ TEXT("Ember"), TEXT("LAMPBEARER"),  EUnitType::Archers,  5,  2, 4.f, ESquadVerb::Raise },	// crystalstaff
		};
		return Table[FMath::Clamp(Index, 0, Num - 1)];
	}
}
