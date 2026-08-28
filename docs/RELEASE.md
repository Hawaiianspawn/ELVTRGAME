# Releasing Kindled to itch.io

## Build then push

```powershell
powershell -File Scripts\release.ps1
powershell -File Scripts\itch-push.ps1
```

`release.ps1` produces `build\web\index.html` (and friends). `itch-push.ps1`
reads the game version out of `godot\scripts\Game.gd` (`const VERSION := "..."`,
falls back to `0.1.0` with a warning if that constant doesn't exist yet) and
runs `butler push build\web hawaiianspawn/kindled:html5 --userversion <ver>`.

Add `-DryRun` to list what would be pushed without pushing. Override the
itch user or channel with `-ItchUser` / `-Channel`.

## itch.io project settings (one-time)

On the kindled project's edit page:

- **Kind of project:** HTML
- **Embed options:** 960x540, "Click to launch in fullscreen" enabled
- **SharedArrayBuffer support:** leave OFF — the export is built with
  `variant/thread_support=false`, so the page needs no `Cross-Origin-Opener-Policy`
  / `Cross-Origin-Embedder-Policy` headers, which itch.io's static host doesn't
  send. Turning this on for a non-threaded build does nothing useful.
- Mark the HTML file as the one to embed (butler push uploads the whole
  `build\web` folder; itch auto-detects `index.html` as the launch target).

## API key

`itch-push.ps1` reads `$env:ITCH_API_KEY`. Get one at
https://itch.io/user/settings/api-keys and set it in your shell before
pushing:

```powershell
$env:ITCH_API_KEY = '...'
```

The script copies it into `BUTLER_API_KEY`, which is what butler itself
reads. Without it set, the script prints where to get a key and exits 1 —
it never prompts or pushes silently.

## Why `build/` is gitignored

`build\web` is a generated export (tens of MB, mostly `index.wasm`). It's
rebuilt by `release.ps1` on demand and pushed straight to itch — there's no
reason to carry it in git history.
