#include "Spike1GameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Mass/SwarmFragments.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "SpikeHeroPawn.h"
#include "UI/KindledUIDebug.h"
#include "TimerManager.h"

namespace
{
	/**
	 * Safety valve for task-064: GetAliveBrood()==0 alone has no way out of a genuine
	 * stall. MEASURED ROOT CAUSE (2026-07-29, live PIE, not guessed): a handful of brood
	 * end up standing 18-38uu from their nearest retinue — well inside Swarm.MeleeRange
	 * (95uu) — yet never take a hit. That retinue has to be an Archer: Swarm.
	 * ArchersMinEngageRange (150uu, SwarmCombatProcessors.cpp) refuses to target anything
	 * closer than that to ITSELF ("won't shoot into its own scrum"), and Archers never
	 * close distance to melee (squad-group-system.md §1.8's "hold formation... without
	 * closing distance"), so a brood that ends up inside that dead zone with no Spearman
	 * near enough to finish it off in MeleeRange just sits there. Confirmed for 135+
	 * continuous seconds at an EXACT unchanged count of 7 in one capture — not slow
	 * combat, a permanent stall. That is Mass-side steering/combat code
	 * (ELVTR/Source/ELVTR/Mass/SwarmProcessors.cpp's URetinueFollowProcessor and
	 * SwarmCombatProcessors.cpp's USwarmCombatProcessor), out of this file's ownership —
	 * see the task-064 handback for the precise fix recommendation. This CVar is the
	 * valve on top of that diagnosis, not a replacement for fixing it.
	 */
	TAutoConsoleVariable<float> CVarWaveClearTimeoutSeconds(
		TEXT("Swarm.WaveClearTimeoutSeconds"), 20.f,
		TEXT("Once the SAME nonzero brood count has held for this many seconds past\n")
		TEXT("WaveClearGraceSeconds, the wave is force-cleared: the stragglers are logged\n")
		TEXT("(position + distance to hero/nearest retinue) then destroyed outright, so they\n")
		TEXT("do not carry over and repeat the stall on every later wave too. 20s is a wide\n")
		TEXT("margin over normal tail-end combat -- measured normal wave clears finish a run\n")
		TEXT("of 5-10 remaining brood in a few seconds once the SwingInterval cadence (0.9s)\n")
		TEXT("is landing hits, so 20s of the identical count not moving at all is already a\n")
		TEXT("strong stall signal, not variance. 0 disables the timeout entirely -- the gate\n")
		TEXT("reverts to GetAliveBrood()==0 with no way out, task-064's original bug."),
		ECVF_Default);

	/**
	 * task-064 diagnostics: this state is FILE-STATIC, not GameMode members, on purpose:
	 * adding a member changes the class layout, which Live Coding cannot apply (reports
	 * success, then crashes the next PIE) — see ASpikeHeroPawn's GCamArmyScale for the
	 * same trick, same reason. Reset from RestartRun() so a stalled count from a previous
	 * PIE session in the same editor process can't be mistaken for one in the next.
	 */
	int32 GStallLastBroodCount = -1;
	float GStallUnchangedSeconds = 0.f;
	bool GStallLogged = false;

	void ResetStallTracking()
	{
		GStallLastBroodCount = -1;
		GStallUnchangedSeconds = 0.f;
		GStallLogged = false;
	}

	/**
	 * Dumps every surviving brood's position and how far it is from the two things that
	 * are supposed to kill it: the hero (Swarm.HeroMeleeRange) and its nearest retinue
	 * unit (Swarm.MeleeRange / Swarm.ArchersEngageRange). Reads only USwarmSubsystem's
	 * public render buffers (GetRenderPositions/GetRenderAnimBits) — no Mass API, so this
	 * stays inside Spike1GameMode's ownership. O(N*M) against the tiny stall-time
	 * population (a handful of brood against ~100+ retinue), which is fine for a
	 * once-per-stall diagnostic dump, not a hot path.
	 */
	void LogStalledBrood(const USwarmSubsystem& Swarm, int32 WaveIndex)
	{
		const FVector Attractor = Swarm.GetAttractor();
		const TArray<FVector>& Positions = Swarm.GetRenderPositions();
		const TArray<int32>& AnimBits = Swarm.GetRenderAnimBits();

		int32 BroodCount = 0;
		for (int32 i = 0; i < Positions.Num(); ++i)
		{
			if ((SwarmRenderPack::Anim(AnimBits[i]) & SwarmAnim::TeamBit) == 0)
			{
				++BroodCount;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Run: wave %d STALL — %d brood alive, positions follow"),
			WaveIndex + 1, BroodCount);

		for (int32 i = 0; i < Positions.Num(); ++i)
		{
			if ((SwarmRenderPack::Anim(AnimBits[i]) & SwarmAnim::TeamBit) != 0)
			{
				continue; // retinue, not what we're hunting for
			}

			const FVector& Loc = Positions[i];
			const float DistToHero = FVector::Dist2D(Loc, Attractor);

			float NearestRetinueDist = -1.f;
			for (int32 j = 0; j < Positions.Num(); ++j)
			{
				if ((SwarmRenderPack::Anim(AnimBits[j]) & SwarmAnim::TeamBit) == 0)
				{
					continue;
				}
				const float D = FVector::Dist2D(Loc, Positions[j]);
				if (NearestRetinueDist < 0.f || D < NearestRetinueDist)
				{
					NearestRetinueDist = D;
				}
			}

			UE_LOG(LogTemp, Warning,
				TEXT("Run:   straggler (%.0f, %.0f) — %.0fuu from hero, %.0fuu from nearest retinue"),
				Loc.X, Loc.Y, DistToHero, NearestRetinueDist);
		}
	}

	/**
	 * task-064 timeout fix: destroys every currently-tracked BROOD entity outright, via
	 * the same UMassEntitySubsystem / FMassEntityManager pattern SwarmSpawn::ClearAll
	 * already uses (SwarmCommands.cpp) — public Mass engine API, not a write into
	 * ELVTR/Source/ELVTR/Mass/. Retinue entities are left untouched, filtered the same
	 * "brood carry no team bit" way SwarmCommands.cpp's LogSquadRoster already does.
	 *
	 * Needed because just advancing WaveIndex without this would carry the same
	 * unkillable stragglers into the NEXT wave's brood count, which would then also never
	 * reach zero — the timeout would refire every subsequent wave forever instead of
	 * once, and the field would slowly accumulate permanently-stuck bodies nobody can see
	 * a reason for. Returns how many were actually destroyed, for the log line.
	 */
	int32 DestroyRemainingBrood(UWorld* World)
	{
		UMassEntitySubsystem* MassSubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
		USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
		if (!MassSubsystem || !Swarm)
		{
			return 0;
		}

		FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
		int32 Destroyed = 0;
		for (const FMassEntityHandle& Handle : Swarm->GetTrackedEntities())
		{
			if (!EntityManager.IsEntityValid(Handle))
			{
				continue;
			}
			const FMassEntityView View(EntityManager, Handle);
			const FSwarmAnimFragment* Anim = View.GetFragmentDataPtr<FSwarmAnimFragment>();
			if (Anim && (Anim->Bits & SwarmAnim::TeamBit) == 0)
			{
				EntityManager.DestroyEntity(Handle);
				++Destroyed;
			}
		}
		return Destroyed;
	}
}

namespace
{
	/**
	 * The end-of-wave board's data, printed the moment a wave clears (docs/ui/
	 * end-of-wave-showcase.md §5) — the only readout of it that exists until W_WaveBoard
	 * is built, and the reconciliation the attribution has to survive: every credited kill
	 * plus the hero's should account for the run's whole KilledBrood total.
	 *
	 * A nonzero delta is expected in exactly one case and is printed, not hidden: brood
	 * force-cleared by the Swarm.WaveClearTimeoutSeconds valve are destroyed outright
	 * rather than dropped to HP <= 0, so they are counted by NEITHER side — and any other
	 * gap means the credit site missed a death.
	 */
	void LogWaveKills(const USwarmSubsystem& Swarm, int32 WaveIndex)
	{
		FString Wave, Run;
		int32 WaveSum = 0;
		int32 RunSum = 0;
		for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
		{
			if (!Swarm.IsSquadClaimed(i))
			{
				continue;
			}
			const TCHAR* TypeName = Swarm.GetSquadType(i) == EUnitType::Archers ? TEXT("Archers") : TEXT("Spearmen");
			Wave += FString::Printf(TEXT(" unit%d(%s x%d)=%d"),
				i, TypeName, Swarm.GetSquadStanding(i), Swarm.GetWaveKilledBySquad(i));
			Run += FString::Printf(TEXT(" unit%d=%d"), i, Swarm.GetRunKilledBySquad(i));
			WaveSum += Swarm.GetWaveKilledBySquad(i);
			RunSum += Swarm.GetRunKilledBySquad(i);
		}

		const int64 Credited = (int64)RunSum + Swarm.GetHeroRunKills();
		UE_LOG(LogTemp, Display, TEXT("Run: wave %d board — WAVE:%s hero=%d (sum %d)"),
			WaveIndex + 1, *Wave, Swarm.GetHeroWaveKills(), WaveSum + Swarm.GetHeroWaveKills());
		UE_LOG(LogTemp, Display, TEXT("Run: wave %d board — RUN: %s hero=%d (sum %lld) vs KilledBrood=%lld, delta %lld"),
			WaveIndex + 1, *Run, Swarm.GetHeroRunKills(), Credited,
			Swarm.GetTotalKilledBrood(), Swarm.GetTotalKilledBrood() - Credited);
	}

	/**
	 * True when launched with -SwarmBench, i.e. ASwarmRenderActor's benchmark owns
	 * the entity population. Evaluated once; the command line cannot change at runtime.
	 */
	bool IsBenchmarkRun()
	{
		static const bool bBench = FParse::Param(FCommandLine::Get(), TEXT("SwarmBench"));
		return bBench;
	}
}

ASpike1GameMode::ASpike1GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ASpikeHeroPawn::StaticClass();
}

void ASpike1GameMode::BeginPlay()
{
	Super::BeginPlay();

	// Under -SwarmBench the benchmark owns the entity population: ASwarmRenderActor
	// clears and respawns an exact retinue/brood count at every step, then times it.
	// Auto-starting the wave run here lays a second, independently-scheduled
	// population on top of that, so each step times some unknown mix of the two and
	// no two runs are comparable — which is fatal for an A/B. Skip the auto-start and
	// let the harness have the field to itself.
	//
	// Manual RestartRun (R on the hero pawn) still works. Arming the benchmark via
	// the actor's bRunBenchmark property instead of the command line will still
	// collide — use -SwarmBench for measurement runs.
	if (!IsBenchmarkRun())
	{
		RestartRun();
	}

	// Show the combat HUD on play by default (toggle with `Kindled.UI.AutoShow 0`).
	// Next tick so the local player's viewport and pawn are ready.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]()
	{
		KindledUI::AutoShowIfEnabled(GetWorld());
	}));
}

void ASpike1GameMode::RestartRun()
{
	SwarmSpawn::ClearAll(GetWorld());

	if (USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->ResetRunState();

		// The formation spawns around the attractor, and the hero pawn may not
		// have ticked yet on the first run — seed it here so wave 1 isn't
		// deployed around the world origin.
		if (const APawn* Hero = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
		{
			Swarm->SetAttractor(Hero->GetActorLocation());
		}
	}

	WaveIndex = 0;
	RetinueSlotCursor = 0;
	ResetStallTracking();

	SwarmSpawn::SpawnRetinue(GetWorld(), StartingRetinue, RetinueSlotCursor);
	RetinueSlotCursor += StartingRetinue;

	EnterPhase(ERunPhase::Deploying);
	UE_LOG(LogTemp, Display, TEXT("Run: restarted (%d retinue, %d waves)"), StartingRetinue, WaveBroodCounts.Num());
}

void ASpike1GameMode::EnterPhase(ERunPhase NewPhase)
{
	Phase = NewPhase;
	PhaseTimer = 0.f;
}

void ASpike1GameMode::BeginWave()
{
	const int32 Count = WaveBroodCounts.IsValidIndex(WaveIndex) ? WaveBroodCounts[WaveIndex] : 0;

	// A wave's kill board is this wave's, not the run's — zero it here, at the start of
	// the wave whose numbers it will show. The run accumulators keep climbing and only
	// reset in USwarmSubsystem::ResetRunState (docs/ui/end-of-wave-showcase.md §5.2).
	if (USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->ResetWaveKills();
	}

	SwarmSpawn::SpawnBrood(GetWorld(), Count);
	EnterPhase(ERunPhase::WaveActive);
	UE_LOG(LogTemp, Display, TEXT("Run: wave %d/%d — %d brood (wave kill board zeroed)"),
		WaveIndex + 1, WaveBroodCounts.Num(), Count);
}

void ASpike1GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Skipping RestartRun in BeginPlay is not enough on its own: Phase starts at
	// Deploying, so this state machine would still advance into wave 1 by itself and
	// spawn brood on top of the benchmark's population. Stay out entirely.
	if (IsBenchmarkRun())
	{
		return;
	}

	USwarmSubsystem* Swarm = GetWorld()->GetSubsystem<USwarmSubsystem>();
	if (!Swarm)
	{
		return;
	}

	PhaseTimer += DeltaSeconds;

	// Hero death ends the run from any phase.
	if (!IsRunOver() && !Swarm->IsHeroAlive())
	{
		EnterPhase(ERunPhase::Lost);
		UE_LOG(LogTemp, Display, TEXT("Run: LOST on wave %d — %d retinue / %d brood still standing"),
			WaveIndex + 1, Swarm->GetAliveRetinue(), Swarm->GetAliveBrood());
		return;
	}

	switch (Phase)
	{
	case ERunPhase::Deploying:
		if (PhaseTimer >= DeploySeconds)
		{
			BeginWave();
		}
		break;

	case ERunPhase::WaveActive:
	{
		// Grace period: live counts come from the render pass, which reads zero
		// for a frame or two right after the batch spawn.
		const int32 CurrentBrood = PhaseTimer >= WaveClearGraceSeconds ? Swarm->GetAliveBrood() : -1;

		// --- task-064: stall tracking + timeout safety valve -------------------
		// Track how long the SAME nonzero count has held past the grace period.
		// A wave that is genuinely still grinding down resets this every time a
		// brood actually dies, so this only accumulates during real stalls (see
		// CVarWaveClearTimeoutSeconds's doc-comment for the measured root cause).
		bool bTimedOut = false;
		if (CurrentBrood > 0)
		{
			if (CurrentBrood != GStallLastBroodCount)
			{
				GStallLastBroodCount = CurrentBrood;
				GStallUnchangedSeconds = 0.f;
				GStallLogged = false;
			}
			else
			{
				GStallUnchangedSeconds += DeltaSeconds;

				const float Timeout = FMath::Max(CVarWaveClearTimeoutSeconds.GetValueOnGameThread(), 0.f);
				if (Timeout > 0.f && GStallUnchangedSeconds >= Timeout)
				{
					bTimedOut = true;
				}
				// Early warning well before the timeout could fire, so a wave that's
				// merely SLOW (not stuck) still gets its stragglers on record.
				else if (!GStallLogged && GStallUnchangedSeconds >= 6.f)
				{
					LogStalledBrood(*Swarm, WaveIndex);
					GStallLogged = true;
				}
			}
		}

		if (bTimedOut)
		{
			LogStalledBrood(*Swarm, WaveIndex);
			const int32 Destroyed = DestroyRemainingBrood(GetWorld());
			UE_LOG(LogTemp, Warning,
				TEXT("Run: wave %d TIMED OUT after %.0fs stuck at %d brood — force-cleared (destroyed %d entit%s). See Swarm.WaveClearTimeoutSeconds."),
				WaveIndex + 1, GStallUnchangedSeconds, CurrentBrood, Destroyed, Destroyed == 1 ? TEXT("y") : TEXT("ies"));
		}

		if (CurrentBrood == 0 || bTimedOut)
		{
			// Before WaveIndex moves and before BeginWave zeroes the wave side.
			LogWaveKills(*Swarm, WaveIndex);

			ResetStallTracking();
			SwarmSpawn::CompactTracked(GetWorld());
			++WaveIndex;

			if (WaveIndex >= WaveBroodCounts.Num())
			{
				EnterPhase(ERunPhase::Won);
				UE_LOG(LogTemp, Display, TEXT("Run: WON with %d retinue alive"), Swarm->GetAliveRetinue());
			}
			else
			{
				// Refill to the cap. Re-seat the slot cursor at the survivor
				// count so the formation re-packs from the inside out instead
				// of marching ever-outward as the cursor accumulates.
				const int32 Survivors = Swarm->GetAliveRetinue();
				const int32 Reinforcements = FMath::Max(0, RetinueCap - Survivors);
				RetinueSlotCursor = Survivors;
				SwarmSpawn::SpawnRetinue(GetWorld(), Reinforcements, RetinueSlotCursor);
				RetinueSlotCursor += Reinforcements;

				EnterPhase(ERunPhase::Breather);
				UE_LOG(LogTemp, Display, TEXT("Run: wave cleared — %d survivors, +%d reinforcements"),
					Survivors, Reinforcements);
			}
		}
		break;
	}

	case ERunPhase::Breather:
		if (PhaseTimer >= BreatherSeconds)
		{
			BeginWave();
		}
		break;

	case ERunPhase::Won:
	case ERunPhase::Lost:
	default:
		break;
	}
}
