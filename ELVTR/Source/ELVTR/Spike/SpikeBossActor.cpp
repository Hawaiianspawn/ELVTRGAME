#include "SpikeBossActor.h"

#include "DrawDebugHelpers.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"	// TActorIterator
#include "HAL/IConsoleManager.h"
#include "Mass/SwarmCombat.h"
#include "Mass/SwarmSpawn.h"
#include "Mass/SwarmSubsystem.h"

namespace
{
	TAutoConsoleVariable<float> CVarBossSpawnDistance(
		TEXT("Kindled.Boss.SpawnDistance"), 2200.f,
		TEXT("How far out, uu, the boss appears — on the same bearing the tide arrives from, so\n")
		TEXT("it walks in through its own wave rather than materialising behind the line.\n")
		TEXT("Just inside Swarm.BroodSpawnRadiusMin, so it is already visible when the wave is."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarBossDrawSize(
		TEXT("Kindled.Boss.DrawSize"), 170.f,
		TEXT("Half-extent, uu, of the boss's body box. Debug-drawn rather than a sprite: the\n")
		TEXT("atlas has no boss row and adding one is an art task, while a box can carry three\n")
		TEXT("mark decorations legibly today. See docs/design/slice-a7.md."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarBossDraw(
		TEXT("Kindled.Boss.Draw"), 1,
		TEXT("0 hides the boss's debug body. It still fights — this only stops it drawing."),
		ECVF_Default);

	/**
	 * Nearest live unit centroid of one TYPE. Same O(MaxSquads) = O(8) shape
	 * SwarmProcessors.cpp's NearestSpearmenCentroid already uses, for the same reason: eight
	 * fixed slots is not a per-body cross-entity query, so it stays Design-Law-5 clean.
	 * bFound is false when that type has nobody standing.
	 */
	FVector NearestCentroidOfType(const USwarmSubsystem& Swarm, const FVector& From, EUnitType Type, bool& bOutFound)
	{
		bOutFound = false;
		float BestSq = TNumericLimits<float>::Max();
		FVector Best = FVector::ZeroVector;
		for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
		{
			if (Swarm.GetSquadStanding(i) <= 0 || Swarm.GetSquadType(i) != Type)
			{
				continue;
			}
			const FVector Centroid = Swarm.GetSquadCentroid(i);
			const float DistSq = FVector::DistSquared2D(Centroid, From);
			if (DistSq < BestSq)
			{
				BestSq = DistSq;
				Best = Centroid;
				bOutFound = true;
			}
		}
		return Best;
	}

	/** Nearest live unit centroid of ANY type — the line, whichever part of it is closest. */
	FVector NearestRetinueCentroid(const USwarmSubsystem& Swarm, const FVector& From, bool& bOutFound)
	{
		bOutFound = false;
		float BestSq = TNumericLimits<float>::Max();
		FVector Best = FVector::ZeroVector;
		for (int32 i = 0; i < USwarmSubsystem::MaxSquads; ++i)
		{
			if (Swarm.GetSquadStanding(i) <= 0)
			{
				continue;
			}
			const FVector Centroid = Swarm.GetSquadCentroid(i);
			const float DistSq = FVector::DistSquared2D(Centroid, From);
			if (DistSq < BestSq)
			{
				BestSq = DistSq;
				Best = Centroid;
				bOutFound = true;
			}
		}
		return Best;
	}

}

uint8 ASpikeBossActor::ParseMarks(const FString& CommaSeparated)
{
	uint8 Marks = 0;
	// '+' and whitespace are accepted alongside ',' for two concrete reasons, not for taste.
	// A comma cannot survive -ExecCmds — the engine splits that argument on commas, so
	// `-ExecCmds="Kindled.Boss.AutoMarks quilled,ram"` arrives as two separate commands and
	// the boss silently comes out carrying only Quilled. And '+' is what MarksToString PRINTS,
	// so accepting it means a mark list can be copied straight back out of a log line.
	const TCHAR* Separators[] = { TEXT(","), TEXT("+"), TEXT(" ") };
	TArray<FString> Parts;
	CommaSeparated.ParseIntoArray(Parts, Separators, UE_ARRAY_COUNT(Separators), true);
	for (const FString& Part : Parts)
	{
		const FString Token = Part.TrimStartAndEnd();
		if (Token.Equals(TEXT("quilled"), ESearchCase::IgnoreCase)) { Marks |= (uint8)EBossMark::Quilled; }
		else if (Token.Equals(TEXT("ram"), ESearchCase::IgnoreCase)) { Marks |= (uint8)EBossMark::Ram; }
		else if (Token.Equals(TEXT("sated"), ESearchCase::IgnoreCase)) { Marks |= (uint8)EBossMark::Sated; }
		else if (Token.Equals(TEXT("none"), ESearchCase::IgnoreCase)) { Marks = 0; }
		else if (!Token.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Boss: unknown mark '%s' — known: quilled, ram, sated, none. "
				"(Wearing / Unblinded / Column-fed are specced in castle-layout.md §6.1 but need "
				"war-sim events that do not exist yet.)"), *Token);
		}
	}
	return Marks;
}

FString ASpikeBossActor::MarksToString(uint8 Marks)
{
	if (Marks == 0) { return TEXT("unmarked"); }
	FString Out;
	auto Append = [&Out](const TCHAR* Name)
	{
		Out += Out.IsEmpty() ? Name : *FString::Printf(TEXT("+%s"), Name);
	};
	if (HasMark(Marks, EBossMark::Quilled)) { Append(TEXT("QUILLED")); }
	if (HasMark(Marks, EBossMark::Ram)) { Append(TEXT("RAM")); }
	if (HasMark(Marks, EBossMark::Sated)) { Append(TEXT("SATED")); }
	return Out;
}

ASpikeBossActor::ASpikeBossActor()
{
	PrimaryActorTick.bCanEverTick = true;
	// Ticks before physics for the same reason every Mass pass does: what it publishes this
	// frame is what the grid build reads, and a post-physics write would be a frame late
	// against a sim that is already tolerating one frame of attractor lag and no more.
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	// AN ACTOR WITH NO ROOT COMPONENT HAS NO TRANSFORM. SpawnActor's location is discarded,
	// SetActorLocation silently returns false and GetActorLocation reports the world origin —
	// so the boss stood on the bearer from the frame it arrived and never moved. It cost a run
	// to find, because everything downstream behaved perfectly on the wrong position: it hit
	// the two nearest bodies for exactly its Ram body blow, and killed the bearer with exactly
	// one Ram objective blow. A bare USceneComponent is all it needs; there is no mesh, since
	// the body is debug-drawn.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetActorEnableCollision(false);
}

void ASpikeBossActor::BeginPlay()
{
	Super::BeginPlay();
	// Desynced from zero the same way every spawned body's swing clock is, so a boss that
	// arrives mid-wave does not land its first blow the instant it reaches the line.
	SwingTime = SwarmCombatTuning::BossSwingInterval() * 0.5f;
}

FVector ASpikeBossActor::ResolveTarget(const USwarmSubsystem& Swarm) const
{
	const FVector Here = GetActorLocation();
	const FVector Bearer = Swarm.GetAttractor();
	const uint8 Marks = Swarm.GetBoss().Marks;

	// §6.1's Ram — "prefers gates over bodies". It does not fight the line, it walks through
	// it at the thing it came for. PROTOTYPE SUBSTITUTION: this slice has no gate (out of
	// scope by the brief), so the bearer stands in as the objective. The behaviour is the
	// specced one; what it is aimed at is not. See docs/design/slice-a7.md.
	if (HasMark(Marks, EBossMark::Ram))
	{
		return Bearer;
	}

	// §6.1's Quilled — "seeks ranged positions". It is armoured against arrows, so it spends
	// that armour going after the archers rather than standing in front of the spear line.
	// Falls through to the nearest line, then the bearer, so it never stalls when the ranged
	// half of the army is dead.
	if (HasMark(Marks, EBossMark::Quilled))
	{
		bool bFound = false;
		const FVector Archers = NearestCentroidOfType(Swarm, Here, EUnitType::Archers, bFound);
		if (bFound)
		{
			return Archers;
		}
	}

	bool bFoundLine = false;
	const FVector Line = NearestRetinueCentroid(Swarm, Here, bFoundLine);
	return bFoundLine ? Line : Bearer;
}

void ASpikeBossActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	USwarmSubsystem* Swarm = GetWorld() ? GetWorld()->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return;
	}
	USwarmSubsystem::FBossState& Boss = Swarm->GetMutableBoss();
	if (!Boss.bAlive)
	{
		Destroy();
		return;
	}

	// --- take what the line dealt last frame -----------------------------------
	// Consume-and-clear, the same shape ConsumePendingHeroDamage has. The combat pass is the
	// only writer and this is the only reader, so nothing races over it.
	const float Damage = Swarm->ConsumePendingBossDamage();
	if (Damage > 0.f)
	{
		Boss.HP -= Damage;
		CalmSeconds = 0.f;
	}
	else
	{
		CalmSeconds += DeltaSeconds;
	}

	// --- §6.1 Sated: regeneration between engagements ---------------------------
	// Gated on time since the last blow, not on distance to anything: "between engagements"
	// is a statement about being left alone, and a boss that is being trickled at by one
	// archer is not being left alone. This is what makes §6.3's counter real — Sated wants
	// burst that outruns regeneration, so a squad that chips and disengages never finishes it.
	if (HasMark(Boss.Marks, EBossMark::Sated) && CalmSeconds >= SwarmCombatTuning::BossSatedCalmSeconds())
	{
		Boss.HP = FMath::Min(Boss.HP + SwarmCombatTuning::BossSatedRegenPerSecond() * DeltaSeconds, Boss.MaxHP);
	}

	if (Boss.HP <= 0.f)
	{
		Boss.HP = 0.f;
		Boss.bAlive = false;
		Boss.bStriking = false;
		UE_LOG(LogTemp, Display, TEXT("Boss: DOWN (%s)"), *ASpikeBossActor::MarksToString(Boss.Marks));
		Destroy();
		return;
	}

	// --- move ------------------------------------------------------------------
	const float MeleeRange = SwarmCombatTuning::BossMeleeRange();
	const float ReachSq = FMath::Square(MeleeRange);
	const FVector Target = ResolveTarget(*Swarm);
	FVector Here = GetActorLocation();
	const FVector ToTarget = FVector(Target.X - Here.X, Target.Y - Here.Y, 0.f);
	const float Dist = ToTarget.Size2D();

	// --- when to stop walking ---------------------------------------------------
	// IT STOPS ON CONTACT, NOT ON THE TARGET POINT — and the difference was measured, not
	// assumed. ResolveTarget returns a unit CENTROID, and a unit's centroid is the average of
	// a formation block that can be over a thousand uu deep; parking on it left the boss
	// standing in a thin part of the line with a peak of 1-3 soldiers ever inside its reach,
	// against entity-tiers.md §4's surround cap of 45. That made the cap untestable, which is
	// the one thing §5 asks a real implementation to produce a measurement for.
	//
	// So it walks until at least one soldier is genuinely within its reach. The grid query is
	// the same 3x3 neighbourhood everything else uses — cheap, and by the time it matters the
	// boss is close enough for that reach to be meaningful; far out it simply finds nothing
	// and keeps marching.
	//
	// RAM DOES NOT STOP FOR BODIES. §6.1: it prefers gates over bodies, so contact with the
	// line is not a reason to stand still — it keeps going until it reaches what it came for.
	const bool bRam = HasMark(Boss.Marks, EBossMark::Ram);
	bool bStop = false;
	if (bRam)
	{
		bStop = Dist <= MeleeRange * 0.75f;
	}
	else
	{
		Swarm->QueryNeighbors(Here, [&bStop, &Here, ReachSq](const USwarmSubsystem::FGridEntry& Entry)
		{
			if (!bStop && Entry.bRetinue && FVector::DistSquared2D(Entry.Location, Here) <= ReachSq)
			{
				bStop = true;
			}
		});
	}

	if (!bStop && Dist > 1.f)
	{
		Here += ToTarget.GetSafeNormal2D() * FMath::Min(SwarmCombatTuning::BossSpeed() * DeltaSeconds, Dist);
		SetActorLocation(Here);
	}

	// --- swing clock -----------------------------------------------------------
	// Edge-triggered on the strike point, exactly like every Mass body's — so its blow lands
	// once per interval regardless of frame rate, and the windup before it is a real tell.
	const float Interval = SwarmCombatTuning::BossSwingInterval();
	const float StrikeAt = Interval * SwarmCombatTuning::SwingStrikeAt();
	const float Previous = SwingTime;
	SwingTime += DeltaSeconds;
	const bool bStrike = (Previous < StrikeAt && SwingTime >= StrikeAt);
	if (SwingTime >= Interval)
	{
		SwingTime = FMath::Fmod(SwingTime, Interval);
	}

	// --- publish ---------------------------------------------------------------
	// Ram's blow against a SOLDIER is cut hard: it is not fighting the line, it is shouldering
	// through it. The full-weight version lands on the objective below.
	const float BaseBlow = SwarmCombatTuning::BossDPS() * Interval;
	Boss.Location = Here;
	Boss.bStriking = bStrike;
	Boss.BlowDamage = HasMark(Boss.Marks, EBossMark::Ram)
		? BaseBlow * SwarmCombatTuning::BossRamBodyScale()
		: BaseBlow;
	Boss.ReachSq = FMath::Square(MeleeRange);
	Boss.TargetsPerHit = SwarmCombatTuning::BossTargetsPerHit();

	// --- the blow against the bearer -------------------------------------------
	// The bearer is a pawn, not a Mass entity, so nothing in the grid can damage him — the
	// boss has to hand it over the same way the combat pass does for brood. Four lines, and
	// it is the mirror of the branch that already exists there.
	if (bStrike)
	{
		const FVector Bearer = Swarm->GetAttractor();
		if (FVector::DistSquared2D(Here, Bearer) <= Boss.ReachSq && Swarm->IsHeroAlive())
		{
			const float Objective = HasMark(Boss.Marks, EBossMark::Ram)
				? BaseBlow * SwarmCombatTuning::BossRamObjectiveScale()
				: BaseBlow;
			Swarm->AddPendingHeroDamage(Objective);
		}
	}

	if (CVarBossDraw.GetValueOnGameThread() != 0)
	{
		Draw(*Swarm);
	}
}

void ASpikeBossActor::Draw(const USwarmSubsystem& Swarm) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const USwarmSubsystem::FBossState& Boss = Swarm.GetBoss();
	const float Size = FMath::Max(CVarBossDrawSize.GetValueOnGameThread(), 10.f);
	const FVector Base = Boss.Location + FVector(0.f, 0.f, Size);

	// §6.1's hard requirement: each mark is VISIBLE ON THE SILHOUETTE, and visible BEFORE it
	// acts. So every decoration below is drawn from the boss's persistent mark state, never
	// from what it is doing this frame — a player reads Quilled the moment it walks out of
	// the dark, not the first time an arrow bounces.
	//
	// ponytail: debug draw, not a sprite. The atlas has no boss row (SwarmSheet::Enemy is nine
	// brood looks) and authoring one is a PixelLab task with its own request file, palette pass
	// and import; a box with three attachments carries the read today. Upgrade path is an
	// enemy-atlas variant plus per-mark overlay frames — nothing in the sim changes, since the
	// boss is already an Actor drawing itself.
	const bool bQuilled = HasMark(Boss.Marks, EBossMark::Quilled);
	const bool bRam = HasMark(Boss.Marks, EBossMark::Ram);
	const bool bSated = HasMark(Boss.Marks, EBossMark::Sated);

	// Sated reads as swollen and lit from inside, so it changes the BODY, not an attachment —
	// the one mark you should be able to read from across the arena at a glance.
	const float Swell = bSated ? 1.28f : 1.f;
	const FVector Extent(Size * Swell, Size * Swell, Size * 1.35f * Swell);

	// Windup tell (Design Law 6): the body brightens through the windup and snaps back on the
	// recovery, so a 2s swing is a beat you can see coming rather than a longer version of a
	// soldier's jab.
	const float Interval = SwarmCombatTuning::BossSwingInterval();
	const float StrikeAt = Interval * SwarmCombatTuning::SwingStrikeAt();
	const float Windup = StrikeAt > 0.f ? FMath::Clamp(SwingTime / StrikeAt, 0.f, 1.f) : 0.f;
	const uint8 Heat = (uint8)FMath::Lerp(70.f, 255.f, SwingTime < StrikeAt ? Windup : 0.15f);
	const FColor Body(Heat, (uint8)(Heat / 4), (uint8)(Heat / 5));

	DrawDebugSolidBox(World, Base, Extent, Body, false, -1.f, 0);

	// A health bar in world space, because the boss's HP is the one number that decides
	// whether a mark is working and the HUD line alone cannot be watched while fighting.
	const float Frac = Boss.MaxHP > 0.f ? FMath::Clamp(Boss.HP / Boss.MaxHP, 0.f, 1.f) : 0.f;
	const FVector BarBase = Boss.Location + FVector(0.f, 0.f, Size * 3.2f);
	const float BarHalf = Size * 1.6f;
	DrawDebugLine(World, BarBase - FVector(0.f, BarHalf, 0.f), BarBase + FVector(0.f, BarHalf, 0.f),
		FColor(40, 40, 40), false, -1.f, 0, 14.f);
	DrawDebugLine(World, BarBase - FVector(0.f, BarHalf, 0.f),
		BarBase - FVector(0.f, BarHalf, 0.f) + FVector(0.f, 2.f * BarHalf * Frac, 0.f),
		bSated ? FColor(120, 255, 140) : FColor(230, 60, 40), false, -1.f, 0, 14.f);

	if (bQuilled)
	{
		// Shafts still in it. Eight of them, radiating and tilted up, so the outline is broken
		// by spikes from every viewing angle rather than only from the side.
		for (int32 i = 0; i < 8; ++i)
		{
			const float Angle = (float)i * (2.f * PI / 8.f);
			const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.55f);
			const FVector Start = Boss.Location + FVector(0.f, 0.f, Size * 1.1f) + Dir * Size * 0.5f;
			DrawDebugLine(World, Start, Start + Dir.GetSafeNormal() * Size * 1.5f,
				FColor(245, 235, 190), false, -1.f, 0, 9.f);
		}
	}

	if (bRam)
	{
		// Carrying the door: a wide flat slab held out in front, on its heading. Ram always
		// walks at the objective (ResolveTarget), so that direction IS its heading — no need
		// to remember a facing.
		FVector Facing = (Swarm.GetAttractor() - Boss.Location).GetSafeNormal2D();
		if (Facing.IsNearlyZero())
		{
			Facing = FVector::ForwardVector;
		}
		const FVector Ahead = Boss.Location + Facing * Size * 1.5f + FVector(0.f, 0.f, Size * 1.1f);
		DrawDebugSolidBox(World, Ahead, FVector(Size * 0.18f, Size * 1.9f, Size * 1.25f),
			Facing.Rotation().Quaternion(), FColor(155, 130, 95), false, -1.f, 0);
	}

	if (bSated)
	{
		// Lit from inside. A small bright core inside the swollen body, so the mark reads even
		// when the body itself is dark at the edge of the flame pool.
		DrawDebugSolidBox(World, Base, FVector(Size * 0.45f), FColor(255, 245, 205), false, -1.f, 0);
	}
}

ASpikeBossActor* ASpikeBossActor::SpawnBoss(UWorld* World, uint8 Marks)
{
	USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
	if (!Swarm)
	{
		return nullptr;
	}
	ClearBoss(World);

	// The SAME bearing the tide arrives on, resolved by the same helper the garrison's held
	// line uses, so the boss walks in through its own wave instead of appearing behind the
	// line — and so the two cannot end up on opposite sides of the bearer.
	const FVector Where = SwarmSpawn::TideBearingPoint(World, CVarBossSpawnDistance.GetValueOnGameThread());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpikeBossActor* Boss = World->SpawnActor<ASpikeBossActor>(ASpikeBossActor::StaticClass(), Where, FRotator::ZeroRotator, Params);
	if (!Boss)
	{
		return nullptr;
	}

	USwarmSubsystem::FBossState& State = Swarm->GetMutableBoss();
	State = USwarmSubsystem::FBossState{};
	State.Location = Where;
	State.MaxHP = SwarmCombatTuning::BossMaxHP();
	State.HP = State.MaxHP;
	State.Marks = Marks;
	State.bAlive = true;
	State.ReachSq = FMath::Square(SwarmCombatTuning::BossMeleeRange());
	State.TargetsPerHit = SwarmCombatTuning::BossTargetsPerHit();
	State.BlowDamage = SwarmCombatTuning::BossDPS() * SwarmCombatTuning::BossSwingInterval();

	UE_LOG(LogTemp, Display, TEXT("Boss: arrives %s — %.0f HP, Armor %.0f, %.0f DPS, reach %.0fuu, at (%.0f, %.0f)"),
		*ASpikeBossActor::MarksToString(Marks), State.MaxHP, SwarmCombatTuning::BossArmor(),
		SwarmCombatTuning::BossDPS(), SwarmCombatTuning::BossMeleeRange(), Where.X, Where.Y);
	return Boss;
}

void ASpikeBossActor::ClearBoss(UWorld* World)
{
	if (!World)
	{
		return;
	}
	if (USwarmSubsystem* Swarm = World->GetSubsystem<USwarmSubsystem>())
	{
		Swarm->GetMutableBoss() = USwarmSubsystem::FBossState{};
		Swarm->ConsumePendingBossDamage();
	}
	for (TActorIterator<ASpikeBossActor> It(World); It; ++It)
	{
		It->Destroy();
	}
}

//----------------------------------------------------------------------
// Console surface (prototype — nothing accretes a mark yet, see the header)
//----------------------------------------------------------------------
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GBossSpawnCmd(
		TEXT("Kindled.Boss.Spawn"),
		TEXT("Spawn the marked boss. Usage: Kindled.Boss.Spawn [quilled,ram,sated|none]\n")
		TEXT("Marks compound (castle-layout.md §6.1): 'Kindled.Boss.Spawn quilled,ram' arrives\n")
		TEXT("as a gate-breaker you cannot shoot. Replaces any boss already standing."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				ASpikeBossActor::SpawnBoss(World, ASpikeBossActor::ParseMarks(FString::Join(Args, TEXT(","))));
			}));

	FAutoConsoleCommandWithWorldAndArgs GBossMarksCmd(
		TEXT("Kindled.Boss.Marks"),
		TEXT("Re-mark the standing boss, live. Usage: Kindled.Boss.Marks quilled,sated | none\n")
		TEXT("Every mark changes behaviour AND silhouette on the frame you set it, so this is\n")
		TEXT("the A/B: watch the same boss stop seeking your archers when Quilled comes off."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm || !Swarm->IsBossAlive())
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Boss.Marks: no boss standing — Kindled.Boss.Spawn first."));
					return;
				}
				Swarm->GetMutableBoss().Marks = ASpikeBossActor::ParseMarks(FString::Join(Args, TEXT(",")));
				UE_LOG(LogTemp, Display, TEXT("Boss: now %s"), *ASpikeBossActor::MarksToString(Swarm->GetBoss().Marks));
			}));

	FAutoConsoleCommandWithWorld GBossClearCmd(
		TEXT("Kindled.Boss.Clear"),
		TEXT("Remove the standing boss."),
		FConsoleCommandWithWorldDelegate::CreateStatic(
			[](UWorld* World) { ASpikeBossActor::ClearBoss(World); }));

	FAutoConsoleCommandWithWorld GBossReportCmd(
		TEXT("Kindled.Boss.Report"),
		TEXT("Log the boss's live state beside the stat block it came from — HP, marks, and the\n")
		TEXT("PEAK number of soldiers that landed a blow since the last report, against\n")
		TEXT("entity-tiers.md §4's surround-cap ESTIMATE. If that peak sits pinned at\n")
		TEXT("Kindled.Boss.SurroundCap the estimate is wrong and wants measuring, not raising."),
		FConsoleCommandWithWorldDelegate::CreateStatic(
			[](UWorld* World)
			{
				USwarmSubsystem* Swarm = World ? World->GetSubsystem<USwarmSubsystem>() : nullptr;
				if (!Swarm || !Swarm->IsBossAlive())
				{
					UE_LOG(LogTemp, Warning, TEXT("Kindled.Boss.Report: no boss standing."));
					return;
				}
				const USwarmSubsystem::FBossState& B = Swarm->GetBoss();
				UE_LOG(LogTemp, Display,
					TEXT("Boss: %s | %.0f / %.0f HP | Armor %.0f (+%.0f vs archers while Quilled) | "
						"blow %.0f | peak attackers %d / cap %d"),
					*ASpikeBossActor::MarksToString(B.Marks), B.HP, B.MaxHP, SwarmCombatTuning::BossArmor(),
					HasMark(B.Marks, EBossMark::Quilled) ? SwarmCombatTuning::BossQuilledArmor() : 0.f,
					B.BlowDamage, Swarm->ConsumeBossAttackersPeak(), SwarmCombatTuning::BossSurroundCap());
			}));
}
