#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Mass/EntityHandle.h"
#include "SwarmCombat.h"
#include "SwarmFragments.h" // SwarmAnim::TeamBit, used to bucket the live counts
#include "SwarmStats.h"
#include "SwarmSubsystem.generated.h"

/**
 * Shared state for the swarm:
 *  - attractor (hero) position that brood seeks / retinue orbits
 *  - hero HP + the retinue's stance intent (Follow/Charge/Hold/Rally)
 *  - a uniform spatial grid rebuilt each frame (separation, targeting, melee)
 *  - packed render buffers the Niagara bridge reads (positions + anim bits)
 *  - live team counts + benchmark counters
 *
 * Deliberately simple. If profiling says the grid or the buffer packing is the
 * bottleneck, replace with measurement in hand — not before.
 */
UCLASS()
class ELVTR_API USwarmSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * World-space side length of one spatial-hash bucket. QueryNeighbors walks a 3x3
	 * neighbourhood around an entity's own cell, so the true search radius this buys is
	 * GridCellSize * 3 — 750uu at this value. Raised from 200 (task-052) specifically so
	 * Swarm.BroodAggroRange / a future ranged-combat EngageRange of 750uu (squad-group-
	 * system.md §2.2) is physically reachable; at 200 the same 3x3 walk topped out at
	 * 600uu no matter what any CVar asked for. Nothing else in the codebase derives a
	 * value from this constant except FIntPoint bucketing in ToCell() below — confirmed
	 * by grep before this change. Cost tradeoff (fewer, denser buckets = cheaper TMap
	 * overhead but more entries walked per QueryNeighbors call) measured in
	 * docs/perf/grid-cell-size.md; re-check that doc before raising this again.
	 */
	static constexpr float GridCellSize = 250.f;

	// --- typed units (muster UI + command layer) --------------------------
	// docs/design/squad-group-system.md §1.0: "squad" and "unit" are the same thing from
	// here on — SquadId/MaxSquads/squads.json all keep their existing names (a terminology
	// merge, not a code rename) but now name a TYPED thing (§1.1: Spearmen, Archers).
	static constexpr int32 MaxSquads = 8;        // hard cap; the shared command-handle budget
	static constexpr int32 SquadTargetSize = 20; // legacy per-unit formation-slot reference size

	// --- the seven, and the war (docs/design/slice-a7.md, castle-layout.md D1) ----------
	// The eight command handles split: 0-6 are the SEVEN named soldiers, one body each, and
	// handle 7 is the whole autonomous garrison. That split is a PROTOTYPE EXPEDIENT, not a
	// verdict on Q25 (Mass entities vs promoted Actors) — it is simply the cheapest thing
	// that already exists, since every per-unit stance/rung/credit/centroid facility below
	// works unchanged whether a handle holds one body or a hundred.
	//
	// The garrison sitting in the LAST handle rather than the first is what lets the seven
	// keep the low, stable indices a HUD and a console command are addressed by, and what
	// makes "is this handle the player's?" a single < comparison everywhere.
	static constexpr int32 NamedSoldiers = 7;
	static constexpr int32 GarrisonUnit = MaxSquads - 1;
	static constexpr bool IsNamedUnit(int32 UnitIndex) { return UnitIndex >= 0 && UnitIndex < NamedSoldiers; }

	/**
	 * Per-type legibility ceiling (§4.1's `squad_size_legibility_ceiling`, docs/data/
	 * squads.json). A type's derived unit COUNT is ceil(Pool(type) / this), recomputed at
	 * every recruit and every formation repack — see AssignRecruit and §4.1's formula.
	 *
	 * 16 since 2026-08-04 (owner call: every unit is a full 8x2 "mini retinue"). With the
	 * 128-body retinue split 96/32 by the deterministic type quota in SwarmCommands.cpp,
	 * this lands exactly 6 Spearmen units + 2 Archers units, all eight full. Was 80,
	 * which derived 2 ragged oversized units from the same pools.
	 */
	static constexpr int32 TypeLegibilityCeiling = 16;

	/**
	 * SquadIdForSlot is GONE (docs/design/squad-group-system.md §1.3): deriving a unit id
	 * from the dense, repackable formation slot index is exactly the defect this task
	 * fixes — that index silently renumbers on any casualty anywhere, which used to be
	 * cosmetic and would not stay cosmetic once a unit owns a stance or a type. SquadId
	 * (now packed with type — see SwarmSquad in SwarmFragments.h) is assigned ONCE, at
	 * recruit time, by AssignRecruit below, and never re-derived from anything that moves.
	 */

	// --- attractor -------------------------------------------------------
	void SetAttractor(const FVector& InLocation) { Attractor = InLocation; }
	FVector GetAttractor() const { return Attractor; }

	// --- Unit Cam spell-cast focus (hookup for the flame-bearer casting) --
	// When the bearer casts a spell FROM the flame, the caster calls SetCastFocus with a world
	// point + an absolute end time; the Unit Cam camera manager pulls focus to it for a "focus
	// punch" until then. Absolute world seconds so the subsystem needs no tick of its own.
	void SetCastFocus(const FVector& Pos, double EndTimeSeconds) { CastFocusPos = Pos; CastFocusEndTime = EndTimeSeconds; }
	bool IsCastFocusActive(double NowSeconds) const { return NowSeconds < CastFocusEndTime; }
	FVector GetCastFocusPos() const { return CastFocusPos; }

	// --- HUD occlusion ----------------------------------------------------
	// Fraction of the viewport height the combat HUD covers along the BOTTOM edge, published
	// by UKindledHud each tick. The hero camera reads it to bias itself so the bearer sits
	// in the middle of the ground you can still see, rather than the middle of the viewport —
	// the rectangle scales with the body count, so this moves under you and can't be a constant.
	void SetHudOccludedFraction(float InFraction) { HudOccludedFraction = FMath::Clamp(InFraction, 0.f, 0.9f); }
	float GetHudOccludedFraction() const { return HudOccludedFraction; }

	// --- hero ------------------------------------------------------------
	void SetHeroAlive(bool bInAlive) { bHeroAlive = bInAlive; }
	bool IsHeroAlive() const { return bHeroAlive; }

	/**
	 * The hero's blow lands this frame. Published by the pawn's own swing clock,
	 * read by the combat pass — so the player's hits land on the same discrete
	 * cadence as everyone else's rather than bleeding continuously. Without this the
	 * one attack the player actually feels would be the one with no impact.
	 */
	void SetHeroStriking(bool bInStriking) { bHeroStriking = bInStriking; }
	bool IsHeroStriking() const { return bHeroStriking; }

	float GetHeroHP() const { return HeroHP; }
	float GetHeroMaxHP() const { return SwarmCombatTuning::HeroMaxHP(); }
	void SetHeroHP(float InHP) { HeroHP = FMath::Clamp(InHP, 0.f, SwarmCombatTuning::HeroMaxHP()); }

	/** Written by the combat pass, consumed + cleared by the hero pawn each tick. */
	void AddPendingHeroDamage(float Damage)
	{
		PendingHeroDamage += Damage;
		TotalHeroDamage += Damage;
	}
	float ConsumePendingHeroDamage()
	{
		const float Damage = PendingHeroDamage;
		PendingHeroDamage = 0.f;
		return Damage;
	}

	// --- balance telemetry ------------------------------------------------
	// Monotonic run totals, never consumed. The telemetry subsystem samples them
	// by differencing, so any number of readers can watch without racing each
	// other over a consume-and-clear. Cost is a handful of adds per frame.

	/** Combat pass: HP actually removed this frame, bucketed by victim team. */
	void AddDamageDealt(double ToRetinue, double ToBrood)
	{
		TotalDamageToRetinue += ToRetinue;
		TotalDamageToBrood += ToBrood;
	}
	double GetTotalDamageToRetinue() const { return TotalDamageToRetinue; }
	double GetTotalDamageToBrood() const { return TotalDamageToBrood; }
	double GetTotalHeroDamage() const { return TotalHeroDamage; }

	/** Death pass: entities destroyed this frame, bucketed by team. */
	void AddKills(int32 Retinue, int32 Brood)
	{
		TotalKilledRetinue += Retinue;
		TotalKilledBrood += Brood;
	}
	int64 GetTotalKilledRetinue() const { return TotalKilledRetinue; }
	int64 GetTotalKilledBrood() const { return TotalKilledBrood; }

	// --- squad kill attribution (docs/ui/end-of-wave-showcase.md §5.2/§5.3) ------------
	// What the end-of-wave board ranks. Credited by the COMBAT pass, not the death pass:
	// by the time USwarmDeathProcessor sees HP <= 0 the attackers have been summed into
	// one Damage number and their identities discarded (see FGridEntry::SquadId).
	//
	// Exactly ONE squad is credited per kill — the first to claim a blow on that victim
	// this frame, by iteration order. Same arbitrary, already-documented tie-break
	// FGridEntry::BlowsClaimed uses ("whichever chunk/entity is iterated first this
	// frame... not a meaningful unfairness over the course of a run"). Proportional-damage
	// credit was considered and rejected by §5.3: it needs a second bookkeeping pass this
	// model doesn't otherwise carry.
	//
	// NO per-TYPE storage on purpose: the board's "Types" view is a read-time fold over
	// squads sharing GetSquadType(i). A per-type counter would be a second source of truth
	// for the same number.
	//
	// Batched, not one call per kill, for the same reason DamageToRetinue/HeroDamage are
	// local accumulators in USwarmCombatProcessor::Execute — the credit site is inside the
	// per-entity chunk loop, and keeping the write chunk-local is what lets that loop
	// survive a future move to ParallelForEachEntityChunk.
	void CreditKills(const int32* KilledBySquad, int32 HeroKilled)
	{
		for (int32 i = 0; i < MaxSquads; ++i)
		{
			WaveKilledBySquad[i] += KilledBySquad[i];
			RunKilledBySquad[i] += KilledBySquad[i];
		}
		HeroWaveKills += HeroKilled;
		HeroRunKills += HeroKilled;
	}

	int32 GetWaveKilledBySquad(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? WaveKilledBySquad[Index] : 0;
	}
	int32 GetRunKilledBySquad(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? RunKilledBySquad[Index] : 0;
	}
	int32 GetHeroWaveKills() const { return HeroWaveKills; }
	int32 GetHeroRunKills() const { return HeroRunKills; }

	/** Zero the per-WAVE accumulators only; the run totals keep climbing. Called from
	 *  ASpike1GameMode::BeginWave. The run side resets in ResetRunState. */
	void ResetWaveKills()
	{
		for (int32& K : WaveKilledBySquad) { K = 0; }
		HeroWaveKills = 0;
	}

	// --- stance ------------------------------------------------------------
	// docs/design/squad-group-system.md §3: an order now targets an ADDRESS — "all units"
	// (default) or one named unit. GetStance/SetStance/GetStanceAnchor keep their EXACT
	// existing meaning and every existing call site (SpikeHeroPawn's four stance hotkeys,
	// KindledHud, SwarmTelemetry, UnitCamProjector's Army View tint) is untouched — this
	// is the main way this task could regress today's only behavior, so nothing about the
	// "all units" path changed: SetStance still records the single last-commanded whole-
	// retinue order AND (new) fans it out to every per-unit slot below, so a soldier's own
	// per-unit lookup reads the identical value it would have read from the old single
	// global Stance/StanceAnchor when no unit has ever been individually addressed.
	ESwarmStance GetStance() const { return Stance; }

	/**
	 * Hold anchors where the retinue stood when the order was issued; Charge
	 * aims at a world point. Both are captured at issue time so the order is a
	 * one-shot intent, not a thing that drags behind the hero.
	 */
	void SetStance(ESwarmStance InStance, const FVector& WorldPoint)
	{
		Stance = InStance;
		StanceAnchor = WorldPoint;
		// "All" writes every PLAYER-COMMANDED slot. It used to write every slot, both types;
		// since the pivot (D1) handle 7 holds the autonomous garrison and the whole point of
		// it is that the player never orders it — a broadcast order that swept the war along
		// with the seven would make the line follow the bearer around, which is the exact
		// opposite of "a front that is already there when you arrive".
		for (int32 i = 0; i < NamedSoldiers; ++i)
		{
			UnitStance[i] = InStance;
			UnitStanceAnchor[i] = WorldPoint;
		}
	}
	FVector GetStanceAnchor() const { return StanceAnchor; }

	/**
	 * Address ONE unit (0..MaxSquads-1) with an order, leaving every other unit's stance —
	 * and the global "all units" Stance/StanceAnchor above — untouched. New in this task;
	 * the real input surface (muster-card click / hotkey) is owned elsewhere (per
	 * UnitCamDirector.cpp's own precedent for CVar-as-placeholder-surface) — Swarm.
	 * UnitStance in SwarmCommands.cpp is the stand-in console command until that lands.
	 */
	void SetUnitStance(int32 UnitIndex, ESwarmStance InStance, const FVector& WorldPoint)
	{
		if (UnitIndex >= 0 && UnitIndex < MaxSquads)
		{
			UnitStance[UnitIndex] = InStance;
			UnitStanceAnchor[UnitIndex] = WorldPoint;
		}
	}
	ESwarmStance GetUnitStance(int32 UnitIndex) const
	{
		return (UnitIndex >= 0 && UnitIndex < MaxSquads) ? UnitStance[UnitIndex] : Stance;
	}
	FVector GetUnitStanceAnchor(int32 UnitIndex) const
	{
		return (UnitIndex >= 0 && UnitIndex < MaxSquads) ? UnitStanceAnchor[UnitIndex] : StanceAnchor;
	}

	// --- the marked boss (docs/design/entity-tiers.md §5) -----------------
	/**
	 * The boss's shared state, published by ASpikeBossActor's tick and read by the sim.
	 *
	 * This is the HERO BRIDGE, mirrored: entity-tiers.md §5 says in as many words to reuse
	 * `HeroMeleeRangeSq` / `FindOwnGridEntry` / `SwarmCombatProcessors.cpp` rather than invent
	 * a second Actor-vs-Mass path, and that is exactly what this is — an Actor that is not a
	 * Mass entity, keeping its authoritative state on the subsystem so both sides of the
	 * exchange can reach it without anyone reaching across entities.
	 *
	 * ONE difference from the hero, and it buys a great deal: the boss is also PUBLISHED INTO
	 * THE GRID each frame as an ordinary enemy entry (USwarmGridBuildProcessor). That single
	 * line hands it, free and with no new code, everything the grid already does — the retinue
	 * find it as their nearest enemy and close on it, their swing clocks advance against it,
	 * they pull its blows victim-side through the same BlowsClaimed budget that caps every
	 * other striker, they take knockback and flash from it, and they turn to face it. The
	 * hero's own bridge could never have that, because the hero has no enemies in the grid to
	 * be found by. Damage IN to the boss is the only half that still needs a hand-written
	 * claim, and that half is a near-verbatim copy of the brood-vs-hero branch.
	 */
	struct FBossState
	{
		FVector Location = FVector::ZeroVector;
		float HP = 0.f;
		float MaxHP = 0.f;
		uint8 Marks = 0;			// EBossMark bitmask
		bool bAlive = false;
		bool bStriking = false;		// its blow lands this frame — same one-frame flag the hero publishes
		float BlowDamage = 0.f;		// DPS * SwingInterval, already scaled by whatever marks apply
		float ReachSq = 0.f;		// MeleeRange squared; anyone inside it is a candidate for the blow
		int32 TargetsPerHit = 6;	// how many victims one blow pays out to, via FGridEntry::BlowsClaimed
	};

	const FBossState& GetBoss() const { return Boss; }
	FBossState& GetMutableBoss() { return Boss; }
	bool IsBossAlive() const { return Boss.bAlive; }

	/** Written by the combat pass, consumed + cleared by the boss actor each tick — the exact
	 *  shape AddPendingHeroDamage/ConsumePendingHeroDamage already has, other side of the board. */
	void AddPendingBossDamage(float Damage) { PendingBossDamage += Damage; }
	float ConsumePendingBossDamage()
	{
		const float Damage = PendingBossDamage;
		PendingBossDamage = 0.f;
		return Damage;
	}

	/**
	 * How many distinct soldiers landed a blow on the boss — the readout that says whether
	 * entity-tiers.md §4's surround cap is actually biting rather than being assumed.
	 *
	 * A PEAK, not the instantaneous count, and that distinction is the whole value of the
	 * number: blows are spread over each unit's own ~0.9s swing cadence, so on any given
	 * frame almost nobody is mid-strike and the per-frame count reads 0 while forty soldiers
	 * are visibly hacking at the thing. Reporting that would say the cap never binds for the
	 * wrong reason. The peak is consumed (and reset) by whoever reports it, so each report
	 * covers the window since the last one.
	 */
	void SetBossAttackers(int32 Count)
	{
		BossAttackers = Count;
		BossAttackersPeak = FMath::Max(BossAttackersPeak, Count);
	}
	int32 GetBossAttackers() const { return BossAttackers; }
	int32 ConsumeBossAttackersPeak()
	{
		const int32 Peak = BossAttackersPeak;
		BossAttackersPeak = 0;
		return Peak;
	}

	// --- the squad-channelled ability kit (task-144, docs/design/ability-kit.md) -----------
	/**
	 * Every live effect of the four verbs, in one struct, held by absolute DEADLINES rather
	 * than by countdown timers.
	 *
	 * Deadlines because the subsystem has no tick of its own — the same reason CastFocusEndTime
	 * above is an absolute world-second stamp. A reader (a Mass processor, the HUD, a console
	 * report) compares against `World->GetTimeSeconds()` and needs nothing to have decremented
	 * anything for it first, so there is no ordering question between the pass that would tick
	 * a timer and the passes that read it.
	 *
	 * WHAT IS DELIBERATELY *NOT* HERE: which SHAPE of Q23 is live. That is
	 * Kindled.Ability.Mode, a CVar, read fresh at every cast — so the mode can be flipped
	 * mid-fight and the effects already standing keep behaving exactly as the shape that cast
	 * them intended. Baking the mode into this struct would make a flip retroactive, which is
	 * the one thing that would spoil a back-to-back comparison.
	 *
	 * NOTHING HERE CLOSES Q23 OR Q26. The two addressings and the three order schemes are all
	 * reachable against the same state; picking one is an owner call.
	 */
	struct FAbilityState
	{
		/**
		 * Mark Quarry. FocusUnits is a BIT PER NAMED SOLDIER, and that bitmask is the whole
		 * mechanical difference between the two shapes of Q23:
		 *   Q23 = A marks with whoever happened to be inside Kindled.Ability.PlayerRange;
		 *   Q23 = B marks with the pack, per CLASSES.md's "the entire pack focus-fires it".
		 * Which means the same verb, cast the two ways, produces a visibly different number of
		 * soldiers turning onto the boss — the comparison, on screen, without a HUD readout.
		 */
		float FocusUntil = 0.f;
		uint8 FocusUnits = 0;

		/** Ward Circle. Centred on the cursor under Q23 = A (the bearer's own spell) and on the
		 *  casting soldier under Q23 = B (the guardian inscribes it where they stand) — the
		 *  second real difference between the shapes, and the one that makes a soldier's
		 *  POSITION load-bearing under B and irrelevant under A. */
		FVector ScreenCentre = FVector::ZeroVector;
		float ScreenUntil = 0.f;

		/** Kindle. One soldier at a time — a second cast retargets rather than stacking, which
		 *  is what "channel onto a unit" means and also stops a mode flip leaving two channels
		 *  running that nothing would ever clear. */
		int32 RaiseUnit = INDEX_NONE;
		float RaiseUntil = 0.f;

		/** Banner Slam. Same centring rule as the Ward Circle above. */
		FVector RallyCentre = FVector::ZeroVector;
		float RallyUntil = 0.f;

		/**
		 * BOTH cooldown sets are kept live at once, on purpose: per-verb is what Q23 = A means
		 * (four clocks on the bearer) and per-soldier is what Q23 = B means (seven clocks, one
		 * each), and a session that flips between them mid-fight must never read the other
		 * shape's clock and refuse a cast the live shape says is ready.
		 */
		float SoldierReadyAt[NamedSoldiers] = {};
		float VerbReadyAt[NumSquadVerbs] = {};

		/**
		 * EVIDENCE, NOT MECHANISM. How many casts each order scheme actually delivered this
		 * run, and how many it refused — indexed by SquadAbilities::EOrderScheme, whose last
		 * slot is the console rather than a scheme. Nothing reads these to decide anything;
		 * Kindled.Ability.Report prints them, because "which scheme did your hand actually
		 * reach for once both were on the same keyboard" is a question a screenshot cannot
		 * answer and recollection answers badly.
		 *
		 * The console gets its OWN slot rather than being folded into the nearest scheme: a
		 * scripted run fires every verb from the console, and counting those as direct-target
		 * clicks would make the one number this exists to produce a lie.
		 */
		static constexpr int32 NumOrderSchemes = 4;
		int32 CastsByScheme[NumOrderSchemes] = {};
		int32 RefusalsByScheme[NumOrderSchemes] = {};
	};

	const FAbilityState& GetAbilities() const { return Abilities; }
	FAbilityState& GetMutableAbilities() { return Abilities; }

	/**
	 * The Ward Circle's multiplier on damage taken at P right now — 1 when no circle stands.
	 *
	 * For ACTOR call sites only (the bearer, and the boss's blow against him): two of them, off
	 * the hot path. A Mass pass must snapshot ScreenCentre/ScreenUntil once and test inline
	 * instead of calling this per body — see USwarmCombatProcessor::Execute for that idiom.
	 */
	float ScreenScaleAt(const FVector& P, float Now) const
	{
		if (Now >= Abilities.ScreenUntil)
		{
			return 1.f;
		}
		const float R = SwarmCombatTuning::AbilityScreenRadius();
		return FVector::DistSquared2D(P, Abilities.ScreenCentre) <= R * R
			? SwarmCombatTuning::AbilityScreenScale()
			: 1.f;
	}

	/** Banner Slam: is P inside a standing banner? Same call-site rule as ScreenScaleAt. */
	bool IsRalliedAt(const FVector& P, float Now) const
	{
		if (Now >= Abilities.RallyUntil)
		{
			return false;
		}
		const float R = SwarmCombatTuning::AbilityRallyRadius();
		return FVector::DistSquared2D(P, Abilities.RallyCentre) <= R * R;
	}

	/** Which of the seven the player currently has selected — Q26 = B's first half. INDEX_NONE
	 *  until something selects one. Purely an input-layer cursor; no sim pass reads it. */
	int32 GetSelectedSoldier() const { return SelectedSoldier; }
	void SetSelectedSoldier(int32 Index)
	{
		SelectedSoldier = IsNamedUnit(Index) ? Index : INDEX_NONE;
	}

	// --- spatial grid (rebuilt by USwarmGridBuildProcessor) ---------------
	struct FGridEntry
	{
		FVector Location;
		bool bRetinue = false;

		/**
		 * This entry's blow lands THIS frame. Published by the grid build pass at the
		 * top of the frame, read by the combat pass a few groups later.
		 *
		 * This one bool is what lets discrete blows exist without breaking the
		 * victim-pull rule: a victim can tell which of its neighbours are actually
		 * connecting right now, so it still applies its own damage. Nothing ever
		 * reaches across entities to write.
		 */
		bool bStriking = false;

		/**
		 * Squared distance to this unit's Kth-nearest enemy, where K is
		 * Swarm.TargetsPerHit. A victim within this radius is a CANDIDATE for this
		 * entry's blow — necessary, but (fixed 2026-07-26) no longer sufficient by
		 * itself. See BlowsClaimed for why and how the real cap is enforced.
		 *
		 * This is a radius computed from LAST frame's neighbour set, published for
		 * THIS frame's victims to test against. Positions move between the two
		 * (steering, separation, knockback), so the number of things that fall inside
		 * a fixed radius one frame later is not reliably K — it can be more (several
		 * victims jostle into the same stale circle) or fewer (the true K nearest
		 * scattered back out). The radius alone therefore does NOT conserve damage;
		 * BlowsClaimed is what actually makes "a striker delivers exactly K blows"
		 * true regardless of how the frame's positions moved.
		 */
		float StrikeReachSq = 0.f;

		/**
		 * The other half of the cap StrikeReachSq alone can't provide: how many blows
		 * this entry's owner is allowed to hand out this swing (its own K —
		 * Swarm.RetinueTargetsPerHit or Swarm.BroodTargetsPerHit, snapshotted at grid
		 * build time), and how many it has actually handed out so far this frame.
		 *
		 * BlowsClaimed is `mutable` so a victim can increment it through the `const
		 * FGridEntry&` QueryNeighbors hands out — the entry itself is still logically
		 * read-only to everyone except this one shared counter. Every successful claim
		 * anywhere this frame (the general neighbour-loop path in
		 * USwarmCombatProcessor, and the hero's own direct check via
		 * FindOwnGridEntry) increments the SAME counter on the SAME entry, so once
		 * BlowsClaimed reaches TargetsPerHit no further victim — no matter how many
		 * still sit inside the stale StrikeReachSq radius — can claim this swing.
		 *
		 * Safe without synchronisation: the whole combat pass is one sequential
		 * ForEachEntityChunk (see USwarmCombatProcessor::Execute), so victims are
		 * visited one at a time, never in parallel — an ordinary read-modify-write,
		 * not a race. Which victim wins a contested last blow is arbitrary (whichever
		 * chunk/entity is iterated first this frame), not a meaningful unfairness over
		 * the course of a run.
		 */
		int32 TargetsPerHit = 0;
		mutable int32 BlowsClaimed = 0;

		/**
		 * How much ONE blow from this entry deals, snapshotted at grid-build time from its
		 * own team/type + DPS (SwarmCombatTuning). Replaces the old "one shared blow value
		 * per TEAM" assumption (Strikers-count * flat TeamBlow) now that retinue strikers
		 * aren't all one type — a Spearman and an Archer striking the same victim this frame
		 * must not deal the same damage. Read by every victim that claims this entry's blow,
		 * same as StrikeReachSq/TargetsPerHit above.
		 */
		float BlowDamage = 0.f;

		/**
		 * The SwarmSquad-packed byte (unit index + type) of whoever owns this entry, so a
		 * victim that dies this frame can credit the kill to a real unit — docs/ui/
		 * end-of-wave-showcase.md §5.3. Meaningless for brood (they carry no unit), read
		 * only on the retinue side of the claim.
		 *
		 * This is the ONLY place attacker identity survives the combat pass:
		 * USwarmDeathProcessor sees HP <= 0 long after every contributing attacker's
		 * Damage was summed into one number and who they were was thrown away.
		 *
		 * Cheap on purpose: FGridEntry is rebuilt from scratch every frame, so a wider
		 * transient struct costs a memcpy, not a class-layout change on a hot persistent
		 * fragment.
		 */
		uint8 SquadId = 0;
	};

	void ResetGrid(int32 ExpectedCount)
	{
		Grid.Reset();
		Grid.Reserve(ExpectedCount / 4 + 16);
	}

	void AddToGrid(const FVector& Location, bool bRetinue, bool bStriking = false,
		float StrikeReachSq = 0.f, int32 TargetsPerHit = 0, float BlowDamage = 0.f, uint8 SquadId = 0)
	{
		const FIntPoint Cell = ToCell(Location);
		Grid.FindOrAdd(Cell).Add(FGridEntry{ Location, bRetinue, bStriking, StrikeReachSq, TargetsPerHit, 0, BlowDamage, SquadId });
	}

	static FIntPoint ToCell(const FVector& Location)
	{
		return FIntPoint(
			FMath::FloorToInt(Location.X / GridCellSize),
			FMath::FloorToInt(Location.Y / GridCellSize));
	}

	int32 GetGridCellCount() const { return Grid.Num(); }

	/**
	 * Find an entity's own just-published grid entry by value match, so a brood's
	 * direct hero-strike check (SwarmCombatProcessors.cpp) can spend from the SAME
	 * BlowsClaimed budget that retinue victims draw from via QueryNeighbors, instead
	 * of bypassing the cap entirely. Matches on Location + bRetinue + bStriking +
	 * StrikeReachSq, all copied verbatim (no arithmetic) from the same fragment read
	 * the caller already did, so exact float equality is reliable here.
	 *
	 * Only called for the bounded set of brood inside Swarm.HeroMeleeRange — not the
	 * general per-pair melee hot path — so the bucket scan this does is cheap in
	 * practice. A same-frame, same-cell, exact-position tie between two entities in
	 * identical state would misattribute the claim to whichever is found first; the
	 * existing Swarm.MaxAttackersPerUnit safety cap still bounds how bad that could
	 * ever get, and two entities occupying the identical FVector is not a state combat
	 * positioning produces.
	 */
	FGridEntry* FindOwnGridEntry(const FVector& Location, bool bRetinue, bool bStriking, float StrikeReachSq)
	{
		if (TArray<FGridEntry>* Bucket = Grid.Find(ToCell(Location)))
		{
			for (FGridEntry& Entry : *Bucket)
			{
				if (Entry.bRetinue == bRetinue && Entry.bStriking == bStriking
					&& Entry.StrikeReachSq == StrikeReachSq && Entry.Location == Location)
				{
					return &Entry;
				}
			}
		}
		return nullptr;
	}

	/** Visit entries in the 3x3 cells around Location. */
	template <typename TFunc>
	void QueryNeighbors(const FVector& Location, TFunc&& Func) const
	{
		const FIntPoint Center = ToCell(Location);
		for (int32 dY = -1; dY <= 1; ++dY)
		{
			for (int32 dX = -1; dX <= 1; ++dX)
			{
				if (const TArray<FGridEntry>* Bucket = Grid.Find(Center + FIntPoint(dX, dY)))
				{
					// Total callback invocations per frame — the N x M number that
					// says whether a pass is slow from work or from bad bucketing.
					// Compiles out entirely when STATS=0.
					INC_DWORD_STAT_BY(STAT_SwarmNeighborVisits, Bucket->Num());

					for (const FGridEntry& Entry : *Bucket)
					{
						Func(Entry);
					}
				}
			}
		}
	}

	// --- static walls (bottleneck structures for the army test) -----------------------
	// A wall is a 2D capsule (segment + HalfWidth) no entity may stand inside. Resolved as
	// a position clamp in the integrate pass AFTER velocity moves the body: the move's
	// normal component is removed, the tangential component survives, so a unit seeking a
	// point beyond the wall slides along it toward the nearest opening — funnelling through
	// gates with no pathfinding. ponytail: position-slide only, no steering lookahead; add
	// a steering-side avoidance push if piles form dead-centre against a long wall.
	struct FSwarmWall
	{
		FVector2D A = FVector2D::ZeroVector;
		FVector2D B = FVector2D::ZeroVector;
		float HalfWidth = 80.f;	// wall half-thickness plus body radius, uu
	};

	void AddWall(const FVector2D& A, const FVector2D& B, float HalfWidth)
	{
		Walls.Add(FSwarmWall{ A, B, FMath::Max(HalfWidth, 1.f) });
	}
	void ClearWalls() { Walls.Reset(); }
	const TArray<FSwarmWall>& GetWalls() const { return Walls; }

	/** Push a ground-plane position out of every wall capsule. O(walls) — the integrate
	 *  pass skips the call entirely while no walls exist, so the shipped game pays nothing. */
	FVector ResolveWalls(const FVector& Pos) const
	{
		FVector Out = Pos;
		for (const FSwarmWall& W : Walls)
		{
			const FVector2D P(Out.X, Out.Y);
			const FVector2D AB = W.B - W.A;
			const float LenSq = AB.SizeSquared();
			const float T = LenSq > 0.f
				? FMath::Clamp(FVector2D::DotProduct(P - W.A, AB) / LenSq, 0.f, 1.f) : 0.f;
			const FVector2D Closest = W.A + AB * T;
			FVector2D Away = P - Closest;
			const float Dist = Away.Size();
			if (Dist < W.HalfWidth)
			{
				Away = (Dist > KINDA_SMALL_NUMBER) ? Away / Dist : FVector2D(1.f, 0.f);
				Out.X = Closest.X + Away.X * W.HalfWidth;
				Out.Y = Closest.Y + Away.Y * W.HalfWidth;
			}
		}
		return Out;
	}

	// --- render buffers (written by integrate, read by render bridge) -----
	void ResetRenderBuffers(int32 ExpectedCount)
	{
		RenderPositions.Reset(ExpectedCount);
		RenderAnimBits.Reset(ExpectedCount);
		RetinueBounds.Init();
		AliveRetinue = 0;
		AliveBrood = 0;
		LeashBroken = 0;
		for (int32& S : SquadStanding) { S = 0; }
		for (FVector& C : SquadCentroidSum) { C = FVector::ZeroVector; }
		for (float& H : SquadHP) { H = 0.f; }
		for (float& H : SquadMaxHP) { H = 0.f; }
		for (int32& P : AlivePoolByType) { P = 0; }
	}

	/**
	 * SizeBucket is a raw 0-15 per-entity roll, packed into the spare high bits of the
	 * anim int32 (SwarmRenderPack). The renderers turn it into a size multiplier; the
	 * amplitude is theirs, not ours.
	 *
	 * FacingIndex is the same deal one field over: a raw 0-31 WORLD facing step, not a
	 * sheet column. Which column that becomes depends on the yaw of whoever is looking
	 * (main camera vs. Unit Cam), so the conversion belongs to each renderer and the
	 * sim stays ignorant of the sheet — see SwarmFacing.
	 *
	 * VariantIndex is the same story a third time: which atlas look this body wears —
	 * SwarmSheet::Enemy (0-8) or SwarmSheet::Team (0-10) depending on TeamBit, since
	 * task-085 split the one variant table into a team side and an enemy side sharing
	 * this same field. Chosen by the caller from a live display-weight table, so this
	 * buffer carries a resolved index and knows nothing about the weights.
	 *
	 * SquadId is now the SwarmSquad-packed byte (unit index + type, see SwarmFragments.h).
	 * It rides into the render int32 too (SwarmRenderPack::Squad) — the piece
	 * docs/design/squad-group-system.md §1.3 and UnitCamDirector.cpp's own comment named
	 * as missing: a render-buffer consumer (the Unit Cam) can now decode a body's real
	 * unit AND type instead of approximating off the whole visible retinue.
	 */
	void PushRenderEntry(const FVector& Location, uint8 AnimBits, uint8 SquadId = 0, int32 SizeBucket = 0,
		int32 FacingIndex = 0, int32 VariantIndex = 0, float HP = 0.f, float MaxHP = 0.f)
	{
		RenderPositions.Add(Location);
		RenderAnimBits.Add(SwarmRenderPack::Pack(AnimBits, SizeBucket, FacingIndex, SquadId, VariantIndex));
		if ((AnimBits & SwarmAnim::TeamBit) != 0)
		{
			++AliveRetinue;
			RetinueBounds += Location;
			const int32 UnitIndex = FMath::Min<int32>(SwarmSquad::UnitIndex(SquadId), MaxSquads - 1);
			SquadStanding[UnitIndex]++;
			SquadCentroidSum[UnitIndex] += Location;
			// Summed, not averaged, on the SAME pass that already receives the body — the
			// third O(N) walk SquadCentroidSum refused to write. For the seven a sum IS the
			// number (one body per handle); for the garrison it is the unit's pooled HP,
			// which is the honest reading of "how much line is left" anyway.
			SquadHP[UnitIndex] += HP;
			SquadMaxHP[UnitIndex] += MaxHP;
			AlivePoolByType[(int32)SwarmSquad::UnitType(SquadId)]++;
		}
		else
		{
			++AliveBrood;
		}
	}

	const TArray<FVector>& GetRenderPositions() const { return RenderPositions; }
	const TArray<int32>& GetRenderAnimBits() const { return RenderAnimBits; }

	/** Live count of standing retinue in squad Index (0..MaxSquads-1). Valid after integrate. */
	int32 GetSquadStanding(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? SquadStanding[Index] : 0;
	}

	/**
	 * Real per-unit centroid (docs/design/squad-group-system.md §1.2's SquadCentroidSum),
	 * accumulated on the SAME PushRenderEntry pass that already receives Location and
	 * SquadId per unit — no second O(N) walk. Zero vector for an empty/unclaimed unit;
	 * callers already gate on GetSquadStanding(Index) > 0 (BuildArmyView, UnitCamDirector).
	 * Valid after integrate, same lifetime as GetSquadStanding.
	 */
	FVector GetSquadCentroid(int32 Index) const
	{
		if (Index < 0 || Index >= MaxSquads || SquadStanding[Index] <= 0) { return FVector::ZeroVector; }
		return SquadCentroidSum[Index] / (float)SquadStanding[Index];
	}

	/** Live HP / MaxHP pooled over unit Index's standing bodies. Valid after integrate, same
	 *  lifetime as GetSquadStanding. For one of the seven this is that soldier's own bar. */
	float GetSquadHP(int32 Index) const { return (Index >= 0 && Index < MaxSquads) ? SquadHP[Index] : 0.f; }
	float GetSquadMaxHP(int32 Index) const { return (Index >= 0 && Index < MaxSquads) ? SquadMaxHP[Index] : 0.f; }

	/**
	 * Open a handle directly, bypassing AssignRecruit's fill-lowest-first policy.
	 *
	 * AssignRecruit exists to grow units out of an anonymous recruit stream; the seven are
	 * the opposite — an authored roster where WHICH handle a soldier lands on is the whole
	 * identity — and the garrison is the other opposite, a hundred bodies that must all
	 * share one handle. Neither is a policy change to AssignRecruit; both simply do not go
	 * through it. Sticky in the same way: type is set once and never rewritten.
	 */
	void ClaimSquad(int32 UnitIndex, EUnitType Type)
	{
		if (UnitIndex >= 0 && UnitIndex < MaxSquads && !SquadClaimed[UnitIndex])
		{
			SquadClaimed[UnitIndex] = true;
			SquadType[UnitIndex] = Type;
		}
	}

	/** Which type unit Index (0..MaxSquads-1) is — valid only once AssignRecruit has claimed
	 *  it (IsSquadClaimed). Sticky: set once by AssignRecruit, never rewritten. */
	EUnitType GetSquadType(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? SquadType[Index] : EUnitType::Spearmen;
	}
	bool IsSquadClaimed(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) && SquadClaimed[Index];
	}

	// --- Adaptation (docs/design/adaptation.md) ---------------------------------------
	/**
	 * A unit's assigned RUNG. adaptation.md §2: a rung is the triple
	 * (unit_type, tier, variant_index) — unit_type is already SquadType[] above, so what
	 * has to land here is the other two.
	 *
	 * Per UNIT, not per soldier, and that is the design rather than a shortcut: §6 rules
	 * that a command handle is a BRANCH and a rung is a look-and-stat move inside one, so
	 * a whole unit adapts together. Eight ints instead of thirty thousand, no fragment
	 * grows a field, and the formation's existing group-by-look repack turns an adapted
	 * unit into one solid detachment for free.
	 *
	 * Variant is a WITHIN-BLOCK index (0-10 spearmen, 0-12 archers), the space
	 * SwarmRenderPack::VariantFromPhase returns; the flat 0-23 atlas index the ladder data
	 * speaks is converted at the Kindled.Adapt boundary, which is also where it is checked
	 * against the unit's own type. Tier indexes Swarm.TierHP / Swarm.TierDPS, transcribed
	 * from upgrades.json tier_ladder — THE stat spine, so no second HP/DPS ladder exists
	 * (adaptation.md §2). -1 in either means "not adapted": today's phase roll and today's
	 * knight-sub-type stats, unchanged, which is what every un-adapted unit still gets.
	 *
	 * NOT persisted across a run reset — ResetRunState clears both. Persistence is army
	 * SIZE (D3/D4, automatic on kills); this is army SHAPE and has no save format yet.
	 */
	void SetSquadRung(int32 UnitIndex, int32 WithinBlockVariant, int32 TierIndex)
	{
		if (UnitIndex < 0 || UnitIndex >= MaxSquads) { return; }
		SquadVariant[UnitIndex] = WithinBlockVariant;
		SquadTier[UnitIndex] = TierIndex;
		++AdaptationRevision;
	}
	void ClearSquadRung(int32 UnitIndex)
	{
		SetSquadRung(UnitIndex, INDEX_NONE, INDEX_NONE);
	}
	int32 GetSquadVariant(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? SquadVariant[Index] : INDEX_NONE;
	}
	int32 GetSquadTier(int32 Index) const
	{
		return (Index >= 0 && Index < MaxSquads) ? SquadTier[Index] : INDEX_NONE;
	}

	/**
	 * The whole assignment table, for a processor to snapshot ONCE PER PASS and capture
	 * into its per-chunk lambda — same idiom SwarmProcessors.cpp's FVariantTable and
	 * SwarmCombat.h's FKnightSubtypeTables already use, and the reason a per-entity
	 * subsystem call never appears in a hot loop. Stable for the world's lifetime.
	 */
	const int32* GetSquadVariants() const { return SquadVariant; }
	const int32* GetSquadTiers() const { return SquadTier; }

	/**
	 * Bumped on every rung change. The formation repack groups soldiers BY LOOK, so
	 * re-skinning a standing unit has to re-rank it the same frame or the block keeps
	 * yesterday's detachments — exactly the case URetinueFormationProcessor's
	 * `bLooksChanged` already handles for the weights CVars. A counter rather than a dirty
	 * flag so the processor can hold its own last-seen value and no consumer has to
	 * remember to clear it for the others.
	 */
	int32 GetAdaptationRevision() const { return AdaptationRevision; }

	/**
	 * World AABB over every standing retinue body, accumulated on the SAME PushRenderEntry
	 * pass that already receives Location and the team bit — the third O(N) walk this would
	 * otherwise need is exactly what SquadCentroidSum refused to write. Invalid (IsValid 0)
	 * while nothing friendly is standing, so callers must check before reading extents.
	 * Brood is deliberately NOT in it: the strategic camera contains YOUR army and lets the
	 * tide run off every edge, because a frame wide enough to hold the horde makes a soldier
	 * a smudge. Valid after integrate, same lifetime as GetSquadStanding.
	 */
	const FBox& GetRetinueBounds() const { return RetinueBounds; }

	// --- live counts (valid from the end of the integrate pass) -----------
	int32 GetAliveRetinue() const { return AliveRetinue; }
	int32 GetAliveBrood() const { return AliveBrood; }
	int32 GetLeashBrokenCount() const { return LeashBroken; }
	void AddLeashBroken(int32 Count) { LeashBroken += Count; }

	/** Live standing pool of one type, valid after integrate (§4.1's Pool(type)). */
	int32 GetAliveByType(EUnitType Type) const { return AlivePoolByType[(int32)Type]; }

	// --- formation repack --------------------------------------------------
	// The formation slot index has to stay DENSE for the shape to mean anything: with a
	// baked index, casualties punch holes through a block and its outline never shrinks,
	// so the formation stops reporting how much army is left. Repacking is O(N log N),
	// so it is gated on the standing count actually having moved rather than run per
	// frame. AliveRetinue is last frame's count when the steering pass reads it, which is
	// exactly right — one frame of lag on re-forming a line is invisible.
	//
	// TWO independent repacks, one per TYPE, not one retinue-wide sort and not eight
	// per-unit-of-8 sorts (docs/design/squad-group-system.md §1.2, §8: "a type's units
	// still share ONE dense index space that subdivides into unit-sized chunks... scoped
	// to the type's own pool instead of the whole retinue" / "TWO independent dense
	// repacks (one per type) instead of one retinue-wide repack"). A stable type skips its
	// repack while the other type reforms — cheaper in aggregate than one big sort, and
	// this is the actual unit of "cheaper" the spec asks for, not a per-unit-of-8 grain.

	bool NeedsFormationRepack() const { return AliveRetinue != PackedRetinueCount; }
	void MarkFormationPacked() { PackedRetinueCount = AliveRetinue; }
	/** Force a repack next pass — for changes the count alone would not reveal. */
	void MarkFormationDirty() { PackedRetinueCount = -1; }

	bool NeedsFormationRepack(EUnitType Type) const
	{
		return AlivePoolByType[(int32)Type] != PackedPoolByType[(int32)Type];
	}
	void MarkFormationPacked(EUnitType Type) { PackedPoolByType[(int32)Type] = AlivePoolByType[(int32)Type]; }

	// --- recruitment (docs/design/squad-group-system.md §1.4, §4.1) -------------------
	/**
	 * Assign a newly recruited soldier's permanent (unit, type) pair. Fill-lowest-first
	 * into the least-full EXISTING unit of Type; opens a new unit — Spearmen claiming the
	 * lowest free global id, Archers the highest (§4.1's "Spearmen claim first", made
	 * concrete as a topology: the two types grow the shared 0..MaxSquads-1 budget from
	 * opposite ends instead of needing to renumber to avoid colliding in the middle) —
	 * once §4.1's derived want (ceil(Pool/TypeLegibilityCeiling)) exceeds the type's
	 * current live unit count and a global slot remains. Falls back to folding into the
	 * type's own existing units (or, if the type owns nothing yet and the whole 8-slot
	 * budget is already claimed by the other type, unit 0) once the budget is exhausted —
	 * §4.2's own honestly-flagged breaking point, not a crash.
	 *
	 * Reads last frame's SquadStanding (the same one-frame lag the formation repack
	 * already tolerates) plus a caller-supplied count of how many of this type this same
	 * spawn batch has already produced, so a big batch (e.g. StartingRetinue) opens
	 * however many new units its own recruits actually justify instead of only ever
	 * reacting to last frame's snapshot.
	 */
	uint8 AssignRecruit(EUnitType Type, int32 AlreadyRecruitedThisBatch = 0)
	{
		int32 BestUnit = INDEX_NONE;
		int32 BestStanding = TNumericLimits<int32>::Max();
		int32 TypeUnitCount = 0;
		int32 TypePool = AlreadyRecruitedThisBatch;
		for (int32 i = 0; i < MaxSquads; ++i)
		{
			if (!SquadClaimed[i] || SquadType[i] != Type) { continue; }
			++TypeUnitCount;
			TypePool += SquadStanding[i];
			if (SquadStanding[i] < BestStanding) { BestStanding = SquadStanding[i]; BestUnit = i; }
		}

		// +1: as-if this recruit already counted, so the soldier that TIPS a unit over the
		// ceiling starts the new one instead of being asked to join the one it just filled.
		const int32 WantedUnits = FMath::Max(1, (TypePool + 1 + TypeLegibilityCeiling - 1) / TypeLegibilityCeiling);
		const bool bWantNewUnit = (TypeUnitCount == 0) || (WantedUnits > TypeUnitCount);

		if (bWantNewUnit)
		{
			int32 NewId = INDEX_NONE;
			if (Type == EUnitType::Spearmen)
			{
				for (int32 i = 0; i < MaxSquads; ++i) { if (!SquadClaimed[i]) { NewId = i; break; } }
			}
			else
			{
				for (int32 i = MaxSquads - 1; i >= 0; --i) { if (!SquadClaimed[i]) { NewId = i; break; } }
			}
			if (NewId != INDEX_NONE)
			{
				SquadClaimed[NewId] = true;
				SquadType[NewId] = Type;
				return SwarmSquad::Pack((uint8)NewId, Type);
			}
			// No free global slot anywhere — §4.2's own named breaking point (the 8-handle
			// budget is fully claimed by the two types combined). Fold into this type's own
			// units below rather than crash.
		}

		if (BestUnit == INDEX_NONE)
		{
			// This type owns nothing yet AND no free slot exists (the other type claimed
			// all 8) — degrade to unit 0 rather than lose the recruit. Same §4.2 edge.
			BestUnit = 0;
			if (!SquadClaimed[0]) { SquadClaimed[0] = true; SquadType[0] = Type; }
		}
		return SwarmSquad::Pack((uint8)BestUnit, Type);
	}

	// --- bookkeeping -------------------------------------------------------
	void TrackSpawned(const TArray<FMassEntityHandle>& Handles) { AllEntities.Append(Handles); }
	TArray<FMassEntityHandle>& GetTrackedEntities() { return AllEntities; }

	void AddHeroContacts(int32 Count)
	{
		HeroContacts += Count;
		HeroContactsThisFrame = Count;	// instantaneous: how mobbed he is *right now*
	}
	int64 GetHeroContacts() const { return HeroContacts; }
	int32 GetHeroContactsThisFrame() const { return HeroContactsThisFrame; }

	/** Reset everything except tracked handles (the Clear command owns those). */
	void ResetRunState()
	{
		HeroHP = SwarmCombatTuning::HeroMaxHP();
		PendingHeroDamage = 0.f;
		bHeroAlive = true;
		bHeroStriking = false;
		Stance = ESwarmStance::Follow;
		StanceAnchor = FVector::ZeroVector;
		for (int32 i = 0; i < MaxSquads; ++i)
		{
			UnitStance[i] = ESwarmStance::Follow;
			UnitStanceAnchor[i] = FVector::ZeroVector;
			SquadClaimed[i] = false;
			SquadType[i] = EUnitType::Spearmen;
			SquadVariant[i] = INDEX_NONE;
			SquadTier[i] = INDEX_NONE;
		}
		++AdaptationRevision;
		for (int32& P : PackedPoolByType) { P = -1; }
		Boss = FBossState{};
		Abilities = FAbilityState{};
		SelectedSoldier = INDEX_NONE;
		PendingBossDamage = 0.f;
		BossAttackers = 0;
		BossAttackersPeak = 0;
		HeroContacts = 0;
		HeroContactsThisFrame = 0;
		TotalDamageToRetinue = 0.0;
		TotalDamageToBrood = 0.0;
		TotalHeroDamage = 0.0;
		TotalKilledRetinue = 0;
		TotalKilledBrood = 0;
		for (int32& K : RunKilledBySquad) { K = 0; }
		HeroRunKills = 0;
		ResetWaveKills();
	}

private:
	FVector Attractor = FVector::ZeroVector;
	FVector StanceAnchor = FVector::ZeroVector;
	ESwarmStance Stance = ESwarmStance::Follow;

	FVector CastFocusPos = FVector::ZeroVector; // Unit Cam spell-cast focus point
	double CastFocusEndTime = 0.0;              // absolute world seconds the focus lasts until

	float HudOccludedFraction = 0.f; // 0 until the HUD reports in, so no HUD = no camera bias

	float HeroHP = SwarmCombatTuning::HeroMaxHP();
	float PendingHeroDamage = 0.f;
	bool bHeroAlive = true;
	bool bHeroStriking = false;

	TMap<FIntPoint, TArray<FGridEntry>> Grid;
	TArray<FSwarmWall> Walls;	// authored by console/scenario commands, empty by default
	TArray<FVector> RenderPositions;
	TArray<int32> RenderAnimBits;
	FBox RetinueBounds = FBox(ForceInit); // friendly-only AABB, refilled each frame
	int32 SquadStanding[MaxSquads] = {}; // live retinue count per unit, refilled each frame
	FVector SquadCentroidSum[MaxSquads] = {}; // sum of member locations per unit, refilled each frame
	float SquadHP[MaxSquads] = {};       // pooled live HP per unit, refilled each frame
	float SquadMaxHP[MaxSquads] = {};    // pooled MaxHP per unit, refilled each frame
	TArray<FMassEntityHandle> AllEntities;

	FBossState Boss;
	FAbilityState Abilities;
	int32 SelectedSoldier = INDEX_NONE;
	float PendingBossDamage = 0.f;
	int32 BossAttackers = 0;
	int32 BossAttackersPeak = 0;

	// --- typed-unit command layer (docs/design/squad-group-system.md §1) --------------
	bool SquadClaimed[MaxSquads] = {};                         // has AssignRecruit ever opened this unit id
	EUnitType SquadType[MaxSquads] = {};                        // sticky — set once by AssignRecruit
	ESwarmStance UnitStance[MaxSquads] = {};                    // per-unit order; "all" writes every slot
	FVector UnitStanceAnchor[MaxSquads] = {};
	int32 AlivePoolByType[NumUnitTypes] = {};                   // live standing per type, refilled each frame
	int32 PackedPoolByType[NumUnitTypes] = { -1, -1 };          // pool as of each type's last formation repack

	// Adaptation: this unit's assigned rung, INDEX_NONE = not adapted. See SetSquadRung.
	int32 SquadVariant[MaxSquads] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE,
	                                  INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	int32 SquadTier[MaxSquads] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE,
	                               INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	int32 AdaptationRevision = 0;

	int32 AliveRetinue = 0;
	int32 AliveBrood = 0;
	int32 LeashBroken = 0;
	int32 PackedRetinueCount = -1;	// standing count as of the last formation repack
	int64 HeroContacts = 0;
	int32 HeroContactsThisFrame = 0;

	double TotalDamageToRetinue = 0.0;
	double TotalDamageToBrood = 0.0;
	double TotalHeroDamage = 0.0;
	int64 TotalKilledRetinue = 0;
	int64 TotalKilledBrood = 0;

	// Brood killed, credited per unit. Independent accumulators, NOT derived from current
	// standing — a wiped squad still shows its real kill count against a x0 suffix
	// (docs/ui/end-of-wave-showcase.md §6.2). Same for the hero: these live here, not on
	// the pawn, so a HeroDown outcome doesn't take the number with it.
	int32 WaveKilledBySquad[MaxSquads] = {};
	int32 RunKilledBySquad[MaxSquads] = {};
	int32 HeroWaveKills = 0;
	int32 HeroRunKills = 0;
};
