Import("env")
import os
import subprocess

# Pre-build hook: apply every patches/*.patch onto the pristine upstream/ submodule.
#
# Why patches instead of editing the submodule: upstream (Simon Tatham's puzzles)
# is vendored unmodified and its pinned SHA must stay clean (see CLAUDE.md). The
# only local changes we carry are small-screen rendering fixes that upstream has
# no reason to take (desktop runs TILESIZE>=32 where they don't bite). Each such
# change lives as an explicit, reviewable diff in patches/ and is re-applied here
# at build time -- the submodule *pointer* never moves.
#
# Idempotent: if a patch already applies in reverse (i.e. is present), skip it, so
# repeated / incremental builds don't fail on an already-patched tree. A patch that
# neither reverses nor forward-applies (conflict) fails the build loudly.

project_dir = env["PROJECT_DIR"]
patches_dir = os.path.join(project_dir, "patches")
upstream_dir = os.path.join(project_dir, "upstream")

if not os.path.isdir(patches_dir):
    print("[patches] no patches/ dir; nothing to apply")
else:
    for name in sorted(os.listdir(patches_dir)):
        if not name.endswith(".patch"):
            continue
        patch = os.path.join(patches_dir, name)
        already = subprocess.run(
            ["git", "apply", "--reverse", "--check", patch],
            cwd=upstream_dir, capture_output=True,
        )
        if already.returncode == 0:
            print("[patches] %s: already applied" % name)
            continue
        result = subprocess.run(
            ["git", "apply", patch],
            cwd=upstream_dir, capture_output=True, text=True,
        )
        if result.returncode != 0:
            print("[patches] %s: FAILED to apply\n%s" % (name, result.stderr))
            env.Exit(1)
        print("[patches] %s: applied" % name)
