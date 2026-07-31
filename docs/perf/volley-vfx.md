# Archer volley cue (task-129) — what was built, what it costs, and how often one shot fires twice

**Built and measured 2026-07-31**, `build-space-differentiates` branch, Development Editor
(`UnrealEditor.exe`), `L_Spike1`. Visual evidence from `ASpike1GameMode`'s own auto-fight wave
run in PIE; frame times from a `-SwarmBench` sweep (§4).

**`EditorPerformanceSettings.bThrottleCPUWhenNotForeground` was disabled on the live CDO for
every run below and restored to `true` afterwards** (`docs/AGENT-TEAMS.md` §8a). Do not skip this
on the strength of `ELVTR/Config/DefaultEditorPerProjectUserSettings.ini:9` already reading
`False` — that line is in the wrong config class and has no effect; the setting that governs is
engine-wide and reads `True`. The first sweep of this task was thrown away because of it (§7).

## 1. What was built

Archers engage from 750uu (`Swarm.ArchersEngageRange`) and, before this, nothing at all
happened on screen when one shot — a brood simply took damage and flashed 750uu away.
`docs/design/squad-group-system.md` §2.2 specced the missing half in one line: *"the visual is
a volley (a cheap arcing Niagara trail or ribbon triggered on `SwingBit`, purely cosmetic, no
gameplay state)"*. That line, and nothing more.

**`UVolleySubsystem`** (`ELVTR/Source/ELVTR/Rendering/VolleySubsystem.{h,cpp}`) — a
`UTickableWorldSubsystem` copied almost verbatim from `UBloodSubsystem`. Each frame it makes
ONE pass over the swarm's already-published render arrays
(`USwarmSubsystem::GetRenderPositions` / `GetRenderAnimBits`) and, from that single pass,
gets both things it needs:

- every non-team entity's position summed, so the brood **centroid** can be averaged out
  afterwards;
- the archers wearing `SwarmAnim::SwingBit` that rolled a cue this frame.

An archer is identified exactly as `SwarmRenderActor.cpp`'s pack loop already does it —
`SwarmSquad::UnitType(SwarmRenderPack::Squad(Bits)) == EUnitType::Archers` — so sprite, stats
and cue cannot disagree about who is an archer. Spearmen and brood produce nothing.

**No sim change.** No new fragment, no new processor, no new render-buffer bit, no gameplay
state, nothing written back into `ELVTR/Source/ELVTR/Mass/**`. The subsystem reads the sim's
published output and is otherwise invisible to it.

**`NS_Volley`** (`ELVTR/Content/Gore/NS_Volley.uasset`) — `NS_Blood` duplicated (via
`NiagaraToolset_System.CreateNiagaraSystem` with `NS_Blood` as template, after the editor
Python `AssetTools.duplicate` turned out to be broken here, §7) and retuned:

| | `NS_Blood` | `NS_Volley` |
|---|---|---|
| `AddVelocity` launch box min | (-100, -100, 40) | **(900, -60, 320)** |
| `AddVelocity` launch box max | (100, 100, 180) | **(1100, 60, 440)** |
| `InitializeParticle` Color | (0.45, 0.015, 0.01) dark red | **(0.95, 0.82, 0.5) pale straw** |
| `User.Lifetime` default | 0.3 | **0.75** |
| `User.Size` default | 6 | **5** |
| `User.Count` default | 0 | **3** |

Everything else is inherited unchanged, and two inherited pieces are doing the real work:

- **`GravityForce` (0, 0, -980), already in `NS_Blood`,** is what bends the shot into an arc.
  No new module was needed to make the volley arc — the blood spray was already ballistic.
- **`AddVelocity` reads its box in LOCAL space** (bound to
  `Engine.Owner.SystemLocalToWorld`), pointing down local +X. So aiming is just the yaw handed
  to `SpawnSystemAtLocation`, exactly the mechanism blood already used to spray along the
  fight line. **Aiming needed no new user parameter and no new module.**

**`NS_Blood` itself is untouched**, checked three ways: its colour and launch box read back
unchanged after the retune (`(0.45, 0.015, 0.01)` and `(100, 100, 180)`); `NS_Blood.uasset` still
carries its pre-existing mtime and size (2026-07-29 21:05, 372,303 bytes) after a session that
ran entirely on 07-31; and the retune of `NS_Volley` demonstrably did not propagate, which
confirms `CreateNiagaraSystem` produced a real copy rather than emitters inheriting from
`NS_Blood`'s. (`NS_Blood.uasset` does show as modified in `git status` — that predates this task
and is not ours.)

Verified against the brief's known traps: `NS_Volley`'s `Swarm` emitter is **`CPUSim`**
(the `GPUComputeSim` trap that made `NS_Swarm` draw nothing), and the disabled
`ConfettiBurst` emitter came across disabled and was left alone. The per-particle-colour trap
does not apply — `M_Blood` already reads particle colour, which is why blood is red at all.

### Direction: one shared centroid, not per-shot targeting

There is no per-shot victim in the render buffer, and putting one there is exactly the
per-entity uniqueness cost Design Law 5 rules out at horde scale — `squad-group-system.md`
§2.2 says so in as many words. So **every cue in a frame flies at the same brood centroid**,
clamped to `Swarm.ArchersEngageRange` so a cue never shows the player a shot the sim could not
have taken. `User.SpeedScale` is set per cue to `Range / (Volley.AuthoredSpeed *
Volley.Lifetime)`, which is what makes the arc land where the brood are.

Yaw only, never pitch: pitching the component toward the target would fight the launch box's
own +Z and flatten the arc into a straight line at exactly the moment the brood get close,
which is when the arc reads best.

## 2. How often one shot produces more than one cue — the honest number

**`SwingBit` is a pose window, not an event, so this is not once-per-shot and cannot be made
so without adding per-entity state this task is scoped out of.**

`USwarmSwingProcessor` (`SwarmProcessors.cpp:1084-1085, 1160`) holds `SwingBit` for
`StrikeAt*0.5 .. StrikeAt + Interval*0.18`. At shipped defaults (`Swarm.SwingInterval` 0.9,
`Swarm.SwingStrikeAt` 0.35) that is **0.1575s .. 0.4770s — a 0.3195s window**, about 35% of
every swing, i.e. **~19 consecutive frames at 60fps all carrying the same single shot**. The
render buffer has no stable per-entity id to edge-detect that one shot with, and adding one is
`SwarmSubsystem.h`'s business, out of bounds here — the identical constraint `BloodSubsystem`'s
header records for `HitFlashBit`.

Emitting a cue on every one of those ~19 frames would be absurd, so each swinging archer
instead rolls `Volley.CueRate * DeltaSeconds` per frame. At the shipped `CueRate 3.1`
(= 1 / 0.3195) the count of cues from one shot is Poisson with mean **0.99**:

| Cues produced by one shot | Probability |
|---|---|
| **0 — the shot is silent** | **37%** |
| 1 | 37% |
| 2 | 18% |
| 3 or more | 8% |

So: **one shot in three produces no arrow at all, and one in four produces two or more.** It
is one cue per shot *in expectation only*. Frame-rate independent, because the roll is a rate
against `DeltaSeconds` rather than a per-frame chance.

This was a deliberate choice over the two alternatives:
- **Fire on every `SwingBit` frame** — correct-by-construction but ~19x the cues, an
  unreadable wall of arrows, and cost scaling with frame rate.
- **Edge-detect the real shot** — needs a per-entity id or a published strike flag in the
  render buffer. That is the sim's territory and the exact cost Design Law 5 forbids.

If the mismatch ever matters more than the cost, the fix is a published edge in the combat
pass, not a cleverer subsystem. It is not needed for a cosmetic cue.

## 3. What the cap does at density

`Volley.MaxPerFrame` (16) is the hard ceiling, the same idiom as `Blood.MaxBurstsPerFrame`.
**At realistic archer counts it never binds** — `CueRate` is what actually keeps the number
down:

- Expected cues per frame = `N_swinging * CueRate * DeltaSeconds`.
- At 60fps that is `N_swinging * 0.0517`, so the cap at 16 is only reached at **~310
  simultaneously-swinging archers**.
- Only ~35% of engaged archers are inside the pose window at any instant (0.3195s of every
  0.9s), so that is roughly **~870 engaged archers** before a single cue is ever dropped.

The wave-3 run captured in §5 had **34-52 archers**, producing ~2-3 cues per frame — an order
of magnitude under the cap. The cap is insurance against an archer line far larger than the
game currently fields, not a dial that shapes the shipped look.

When it does bind, archers beyond the cap simply don't fire a visible arrow that frame. They
still deal their damage — the cue is cosmetic and always was. Which archers get skipped
follows the render buffer's publish order, the same arbitrary-but-bounded tie-break
`Swarm.MaxAttackersPerUnit` and `Blood.MaxBurstsPerFrame` already use.

The cap is deliberately tested **inside** the retinue branch of the scan, not at the top of
the loop, so hitting the cue budget can never truncate the brood centroid — otherwise the last
cues of a frame would aim at the average of only the brood that happened to be published early.

## 4. Frame time

`-SwarmBench` sweep, retinue 100 (so ~20 archers at `Swarm.ArcherGrowthWeight` 0.2), 8s settle
+ 5s sample per step, three configs in one session. Full CSV:
`docs/perf/evidence/task129/SwarmBench-task129.csv`. `frame_ms`:

| brood | volley OFF | volley ON | Δ ON | saturated | Δ saturated |
|---|---|---|---|---|---|
| 1,000 | 8.795 | 8.353 | **-0.442 (-5.0%)** | 11.972 | **+3.177 (+36.1%)** |
| 5,000 | 11.147 | 10.324 | **-0.823 (-7.4%)** | 12.857 | +1.710 (+15.3%) |
| **10,000** | **14.087** | **13.329** | **-0.758 (-5.4%)** | **16.666** | **+2.579 (+18.3%)** |
| 20,000 | 21.211 | 20.330 | -0.881 (-4.2%) | 21.466 | +0.255 (+1.2%) |
| 30,000 | 28.132 | 28.313 | +0.181 (+0.6%) | 29.800 | +1.668 (+5.9%) |
| **40,000** | **36.133** | **36.440** | **+0.307 (+0.8%)** | **37.810** | **+1.677 (+4.6%)** |

**At the shipped `CueRate` the cue is free — and the reason to believe that is that four of the
six ON deltas are NEGATIVE.** Added work cannot make a frame faster, so those -0.4 to -0.9ms
readings are the harness's own drift, and they put its measurement floor at roughly **±0.9ms**.
Every shipped-rate delta sits inside that floor. The honest statement is not "the volley costs
+0.3ms at 40k", it is **"the volley's cost is below what this harness can resolve, which is
about 0.9ms"**.

The drift has an identifiable cause worth recording: configs exec into **one** session in
sequence, so OFF ran on a cold machine and ON ran a few minutes later on a warm one. Interleaving
counts within a config, or one launch per config, would remove it. Not re-run — the conclusion
("below the floor") does not change, and the SATURATED column already bounds the real cost from
above.

**The saturated column is the number that actually matters**, because it is the only one that is
comfortably outside the floor. With `Volley.CueRate 1000` (so the clamp makes every swinging
archer fire every frame and `Volley.MaxPerFrame` is hit continuously) the cue costs **+3.18ms at
1,000 brood** — real, and consistent with `blood-particles.md` §1's finding that
`SpawnSystemAtLocation` per burst is genuinely expensive on the game thread. **So the cap's
ceiling is not free; `Volley.CueRate` is what keeps the shipped build off it.**

**Caveat on the high-count rows, stated plainly:** the saturated delta *shrinks* as brood count
rises (+3.18ms at 1k, +0.26ms at 20k). That is not the cue getting cheaper — it is the 100-strong
retinue being wiped by 20,000+ brood before the 5s sample window opens, leaving few or no archers
alive to fire. **At 20k-40k these rows increasingly measure only the subsystem's O(N) scan over
the render arrays, not the cue-spawn path.** The 1,000-10,000 rows are where the cue-spawn cost is
genuinely exercised. A harness that could hold an archer line alive at 40k would report a larger
saturated delta there; this one cannot, and `BenchmarkRetinueCount` is not a CVar
(`SwarmRenderActor.h`, outside this task's files).

**Against `task126`:** that CSV reports 10,000 → 12.410ms and 40,000 → 35.077ms. This session's
volley-OFF baseline is 14.087 / 36.133 — within 3% at 40k, ~13% at 10k, and `task126` reports a
non-zero `draw_ms` (~2ms) where every row in this session reports 0.000. Different session,
slightly different machine state; **the in-session OFF column, not `task126`, is the baseline the
deltas above are computed against**, which is why it was measured at all.

Both `task126`'s numbers and these sit at the same place relative to budget: 60fps (16.6ms) holds
to somewhere between 10,000 and 20,000 brood, and the volley does not move that boundary.

## 5. Does the centroid direction read at density?

**Yes, at both camera angles tested, and it is not marginal.** Evidence in
`docs/perf/evidence/task129/`:

Densities below are read off the game's own `wave N board` / `spawned N brood` log lines at each
capture instant, not estimated from the picture.

| Shot | Density at capture | What it shows |
|---|---|---|
| `01-shipped-framing-volley-on.png` + `01b-shipped-zoom.png` | **570** (450 brood + 120 retinue), 6s into wave 2 | **The shipped eye-level camera** (`Kindled.Cam.Pitch -8.2`) — the shot the player actually gets. The strongest read of the set: near-camera arrows are large pale squares climbing over the fight line, unmistakable against the near-black brood, alongside blood and hit-flash. |
| `03-overhead-volley-on.png` + `03b-overhead-zoom.png` | **820** (700 brood + 120 retinue), 26s into **wave 3** | Overhead diagnostic (pitch -62, dist 2000, brood lighting raised so the target mass reads; **not the shipped look**). Bow-carrying archers on one side, brood on the other, a scatter of arrows crossing the gap between them. This is the wave-3-density capture the task asked for. |
| `04-overhead-control-volley-off.png` + `04b-control-zoom.png` | **570**, wave 2 | **The control** — same scenario and framing, `Volley.Enable 0`, zero arrows anywhere. What makes the shot above evidence rather than an assertion about pale pixels. **Caveat: it landed a wave earlier than its ON counterpart** (the wave clock drifts run to run), so the pair is matched on framing and pipeline but not on entity count. It supports "nothing appears when disabled", which density does not affect; it is not a like-for-like density A/B. |
| `06-encircled-arc360.png` | 570, wave 2 | An **attempted** encirclement test, see below. |

**The honest weakness is encirclement, and it is not fixed.** The centroid is the mean of *every*
brood. With the shipped `Swarm.BroodSpawnArc 120` they arrive as a front, the mean sits out ahead
of the line, and every archer fires the same way the fight is facing — which is exactly why it
reads. If the brood ever genuinely surround the army, that mean collapses toward the army's own
centre, and archers on the far side would fire **inward, across their own line**. The spawner can
produce that state (`Swarm.BroodSpawnArc` 360 — its own comment calls it "the surrounded case"),
so this is a reachable configuration, not a hypothetical.

**I tried to capture it and did not succeed.** `06-encircled-arc360.png` was shot with
`Swarm.BroodSpawnArc 360`, but in the captured frame the brood had still converged into one mass
on one side rather than a ring, so **it neither confirms nor refutes the concern**. It is
included labelled as such rather than dropped. Treat the inward-fire risk as **reasoned from the
code, not measured** — and cheap to fix if it ever bites (weight the centroid toward brood inside
`Swarm.ArchersEngageRange` of the *archer line's* own centroid, still one pass, still no
per-entity targeting).

**One thing the brief asked for that did not land as specified:** §2.2 and the task both say
"arcing streak". What ships is a clutch of **three square sprites on a ballistic arc**, not a
stretched shaft or a ribbon. Because the launch box spreads forward speed 900-1100, the three
string out along the trajectory, so in *motion* they read as a streak — but a **still frame shows
dots, not a streak**. A true shaft needs the sprite renderer set to velocity-aligned with a
non-uniform sprite size, which breaks the `User.Size` → `Uniform Sprite Size` binding that
`Volley.Size` drives. Judged not worth that trade for a cosmetic cue, but it is the obvious next
move if the owner wants a harder read: it is a renderer property plus one size module, not a
rebuild.

## 6. The dials

| CVar | Value | Why |
|---|---|---|
| `Volley.Enable` | 1 | Master on/off; 0 skips the render-array scan entirely, so the feature costs nothing when off. This is the switch §4's control column used. |
| `Volley.CueRate` | 3.1 | **The dial that makes it free.** Cues/sec per swinging archer; `1 / 0.3195` = one cue per shot in expectation. Retune if `Swarm.SwingInterval` or `Swarm.SwingStrikeAt` move — the formula is 1 / (pose window). |
| `Volley.MaxPerFrame` | 16 | **The cost-bounding dial**, same idiom as `Blood.MaxBurstsPerFrame`. Never binds below ~870 engaged archers (§3). §4 shows saturating it costs +3.18ms, which is what it exists to prevent. |
| `Volley.ArrowsPerCue` | 3 | Sprites per cue. A volley reads as a clutch of shafts, not one dot; the launch box's speed spread strings them along the arc. Drives `User.Count`. |
| `Volley.Lifetime` | 0.75s | Flight time, and half of how far the arc reaches (see `AuthoredSpeed`). Drives `User.Lifetime`. |
| `Volley.Size` | 5uu | Sprite half-size, pixel-scale to match the sprites. Drives `User.Size`. |
| `Volley.HeightOffset` | 25uu | Launch height above the published (feet) position — bow height. Higher than `Blood.HeightOffset` 15 on purpose. |
| `Volley.AuthoredSpeed` | 1000 | **Not a speed dial.** The forward speed `NS_Volley` is authored at, used to turn a wanted range into `User.SpeedScale`. Change only to match an actual edit to the asset's launch box. |

These are C++ defaults only — **nothing was added to `Saved/SwarmExecOnPlay.txt` or
`ELVTR/Config/SwarmExecOnPlay.canonical.txt`.** That surface is the `/cvars` skill's, and the
shipped defaults need no override. The working copy was restored byte-exact (MD5 verified) after
the temporary evidence-staging block was removed.

## 7. Notes for whoever touches this next

- **`editor_toolset.toolsets.asset.AssetTools` is broken in this editor build for
  path-addressed calls.** `exists`, `is_dirty`, `duplicate` and `save_assets(["/Game/..."])`
  all return `false` / `"Asset does not exist"` for paths that `find_assets` in the *same
  toolset* returns verbatim (`/Game/Gore/M_Blood` included). What works: `find_assets`, and
  `save_assets([])` — the save-all form, which is the idiom this project already records.
  Duplication had to go through `NiagaraToolset_System.CreateNiagaraSystem` with a
  `templateSystem` instead. Worth knowing before losing a session to it.
- **`save_assets([])` saves everything dirty, including other agents' work.** Since every
  path-addressed form is broken (above), the save-all form is the only way to get an asset to
  disk — and in this run it also wrote `ELVTR/Content/Spike1/L_Spike1.umap`, which is outside
  this task's owned files. Flagged in the handback; the level was already dirty in the shared
  editor, so the write makes disk match what the editor was holding, but it is still a write
  this task did not intend and should not have been able to make in isolation.
- `describe_toolset`'s JSON schemas **drift from the real ones** (it reports `path` where the
  function wants `asset_path`, `path` where it wants `root_path`). The reliable way to get a
  true schema is to call the tool with a deliberately wrong argument and read the error.
- `Swarm.DebugShotAfter` latches on `bDebugShotTaken` — **one capture per PIE session**. Several
  timings means several PIE runs, which is what `§5`'s shots are.
- **`docs/AGENT-TEAMS.md` §8a is still live and cost a whole benchmark run here.** The first
  sweep came back a flat `333.334ms / 3.00fps` on every row. Note that
  `ELVTR/Config/DefaultEditorPerProjectUserSettings.ini:9` already says
  `bThrottleCPUWhenNotForeground=False` **and it does not work** — the real setting is
  engine-wide in `%LOCALAPPDATA%\UnrealEngine\5.8\Saved\Config\WindowsEditor\EditorSettings.ini`,
  which says `True`. The project ini line is in the wrong config class and has presumably never
  had any effect. §8a's live-CDO write is what actually works; the sweep now does it itself and
  restores `true` afterwards. **Don't trust the project ini line as evidence the throttle is off.**
- The throttle also silently changes *behaviour*, not just timing: `Volley.CueRate * DeltaSeconds`
  clamps to 1.0 at a 0.333s frame, so a throttled session fires **every** swinging archer **every**
  frame — the saturated case, not the shipped one. An early set of screenshots had to be thrown out
  for this.
- **Leaving the editor launched with `-SwarmBench` poisons every later PIE.** The benchmark stays
  armed, so an ordinary PIE re-runs the whole sweep — including its first config's
  `Volley.Enable 0` — and `ASpike1GameMode` hands population control to it, so the wave sequence
  never runs either. A second set of screenshots was thrown out for this. Relaunch without the
  flag before capturing anything.
