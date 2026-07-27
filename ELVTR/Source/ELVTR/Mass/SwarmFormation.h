#pragma once

#include "CoreMinimal.h"

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
		int32 Columns = 12;         // slots per rank — Block and Arc
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
	 * hero — it holds a fixed bearing (Emberkeep.Cam.Yaw), so "away from the viewer"
	 * is a constant world direction, not something derived per-unit. At the default
	 * yaw of 0 that direction is world +X, which is also what Emberkeep.Cam.YawInput
	 * makes 'W' mean, so the formation, the camera and the controls all agree on which
	 * way "forward" is by construction.
	 */
	FParams ReadParams();

	/**
	 * Ground-plane offset from the anchor for dense slot Index, already rotated into
	 * world space. Pure — same index and params give the same offset on every caller,
	 * which is what lets the spawner place a unit and the steering pass agree.
	 */
	FVector2D SlotOffset(int32 Index, const FParams& Params);
}
