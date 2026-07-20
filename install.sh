#!/bin/sh
# install.sh - one-shot deploy of the current RIST-capture iteration.
#
# REF policy: defaults to the working branch, which auto-tracks every pushed
# iteration, so re-running the SAME curl one-liner always installs the latest.
# To pin a specific drop instead, override:  REF=iter1 curl -fsSL .../install.sh | sh
#
# Usage:   curl -fsSL <raw-url>/install.sh | sh
#   env overrides:  SDK_ROOT=...  REF=...  ENV_SH=...
#
# Plain POSIX sh (busybox/dash safe). Needs only curl, make, coreutils and the
# toolchain already on the VM. No git, no package installs.

set -eu

# ============================================================================
# CONFIG  -- future iterations: usually only FILES / OBJS need editing.
# ============================================================================
RAW_BASE="https://raw.githubusercontent.com/caritechsolutions/pacman"
REF="${REF:-claude/gx6631-ts-userspace-rist-fe2wkc}"
REPO_PREFIX="6631SDK/solution"                        # repo path that maps to $SDK_ROOT
SDK_ROOT="${SDK_ROOT:-/opt/stb/sdk-clean/6631SDK/solution}"
ENV_SH="${ENV_SH:-/opt/stb/env.sh}"

# Files changed by THIS iteration, as paths relative to $SDK_ROOT
# (each is identical to its path under $REPO_PREFIX in the repo).
FILES="app/module/app_rist_capture.c
app/module/app_play_control.c"

# Objects to force-rebuild so we never ship a stale out.elf.
OBJS="output/objects/app_rist_capture.o
output/objects/app_play_control.o"

# Marker used to detect an already-hooked (hand-edited) app_play_control.c.
HOOK_MARKER="app_rist_play_change"

# ============================================================================
TS="$(date +%Y%m%d-%H%M%S)"
MAKE_LOG="/tmp/rist_make.log"
MAKEBIN_LOG="/tmp/rist_makebin.log"

log() { printf '%s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

# ---- 1. sanity -------------------------------------------------------------
command -v curl >/dev/null 2>&1 || die "curl not found on PATH"
command -v make >/dev/null 2>&1 || die "make not found on PATH"
[ -d "$SDK_ROOT" ] || die "SDK_ROOT is not a directory: $SDK_ROOT   (set SDK_ROOT=...)"
[ -f "$ENV_SH" ]   || die "env script not found: $ENV_SH   (set ENV_SH=...)"

log "=== RIST capture install ==="
log "  REF      : $REF"
log "  SDK_ROOT : $SDK_ROOT"
log "  raw base : $RAW_BASE"
log ""

TMP="$(mktemp -d "${TMPDIR:-/tmp}/ristinstall.XXXXXX")" || die "mktemp -d failed"
trap 'rm -rf "$TMP"' EXIT INT TERM
REPORT="$TMP/report"
: > "$REPORT"
HOOK_STATUS="n/a"
STAGED=""

# ---- 2. download everything to temp FIRST (never half-install) -------------
for rel in $FILES; do
    dest="$SDK_ROOT/$rel"
    url="$RAW_BASE/$REF/$REPO_PREFIX/$rel"
    base="$(basename "$rel")"

    # Protect the hand-edited hook host: if the hook is already present, leave
    # the VM's file completely alone (it may carry your other local edits).
    if [ "$base" = "app_play_control.c" ]; then
        if [ -f "$dest" ] && grep -q "$HOOK_MARKER" "$dest" 2>/dev/null; then
            HOOK_STATUS="already present -> left your $base untouched"
            log "hook: '$HOOK_MARKER' already in $base -> skipping download (your edits preserved)"
            continue
        fi
        HOOK_STATUS="was missing -> installed repo's hooked $base (your copy backed up)"
        log "hook: '$HOOK_MARKER' NOT found in $base -> will install repo's hooked copy"
    fi

    mkdir -p "$TMP/$(dirname "$rel")"
    log "download: $url"
    curl -fsSL "$url" -o "$TMP/$rel" || die "download failed (404 or network): $url"
    [ -s "$TMP/$rel" ] || die "downloaded an empty file: $url"
    STAGED="$STAGED$rel
"
done

# ---- 3. install staged files (back up anything we overwrite) ---------------
log ""
log "installing:"
for rel in $STAGED; do
    dest="$SDK_ROOT/$rel"
    mkdir -p "$(dirname "$dest")"
    if [ -f "$dest" ]; then
        cp -p "$dest" "$dest.bak.$TS"
        log "  backup : $dest -> $dest.bak.$TS"
    fi
    cp "$TMP/$rel" "$dest"
    sz="$(wc -c < "$dest" | tr -d ' ')"
    log "  write  : $dest ($sz bytes)"
    printf '%s  (%s bytes)\n' "$rel" "$sz" >> "$REPORT"
done
[ -s "$REPORT" ] || log "  (no files needed installing)"

# ---- 4. force object rebuild ----------------------------------------------
log ""
log "forcing object rebuild:"
for o in $OBJS; do
    if [ -f "$SDK_ROOT/$o" ]; then
        rm -f "$SDK_ROOT/$o"
        log "  rm $o"
    else
        log "  (absent) $o"
    fi
done

# ---- 5. build (make, then make bin; stop hard if make fails) ---------------
log ""
log "=== build: make ==="
cd "$SDK_ROOT"
set +e; . "$ENV_SH"; set -e     # sourcing env scripts can return non-zero benignly

{ make 2>&1; echo $? > "$TMP/make.rc"; } | tee "$MAKE_LOG" | grep -iE 'rist_capture|app ok|error' || true
mk="$(cat "$TMP/make.rc" 2>/dev/null || echo 1)"
if [ "$mk" != "0" ]; then
    log ""
    log "---- last 25 lines of $MAKE_LOG ----"
    tail -n 25 "$MAKE_LOG" 2>/dev/null || true
    die "'make' failed (rc=$mk) -- NOT running 'make bin'. Full log: $MAKE_LOG"
fi

log ""
log "=== build: make bin ==="
{ make bin 2>&1; echo $? > "$TMP/makebin.rc"; } | tee "$MAKEBIN_LOG" | grep -iE '\.boot|error' || true
mb="$(cat "$TMP/makebin.rc" 2>/dev/null || echo 1)"
if [ "$mb" != "0" ]; then
    log ""
    log "---- last 25 lines of $MAKEBIN_LOG ----"
    tail -n 25 "$MAKEBIN_LOG" 2>/dev/null || true
    die "'make bin' failed (rc=$mb). Full log: $MAKEBIN_LOG"
fi

# ---- 6. summary ------------------------------------------------------------
IMG="$SDK_ROOT/output/image/download_linux.bin"
log ""
log "======================= SUMMARY ======================="
log "REF                        : $REF"
log "hook (app_play_control.c)  : $HOOK_STATUS"
log "files installed            :"
if [ -s "$REPORT" ]; then
    while IFS= read -r line; do log "    $line"; done < "$REPORT"
else
    log "    (none - play_control hook already present, nothing else changed)"
fi
log ""
if [ -f "$IMG" ]; then
    isz="$(wc -c < "$IMG" | tr -d ' ')"
    its="$(date -r "$IMG" '+%Y-%m-%d %H:%M:%S' 2>/dev/null \
           || ls -l "$IMG" 2>/dev/null | awk '{print $6, $7, $8}' \
           || echo '?')"
    log "image  : $IMG"
    log "  size : $isz bytes"
    log "  time : $its"
else
    log "image  : $IMG   (NOT FOUND - did 'make bin' succeed?)"
fi
log ""
log "next:"
log "  1) flash $IMG to the box"
log "  2) zap a channel (Iter 1 auto-arms the capture on zap; the /tmp/ristcap"
log "     marker-input file is reserved for Iter 2, not needed yet)"
log "  3) on the serial console watch for:  [RIST] reader: FIRST read = N bytes"
log "     then the per-second byte counter climbing (not 'ZERO PAYLOAD'), and"
log "     /media/sda1/rist_dump.ts growing with real video+audio ES."
log "======================================================="
