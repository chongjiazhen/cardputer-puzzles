# patches/

Local fixes carried against the vendored `upstream/` submodule (Simon Tatham's
Portable Puzzle Collection), applied at build time by `apply-patches.py`
(PlatformIO `pre:` hook) — the submodule is never edited in place and its pinned
SHA never moves.

Only small-screen rendering fixes live here: things that are real defects on the
240×135 Cardputer but not on desktop, so upstream has no reason to carry them.
Never patch game *rules/logic* — a rule bug belongs upstream.

## Applied patches

- **`dominosa-min-gutter.patch`** — `DOMINO_GUTTER = max(1, TILESIZE/16)`.
  The board fits the screen at `TILESIZE≈15`, where `TILESIZE/16` integer-divides
  to **0** → adjacent domino bodies (pure black) touch → the whole board reads as
  one connected blob. Floor the gutter at 1px so the inter-domino gap survives.
  Upstream runs `TILESIZE≥32` (gutter ≥2), so it never sees this.

## Adding a patch

1. Edit the file under `upstream/`, then `cd upstream && git diff <file> > ../patches/<name>.patch`.
2. Revert the submodule working tree: `git checkout <file>` (keep it clean).
3. Document it above (what + why + why-not-upstream).

The hook is idempotent: an already-applied patch is skipped; a conflicting one
fails the build loudly.
