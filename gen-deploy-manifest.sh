#!/bin/sh
# gen-deploy-manifest.sh - regenerate deploy.manifest from git.
#
# WHY: install.sh's FILES list is hand-maintained, and twice now a changed file
# was left out of it -- app_rist_capture.c (stale source silently used) and
# full_screen.h (build failure). This derives the truth from git so install.sh
# can cross-check FILES against it and fail loudly instead of building something
# subtly wrong.
#
# RUN THIS BEFORE EVERY PUSH that touches app sources, then commit the result:
#     ./gen-deploy-manifest.sh && git add deploy.manifest
#
# The manifest lists every file this branch changes under solution/app, with
# paths relative to $SDK_ROOT (i.e. the same form install.sh's FILES uses).

set -eu

BASE="${BASE:-origin/main}"
PREFIX="6631SDK/solution/"
OUT="deploy.manifest"

command -v git >/dev/null 2>&1 || { echo "ERROR: git not found" >&2; exit 1; }
git rev-parse --git-dir >/dev/null 2>&1 || { echo "ERROR: not a git repo" >&2; exit 1; }

git rev-parse --verify "$BASE" >/dev/null 2>&1 || {
    echo "ERROR: base ref '$BASE' not found (try: git fetch origin main)" >&2
    exit 1
}

git diff --name-only "$BASE"...HEAD \
    | grep "^${PREFIX}app/" \
    | sed "s|^${PREFIX}||" \
    | sort -u > "$OUT"

n="$(wc -l < "$OUT" | tr -d ' ')"
echo "deploy.manifest: $n file(s) changed vs $BASE"
sed 's/^/  /' "$OUT"
