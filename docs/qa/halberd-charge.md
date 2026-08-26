# Halberdier charge: synchronized arrival + arrival blow

task-165. Change is in `godot/scripts/Unit.gd` only (`charge()`, `_process`'s charge
branch, new `_deliver_blow()`). Evidence gathered with a temporary
`print("CHARGE_ARRIVE ", get_instance_id(), " ", Time.get_ticks_msec())` at the frame
each charger reaches `charge_to`, removed after capture. Both runs: the console Godot
binary directly (`--path godot -- --probe=battle,20,charge`), which cycles the army to
halberdier ~6s in and lets all 8 rows (48 halberdiers, `CHARGE_GAP`-staggered start
depths) charge to the pile line.

## Before (unsynced: each charger ran at its own `speed * CHARGE_SPEED`)

48 arrivals span **7639 -> 8765 ms, a 1126 ms spread**, in 8 visible clusters of 6 (one
per row — each row's own 6 units start at the same depth so they already arrive
together; the *rows* did not):

```
CHARGE_ARRIVE 60850964693 7639
CHARGE_ARRIVE 60918073553 7644
CHARGE_ARRIVE 60985182318 7648
CHARGE_ARRIVE 61052291305 7653
CHARGE_ARRIVE 61119400168 7657
CHARGE_ARRIVE 61186509000 7661
CHARGE_ARRIVE 61253617860 7797
CHARGE_ARRIVE 61320726720 7802
CHARGE_ARRIVE 61387835621 7807
CHARGE_ARRIVE 61454944404 7811
CHARGE_ARRIVE 61522053264 7816
CHARGE_ARRIVE 61589162124 7820
CHARGE_ARRIVE 61656271066 7960
CHARGE_ARRIVE 61723379833 7964
CHARGE_ARRIVE 61790488693 7967
CHARGE_ARRIVE 61857597645 7969
CHARGE_ARRIVE 61924706591 7972
CHARGE_ARRIVE 61991815459 7974
CHARGE_ARRIVE 62058924327 8114
CHARGE_ARRIVE 62126033195 8117
CHARGE_ARRIVE 62193142063 8119
CHARGE_ARRIVE 62260250931 8121
CHARGE_ARRIVE 62327359799 8124
CHARGE_ARRIVE 62394468667 8126
CHARGE_ARRIVE 62461577535 8268
CHARGE_ARRIVE 62528686403 8270
CHARGE_ARRIVE 62595795271 8272
CHARGE_ARRIVE 62662904139 8275
CHARGE_ARRIVE 62730013007 8277
CHARGE_ARRIVE 62797121875 8280
CHARGE_ARRIVE 62864230743 8436
CHARGE_ARRIVE 62931339611 8439
CHARGE_ARRIVE 62998448479 8441
CHARGE_ARRIVE 63065557347 8443
CHARGE_ARRIVE 63132666215 8445
CHARGE_ARRIVE 63199775083 8449
CHARGE_ARRIVE 63266883951 8589
CHARGE_ARRIVE 63333992819 8591
CHARGE_ARRIVE 63401101687 8594
CHARGE_ARRIVE 63468210555 8596
CHARGE_ARRIVE 63535319423 8598
CHARGE_ARRIVE 63602428291 8600
CHARGE_ARRIVE 63669537159 8753
CHARGE_ARRIVE 63736646027 8755
CHARGE_ARRIVE 63803754895 8758
CHARGE_ARRIVE 63870863763 8760
CHARGE_ARRIVE 63937972631 8763
CHARGE_ARRIVE 64005081499 8765
PROBE charge ok: 48 charged, all home
```

## After (synced: shared lead + `T`, each charger lerps its own start->`charge_to` over `T`)

Same probe, same scenario. 48 arrivals span **7611 -> 7717 ms, a 106 ms spread** — no
row clustering left, one continuous near-simultaneous block:

```
CHARGE_ARRIVE 62662903996 7611
CHARGE_ARRIVE 62730012870 7613
CHARGE_ARRIVE 62797121730 7616
CHARGE_ARRIVE 62864230590 7618
CHARGE_ARRIVE 62931339450 7621
CHARGE_ARRIVE 62998448307 7623
CHARGE_ARRIVE 63065557167 7625
CHARGE_ARRIVE 63132666027 7627
CHARGE_ARRIVE 63199774886 7630
CHARGE_ARRIVE 63266883961 7632
CHARGE_ARRIVE 63333992829 7634
CHARGE_ARRIVE 63401101697 7636
CHARGE_ARRIVE 63468210565 7639
CHARGE_ARRIVE 63535319433 7641
CHARGE_ARRIVE 63602428301 7643
CHARGE_ARRIVE 63669537169 7645
CHARGE_ARRIVE 63736646037 7647
CHARGE_ARRIVE 63803754905 7649
CHARGE_ARRIVE 63870863773 7652
CHARGE_ARRIVE 63937972641 7654
CHARGE_ARRIVE 64005081509 7657
CHARGE_ARRIVE 64072190377 7659
CHARGE_ARRIVE 64139299245 7661
CHARGE_ARRIVE 64206408113 7664
CHARGE_ARRIVE 64273516981 7666
CHARGE_ARRIVE 64340625849 7668
CHARGE_ARRIVE 64407734717 7670
CHARGE_ARRIVE 64474843585 7673
CHARGE_ARRIVE 64541952453 7675
CHARGE_ARRIVE 64609061321 7677
CHARGE_ARRIVE 64676170189 7679
CHARGE_ARRIVE 64743279057 7681
CHARGE_ARRIVE 64810387925 7683
CHARGE_ARRIVE 64877496793 7686
CHARGE_ARRIVE 64944605661 7688
CHARGE_ARRIVE 65011714529 7691
CHARGE_ARRIVE 65078823397 7693
CHARGE_ARRIVE 65145932265 7695
CHARGE_ARRIVE 65213041133 7697
CHARGE_ARRIVE 65280150001 7700
CHARGE_ARRIVE 65347258869 7702
CHARGE_ARRIVE 65414367737 7704
CHARGE_ARRIVE 65481476605 7706
CHARGE_ARRIVE 65548585473 7708
CHARGE_ARRIVE 65615694341 7711
CHARGE_ARRIVE 65682803209 7713
CHARGE_ARRIVE 65749912077 7715
CHARGE_ARRIVE 65817020945 7717
PROBE charge ok: 48 charged, all home
```

1126 ms -> 106 ms (~11x tighter); the remaining spread is per-frame jitter across the
~500 fps this probe ran at (48 units resolving their own `_process` in the same frame
window), not row staggering.

## Screenshot v1 (rejected): `halberd-charge.png` original capture

Captured with a temporary post-arrival hook fired ~0.15s after the synced arrival
frame. At the original halberdier speed the out-leg/blow/back-leg resolved in well
under 0.15s, so the block was already back in rank and the throw was a same-frame
`wd` tween — nothing visibly flew. Owner verdict: "throw not visible, enemies should
fly." Sync (106ms) was kept; the throw and screenshot were redone below.

## Revision 2: real launch arc + a freeze at the line

`_deliver_blow()` no longer tweens `o.wd` directly. It now calls the existing
`launch()` (used everywhere else airborne units get juggled/thrown), extended with an
optional depth-velocity `vd` param and a matching `air_vd` field that `_process`'s
airborne branch advances (`wd += air_vd * delta`, mirroring the existing `air_vx`
lateral drift). Impulse `v` scales 220-320 with throw distance (farther = bigger, so
early-hit enemies arc higher and land farther, fanning the pile out); `vd` is aimed at
landing on the same `charge_to + (charge_to - hit_wd)` depth using the hang time from
`GRAV`'s own "peaks at V, hangs ~2V/GRAV" relationship (see the `GRAV` constant's
comment) — an aim, not exact, since it ignores the floaty apex easing (`FLOAT_G`);
fine for a spectacle throw. `_charge_thrown`/`rooted_until` now clear on the real
landing frame (`air_h == 0.0`), not a tween callback.

Chargers also now hold at `charge_to` for `CHARGE_HOLD = 0.15s` (`_charge_hold_until`)
before the fall-back leg starts, so the block is still standing at the line at the
instant the blow launches instead of already peeling off.

Re-ran the same `--probe=battle,12,charge` scenario after the fix (fresh run, not the
same numbers as above — run-to-run timing jitters a few ms, still the same story):
48 arrivals span **7556 -> 7666 ms, a 110 ms spread** — sync unaffected by the throw
rework, as expected (the hold/throw both happen strictly after arrival).

```
CHARGE_ARRIVE 71286393313 7556 hits=0
CHARGE_ARRIVE 71353502181 7559 hits=0
CHARGE_ARRIVE 71420611049 7561 hits=3
CHARGE_ARRIVE 71487719917 7564 hits=5
CHARGE_ARRIVE 71554828785 7566 hits=1
CHARGE_ARRIVE 71621937653 7568 hits=3
... (42 more, 7570 -> 7666, hits=0: no enemies in reach for the back 7 rows this run)
PROBE charge ok: 48 charged, all home
```

## Screenshot: `halberd-charge.png` (current)

Captured with a temporary hook: `get_tree().create_timer(0.6).timeout` ->
`get_viewport().get_texture().get_image().save_png(...)`, fired 0.6s after the first
qualifying charger's arrival (long enough into the now multi-second hang time —
`GRAV=125` means a 220-320 impulse hangs ~3.5-5s — to be clearly mid-arc, well before
`PILE_HOLD` landing settles anything). Both the print and the hook were removed after
capture. The shot shows several green-tinted enemy sprites clearly airborne above and
ahead of the halberdier rank line, at different heights/depths rather than one flat
pile — the fan-out from scaling impulse by throw distance is visible.

## Adversary check

`pwsh Scripts\godot-run.ps1 -Adversary battle,30,7`, re-run against revision 2:
**0 findings, 0 engine errors.** Earlier (revision 1) this same command produced 1
finding both with and without this task's change — `boundary_break @Battle.depth: unit
depth -1 outside 0..1250` — isolated by stashing `Unit.gd` and confirming it reproduced
against unmodified code too. That's a halberdier's staged charge-in depth
(`BEHIND_D - CHARGE_GAP*1 == -1`, from `Battle.gd`'s deploy code, not touched here), not
something introduced by this task; it appears to be sampled only sometimes (this run
didn't trigger it either way). No new findings from either revision.
