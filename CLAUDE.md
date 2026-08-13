# ELVTR / Kindled

## Commit messages

Use the `caveman-commit` skill for every commit on this repo — including commits
made by subagents and orchestrated task dispatch, with two overrides:

- **No Conventional Commits type prefix.** Keep this repo's sentence subjects:
  one line, imperative, sentence case, no trailing period, ≤80 chars.
- **Body ≤250 chars**, and only when the *why* isn't obvious from the subject.
  Wrap at 72. Bullets `-` not `*`. No "this commit does X", no restating
  filenames, no `I`/`we`. Trailers and `Refs`/`Closes` lines don't count.

Keep the `Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>` trailer.

Always include a body for: breaking changes, save-format/data migrations, and
anything reverting a prior commit. If 250 chars genuinely can't hold the *why*,
write the detail in the task file or a doc and reference it — don't overrun.
