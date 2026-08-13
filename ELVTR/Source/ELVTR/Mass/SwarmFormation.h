#pragma once

#include "CoreMinimal.h"
#include "SwarmCombat.h" // EUnitType

/**
 * How your army arranges itself around the bearer.
 *
 * Until 2026-07-26 this was one hard-coded function: concentric rings, 110uu apart,
 * baked into each unit at spawn. That shape has a specific cost under a camera that
 * never rotates — a ring puts half your army BEHIND you, off the bottom of the frame,
 * so the thing the player is meant to read at a glance ("how much of my line is left")
 * is the one thing the ring hides. A formation that faces the camera puts the whole
 * army broadside in frame, and losing a rank is then visible as a rank.
 *
 * Two consequences follow, and both are why this is a header and not a static function:
 *
 *  1. The slot is no longer baked. Units store a slot INDEX and the offset is derived
 *     every frame, so shape, spacing and facing are live dials you can move while the
 *     battle is running instead of properties of a spawn that already happened.
 *
 *  2. The index is DENSE, not the spawn order. See Compact below — with a baked index,
 *     casualties leave holes scattered through a block and the outline never shrinks,
 *     which defeats the entire point of the change.
 */
namespace SwarmFormation
{
	enum class EShape : uint8
	{
		/** Concentric rings around the bearer. The original shape: surrounds you, hides half of itself. */
		Ring = 0,
		/** Rectangular block, Columns wide, ranks stacking away from camera. The readable one. */
		Block = 1,
		/** V with the point away from camera — wings trail back toward the bearer. */
		Wedge = 2,
		/** Shield wall: ranks bowed on an arc, curving around the far side of the bearer. */
		Arc = 3,
	};

	/** Everything the formation reads, snapshotted once per processor pass. */
	struct FParams
	{
		EShape Shape = EShape::Block;
		float Spacing = 110.f;      // lateral gap between neighbours in a rank, uu
		float RankSpacing = 110.f;  // gap between ranks (depth), uu
		int32 Columns = 8;         // slots per rank — Block and Arc
		float GroupGap = 80.f;     // Block: clear ground between per-look detachments, uu
		int32 GroupsPerRow = 4;     // Block: detachments abreast before wrapping back, 0 = never wrap
		float GroupRowPitch = 300.f;// Block: depth step between wrapped detachment rows, uu
		int32 GroupDepthCap = 2;    // Block: ranks a look fills before opening a sibling detachment
		                            // of the same look, rather than deepening forever. Soldier cap
		                            // per detachment = Columns * GroupDepthCap. Shared across types,
		                            // same reasoning as GroupGap/GroupsPerRow above.
		float Forward = 150.f;      // shift the whole formation away from camera, uu
		float ArcDegrees = 140.f;   // Arc: angle the front rank subtends
		float ArcRadius = 700.f;    // Arc: radius of the front rank, uu
		float YawRadians = 0.f;     // resolved world bearing the formation faces
		bool bCompact = true;
	};

	/**
	 * Read the console dials and resolve the facing.
	 *
	 * The facing is the part worth explaining. The camera does not rotate with the
	 * hero — it holds a fixed bearing (Kindled.Cam.Yaw), so "away from the viewer"
	 * is a constant world direction, not something derived per-unit. At the default
	 * yaw of 0 that direction is world +X, which is also what Kindled.Cam.YawInput
	 * makes 'W' mean, so the formation, the camera and the controls all agree on which
	 * way "forward" is by construction.
	 */
	FParams ReadParams();

	/**
	 * Per-type formation params (docs/design/squad-group-system.md §1.7): Spearmen ARE
	 * ReadParams() above, unchanged (same CVars, same shipped defaults — "mechanically
	 * today's retinue, renamed and formalized as a type"). Archers read an independent
	 * `Swarm.Formation.Archers.*` CVar set — a wide, shallow line sitting close behind the
	 * bearer instead of pushed out ahead, per the spec's table. Mirrors the precedent
	 * `Swarm.BroodFormation.*` already set (task-047): a formation is a live dial while the
	 * game runs, not a spawn-time constant, so two types need two independently-tunable
	 * FParams, not one shared global.
	 *
	 * Deviation from §1.7's literal mechanism, disclosed: ArcDegrees/ArcRadius/Yaw/
	 * FaceCamera/Compact stay the SHARED `Swarm.Formation.*` CVars rather than duplicated
	 * per type. unit-types.json's own formation block ships IDENTICAL arc_degrees/
	 * arc_radius (140/700) for both types, so a duplicated dial there would be two numbers
	 * that must always agree — not a real per-type difference like Shape/Columns/Spacing/
	 * RankSpacing/Forward (which §1.7's table DOES differentiate and this DOES duplicate).
	 * Bearing-tracking and the compact-shrink toggle are army-wide behaviors, not per-type
	 * ones. Cuts CVar count without dropping anything the spec's own data currently varies.
	 */
	FParams ReadParamsForType(EUnitType Type);

	/**
	 * Ground-plane offset from the anchor for dense slot Index, already rotated into
	 * world space. Pure — same index and params give the same offset on every caller,
	 * which is what lets the spawner place a unit and the steering pass agree.
	 *
	 * GroupIndex is the soldier's DETACHMENT — one per unique sprite WITHIN one command
	 * unit, ranked densely among the looks that unit currently has standing
	 * (URetinueFormationProcessor assigns it, and Index is then the position WITHIN
	 * that detachment). Each detachment is its own Columns-wide block with GroupGap of
	 * clear ground beside it, capped at Columns * GroupDepthCap soldiers deep — a look
	 * that outgrows the cap opens a sibling detachment instead of deepening forever,
	 * and a detachment never straddles two different command units (a fresh depth row
	 * always starts at a unit boundary), so the formation reads as "these are Unit 3's
	 * soldiers, in these looks" rather than one undifferentiated type-wide slab.
	 *
	 * Block shape only. The other three shapes have no lateral band to give a group,
	 * so the repack leaves every unit in group 0 for them and they behave as before —
	 * see URetinueFormationProcessor::Execute. Pass 0 when there is no grouping to
	 * apply (the spawner's frame-one placement does).
	 */
	FVector2D SlotOffset(int32 Index, int32 GroupIndex, const FParams& Params);

	/**
	 * Brood's version of SlotOffset. Same idea — a dense index resolves to a ground-plane
	 * offset, pure in the same sense — but ranks step OUTWARD from Params.ArcRadius (the
	 * front rank, which leads the wave and arrives first) instead of inward, because the
	 * brood is a tide still closing on the anchor, not a shield wall standing at rest
	 * around it. Columns fan across Params.ArcDegrees exactly as the retinue's own Arc
	 * shape does — same fields, same meanings, just read by a function that steps the
	 * other way in depth. Kept separate rather than folded into EShape as a fifth case
	 * because the retinue never wants ranks that grow away from it; sharing one switch
	 * would just be a trap for whichever shape reads it next. Params.Shape/Spacing/Forward/
	 * bCompact are unused here — see SwarmCommands.cpp for how the brood-side FParams is
	 * built (ArcDegrees/ArcRadius/YawRadians from the Swarm.BroodSpawn* CVars, Columns/
	 * RankSpacing from Swarm.BroodFormation.*).
	 */
	FVector2D BroodSlotOffset(int32 Index, const FParams& Params);

	/**
	 * Kindled.Cam.Yaw, looked up by name and cached (see ReadParams for why by-name).
	 * Exposed so the brood spawn arc (SwarmCommands.cpp, Swarm.BroodSpawnFaceCamera) can
	 * track the exact same camera reading the retinue formation does — two separate
	 * lookups of the same CVar would risk drifting apart by a frame under a live edit.
	 */
	float CameraYawDegrees();
}
