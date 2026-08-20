# Boss marks visualization probe — REPORT

**2026-08-13 · south-only readability probe gating the 8-direction spend.**

> **OWNER VERDICT 2026-08-13: ALL DENIED.** No image from this probe is
> viable. Direction pivoted to a multi-genre boss identity probe (dragon,
> ogre, etc.), palette/shape guardrails lifted — see
> `RawArt/Renders/boss-genre-probe/`. Images retained per retention rule.
> Marks system remains canon; mark VISUALIZATION restarts once a base boss
> identity is approved.

- Verdict page: https://claude.ai/code/artifact/6b715663-98e9-4232-a38b-8ba57a1e1768
- Spec (landed mid-run, generations predate it): `docs/art/boss-marks-prototype.md`
- Request: `docs/data/art/requests/brood-boss-marks-prototype.json`
- Canon marks source: `docs/design/castle-layout.md` §6.1

## Generations

10 of 12 budget (subscription credits only). Base + 6 mark states + 1 failed
Ram retry + 1 compound + 1 successful reworded Ram retry. Probe cell 64px; production is spec-locked to 48px —
contact sheet carries a 48px nearest-neighbor row for true-size judging.

| Image | PixelLab id | Files |
|---|---|---|
| base | 99e40da9-3862-4574-b95f-defe016fdb79 | `raw/base/rotations/` |
| quilled | dd383c5c-4fca-4af0-9838-30c4f073a5b8 | `raw/quilled/rotations/` |
| ram (FAILED, retained) | 8453d00c-2f71-4a8a-a7ce-ef95f7c21bfc | `raw/ram/rotations/` |
| ram retry (SUCCESS) | 51246cd8-19c3-4f68-978d-fca2b4c2696c | `raw/ram-retry2/rotations/` |
| sated | 8dbf0b2d-ccd9-4f93-a7d4-47c534a22ef4 | `raw/sated/rotations/` |
| wearing | cc2dd96a-0f6b-4873-a821-6c4de30439ee | `raw/wearing/rotations/` |
| unblinded | 3c119606-95ce-462c-9a39-304f31e46de4 | `raw/unblinded/rotations/` |
| column-fed | e314dd5d-5776-43ce-8afe-1fe79f0bcd11 | `raw/column-fed/rotations/` |
| compound (Ram+Column-fed+Quilled) | see verdict page | proto folder |

Nothing deleted, per retention rule.

## Measured outline distinguishability (south, vs base 0.95 / 0.69 / 0.00 aspect/solidity/asymmetry)

| Mark | Aspect Δ | Solidity Δ | Asymmetry Δ | Reads by outline? | Spec zone match |
|---|---|---|---|---|---|
| Quilled | +0.12 | −0.04 | **+0.24** | YES | ~zone E flank, slightly high |
| Ram (retry) | **+0.25** | −0.01 | +0.09 | YES — largest aspect change of set; slab centered, so asymmetry low | zone B exact, incl. the "only permitted straight edge" rule |
| Sated | +0.00 | +0.02 | +0.00 | NO — interior/color only | **COLLISION** — swelled the crown (zone A) instead of dorsal (zone D) |
| Wearing | +0.08 | +0.01 | **+0.16** | YES | ~zone C shoulder |
| Unblinded | +0.00 | +0.00 | +0.00 | NO — pure recolor | zone A placement OK |
| Column-fed | +0.14 | −0.02 | **+0.24** | YES | zone F exact |
| Compound | | | **+0.31** | YES — all three deltas read at once | matches spec's chosen three-stack |

Compound asymmetry (0.31) exceeds the best solo mark (0.25): outline
complexity compounds — supports castle-layout §6.1's marks-compound claim.

## Failures and divergences

- **Ram**: first gen drew a literal gate-grille filling the canvas (solidity
  0.99, not a creature); first retry blocked by PixelLab content policy on
  "battering ram"/"breaking" wording. Reworded prop-carrying prompt ("carries
  a massive splintered door slab held flat against its front") succeeded —
  policy blocks avoided by describing carried debris, not weapons/destruction.
- **PixelLab process note**: a job that shows "stuck" in `list_jobs` may still
  be live — one live job was cancelled by mistake at ~20 min (it requeued, no
  loss). Confirm `get_character` shows a terminal status before `cancel_job`.
- **Base diverges from spec**: (1) carries two dark almond eye-shapes — banned
  by both the family register and the spec; (2) rounded narrow-based mound,
  closer to the elite's "rears" language than the spec's low/wide/jagged
  six-stub base. Dog-template 4-leg limit expected; proportion is a real issue.
- **Sated/Unblinded zone collision traces to the base**: no dorsal ridge on
  the base, so "swollen" enlarged the crown mark into Unblinded's zone. The
  spec's §1 dorsal ridge ("unmarked at rest... the site Sated will swell")
  would fix both. **Single recommended fix before any further spend.**
- Spec's harder stress-stack (Sated+Wearing+Unblinded, all upper-body) is
  untested — deliberately deferred until the base has a dorsal ridge.
- Register tensions resolved as: Sated = enlarged matte pale patch (no glow);
  Unblinded = two fixed pale marks (deliberate eyes-without-face exception).
  Matches the spec's independent rulings.

## Next steps (pending owner verdicts on the page)

1. Owner: approve/deny per mark on the verdict page.
2. Regenerate base with dorsal ridge + no eyes + wider/lower stance (spec §1).
3. Sated/Unblinded need actual outline protrusions if silhouette-only
   readability is a hard requirement; Unblinded may be acceptable as the one
   color-read mark — owner call.
4. Only after the above: the 8-direction spend.
