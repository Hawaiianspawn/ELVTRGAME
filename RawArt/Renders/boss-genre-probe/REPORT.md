# Boss genre probe — REPORT

**2026-08-13 · south-only identity probe, second round, gating the
8-direction spend.**

> **OWNER VERDICT 2026-08-13 (covers all rounds to date — genre, chibi,
> twists, horror): APPROVED — spider-broodmother "metal tank",
> **specifically `twists/spider-broodmother/south.png`** (riveted bronze/tan
> orb, glowing amber core — owner confirmed with the image 2026-08-14; the
> horror-v3 ash version is NOT the approved one). DENIED — everything else. Next: fresh
> batch of new concepts anchored on the approved spider recipe (chibi +
> horror wrongness + creature fused with siege-structure).**

- Verdict page (updated in place): https://claude.ai/code/artifact/6b715663-98e9-4232-a38b-8ba57a1e1768
- Predecessor: `RawArt/Renders/boss-marks-proto/REPORT.md` — brood mark-state
  probe, **all denied by owner**, retained.
- Marks system (`docs/design/castle-layout.md` §6.1) stays canon; mark
  visualization restarts once a base identity is approved here.

## Workflow root cause from round one

Round one's failure was the TOOL, not the prompting: `create_character` only
offers fixed template rigs (humanoid + dog/cat/bear/horse/lion quadrupeds) —
no monster anatomy exists in that family, so the dog rig forced every
attempt into a rounded 4-legged mound. This round uses
`create_image_pixflux`: free-form shape, 1 gen/call (~10-40s), full
direction/view/outline/shading control. `create_image_pro` rejected (20-40
gens per call, wrong for one-each probing); `create_image_pixen` rejected
(no outline/shading control).

## Generations

8 of 16 budget, ALL first-try successes, no retries, no policy blocks.
128×128, south-facing, low top-down (game camera), single black outline,
no_background, **no palette or shape constraints** (owner lifted guardrails).

## Measured silhouettes (aspect / solidity / asymmetry, alpha-cropped)

| Boss | Aspect | Solidity | Asymmetry | Read |
|---|---|---|---|---|
| dragon | 1.00 | 0.55 | **0.71** | highest asymmetry — side stance, wings + tail trailing |
| ogre | 0.90 | 0.62 | 0.20 | classic heavy biped |
| spider-broodmother | 1.16 | 0.51 | 0.59 | wide, leggy |
| lich-king | 0.81 | 0.61 | 0.32 | tall narrow |
| armored-titan | 0.98 | 0.66 | **0.00** | perfectly symmetric — frontal immovable block |
| treant | 0.81 | 0.48 | 0.20 | tall, gappy canopy |
| wraith | 0.86 | **0.42** | 0.20 | lowest solidity — genuinely incorporeal read |
| great-worm | **1.24** | 0.56 | 0.47 | only wider-than-tall entry |

Spread: aspect 0.81–1.24, asymmetry 0.00–0.71 — real variety, vs round one
where every mark sat within 0.05 of base on every axis. Direct payoff of
leaving the shared rig.

Files: `{genre}/south.png` per boss + `contact-sheet.png` +
`measurements.json`. Nothing deleted, per retention rule.

## Next steps

1. Owner: Approve/Deny per boss on the verdict page.
2. Approved identities: art-direction pass (palette/register decision now
   that guardrails are off — old demichrome brood register does not bind
   here) and marks-visualization restart on the approved bases.
3. Only then: 8-direction spend per approved boss.
