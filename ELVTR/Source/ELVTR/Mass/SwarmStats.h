#pragma once

#include "CoreMinimal.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Stats/Stats.h"

/**
 * Per-pass timing for the swarm chain. Permanent, not scaffolding: perf work you
 * can't re-measure next month is perf work you'll redo.
 *
 *   stat Swarm            in-game overlay, per-pass ms + entity counters
 *   Unreal Insights       same scopes show up as CPU trace events
 *
 * Every pass in the chain has a counter, so the overlay reads as the chain:
 *   GridBuild -> BroodSteering / RetinueFollow -> Combat -> Integrate -> Death / Contact -> RenderBridge
 */
DECLARE_STATS_GROUP(TEXT("Swarm"), STATGROUP_Swarm, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Grid build"), STAT_SwarmGridBuild, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Brood steering"), STAT_SwarmBroodSteering, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Retinue follow"), STAT_SwarmRetinueFollow, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Combat"), STAT_SwarmCombat, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Integrate"), STAT_SwarmIntegrate, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Death"), STAT_SwarmDeath, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Contact"), STAT_SwarmContact, STATGROUP_Swarm, );

// Render bridge, split so "the sim is slow" and "the bridge is slow" can't be confused.
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render bridge"), STAT_SwarmRenderBridge, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render bridge - debug draw"), STAT_SwarmDebugDraw, STATGROUP_Swarm, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Render bridge - Niagara push"), STAT_SwarmNiagaraPush, STATGROUP_Swarm, );

// Counters: a ms number means nothing without the N it was measured at.
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Entities (retinue)"), STAT_SwarmAliveRetinue, STATGROUP_Swarm, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Entities (brood)"), STAT_SwarmAliveBrood, STATGROUP_Swarm, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Grid cells occupied"), STAT_SwarmGridCells, STATGROUP_Swarm, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Neighbour visits"), STAT_SwarmNeighborVisits, STATGROUP_Swarm, );

/**
 * One scope, both tools. Name must be a bare identifier (Insights event name);
 * Stat is the STAT_ id declared above.
 */
#define SWARM_SCOPE(Stat, Name) \
	SCOPE_CYCLE_COUNTER(Stat); \
	TRACE_CPUPROFILER_EVENT_SCOPE(Name)
