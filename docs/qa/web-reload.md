# Web reload/resume check (task-177)

Question: does closing the tab and opening the same origin again land on the
title card or mid-run?

## Procedure as performed

1. **Serve.** `py -m http.server -d build/web 8765` (build already exported,
   `build/web/index.html` present — no re-export run).
2. **Clear origin storage before first load.** New Chrome tab, navigated to
   `http://localhost:8765`. `indexedDB.databases()` returned `["/userfs"]`
   (leftover from earlier testing); deleted it, then reloaded. Screenshot
   confirmed a clean title card (`KINDLED / The Green Dot / v0.1.0`, hero:
   knight, `[Space] begin`).
3. **Play for real.** Clicked the page, pressed Space. Within ~5s wall-clock
   the HUD already read `HALL 2 / 4` — hall progression in this build is
   sim-driven and advances fast regardless of player input intensity, so the
   ~5-10 min estimate in the task brief didn't hold; continuing wasn't needed
   to reach the target state, but real WASD/LMB input was driven anyway
   (three rounds of movement keys + clicks with `browser_batch`) to build up
   genuine mid-run state before the reload test. Ended at `HALL 3 / 4`,
   `MAGIC 1992 (lifetime 2008)`, retinue at x140 per unit type, `FPS 20`.
   - `document.visibilityState`/`hidden` checked mid-play: `visible`/`false`.
   - Checked again immediately after the play batch's final screenshot:
     `hidden`/`true`. FPS was 20 (not single-digit) in the screenshot taken
     one step earlier, so this reads as an artifact of the extension's tab
     focus moving away after the last automated action, not the game
     genuinely freezing mid-battle. No backgrounding recovery step was
     needed as a result (the single-digit-FPS trigger condition wasn't met).
4. **Reload.** Closed that tab (`tabs_close_mcp`, storage untouched).
   Opened a new tab on `http://localhost:8765`, waited 10s, screenshot.
   **Result: title card**, not `HALL n`. See `docs/qa/web-reload.png`.
5. **Post-reload state.** `indexedDB.databases()` on the new tab: still
   `[{"name":"/userfs","version":21}]` — the database itself persists (it's
   Godot's Emscripten IDBFS mount for the HTML5 export, auto-created by the
   engine runtime on boot) but nothing in it is read back into the title
   screen. `grep -rn "user://" godot/scripts` hits only:
   - `godot/scripts/Adversary.gd:4,268,284` — QA/adversary harness
     (`user://adversary_<seed>.json/.csv`, debug screenshots).
   - `godot/scripts/Probe.gd:3,195` — FPS/screenshot probe tool
     (`user://probe_<scene>*.png`).

   `Game.gd` never touches `user://`, confirming the pre-verified premise.

## Conclusion

Closing the tab and reopening the same origin lands on the **title card**,
not mid-run — it does not reproduce a resume. This matches the premise:
`progressive_web_app/enabled=false` in `godot/export_presets.cfg` means no
service worker to intercept navigation or cache app state, and the only code
that writes `user://` is QA tooling (`Adversary.gd`, `Probe.gd`), never
`Game.gd`. The `/userfs` IndexedDB database that survives the reload is
Godot's default HTML5-export virtual filesystem mount, created empty by the
engine on every boot regardless of whether anything is ever written to it —
its mere presence is not evidence of persisted game state. No fix is needed
today; the only thing worth flagging is that if a future feature (autosave,
settings, unlocks) starts writing `user://` from `Game.gd`, it will need to
explicitly gate any resume-on-load behavior rather than relying on the
IDBFS mount staying empty by accident.
