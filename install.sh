#!/bin/sh
# install.sh - enable + deploy the dvb2ip HTTP-TS server.
#
# WHY THIS EXISTS: the userspace-decrypt RIST capture is a dead end (the M2M
# cipher panics the kernel on app-owned buffers). We pivot to dvb2ip, a shipping
# SDK feature that captures the selected program THROUGH the hardware demux
# descrambler -> CLEAR TS in a DVR MEM buffer -> served over HTTP. No decrypt.
#
#   playlist :  http://<stb-ip>:8998/play_file
#   stream   :  http://<stb-ip>:8999/stream=<prog_id>.ts
#
# dvb2ip is compiled out by default (DVB2IP_SERVER_SUPPORT 0). This script:
#   1. installs the changed sources (diagnostics + disabled RIST hook),
#   2. flips BR2_MOD_DVB2IP_SERVER=y in .config,
#   3. FORCES A FULL APP REBUILD -- mandatory: enabling the macro inserts two
#      values into the GXMSG_* enum (app_msg.h), renumbering every later message
#      id, so any stale object would mis-dispatch messages. A partial rebuild is
#      NOT safe here.
#
# REF policy: defaults to the working branch (auto-tracks each pushed iteration).
# Usage:   curl -fsSL <raw-url>/install.sh | sh
#   env overrides:  SDK_ROOT=...  REF=...  ENV_SH=...
#
# Plain POSIX sh (busybox/dash safe). Needs curl, make, sed, coreutils + toolchain.

set -eu

# ============================================================================
RAW_BASE="https://raw.githubusercontent.com/caritechsolutions/pacman"
REF="${REF:-claude/gx6631-ts-userspace-rist-fe2wkc}"
REPO_PREFIX="6631SDK/solution"                        # repo path that maps to $SDK_ROOT
SDK_ROOT="${SDK_ROOT:-/opt/stb/sdk-clean/6631SDK/solution}"
ENV_SH="${ENV_SH:-/opt/stb/env.sh}"

# Source files changed by THIS iteration (paths relative to $SDK_ROOT, identical
# to their path under $REPO_PREFIX in the repo).
#   - app_dvb2ip_platform.c : always-on serial diagnostics on the start path.
#   - app_play_control.c    : RIST capture hook DISABLED (it panics + would fight
#                             dvb2ip for the single DVR/TSW hardware).
FILES="app/dvb2ip_server/app_dvb2ip_platform.c
app/dvb2ip_server/app_ts_record.c
app/module/app_play_control.c"

# Objects for those sources (flat basename.o), used for the fast INCREMENTAL path
# once dvb2ip is already enabled.
OBJS="output/objects/app_dvb2ip_platform.o
output/objects/app_ts_record.o
output/objects/app_play_control.o"

CONFIG_OPT="BR2_MOD_DVB2IP_SERVER"                    # Kconfig symbol to enable

# ============================================================================
TS="$(date +%Y%m%d-%H%M%S)"
MAKE_LOG="/tmp/dvb2ip_make.log"
MAKEBIN_LOG="/tmp/dvb2ip_makebin.log"

log() { printf '%s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

# ---- 1. sanity -------------------------------------------------------------
command -v curl >/dev/null 2>&1 || die "curl not found on PATH"
command -v make >/dev/null 2>&1 || die "make not found on PATH"
command -v sed  >/dev/null 2>&1 || die "sed not found on PATH"
[ -d "$SDK_ROOT" ]        || die "SDK_ROOT is not a directory: $SDK_ROOT   (set SDK_ROOT=...)"
[ -f "$ENV_SH" ]          || die "env script not found: $ENV_SH   (set ENV_SH=...)"
[ -f "$SDK_ROOT/.config" ]|| die ".config not found in $SDK_ROOT (is this the solution dir?)"

log "=== dvb2ip enable + install ==="
log "  REF      : $REF"
log "  SDK_ROOT : $SDK_ROOT"
log ""

TMP="$(mktemp -d "${TMPDIR:-/tmp}/dvb2ipinstall.XXXXXX")" || die "mktemp -d failed"
trap 'rm -rf "$TMP"' EXIT INT TERM
REPORT="$TMP/report"
: > "$REPORT"

# ---- 2. download changed sources to temp FIRST (never half-install) --------
for rel in $FILES; do
    url="$RAW_BASE/$REF/$REPO_PREFIX/$rel"
    mkdir -p "$TMP/$(dirname "$rel")"
    log "download: $url"
    curl -fsSL "$url" -o "$TMP/$rel" || die "download failed (404 or network): $url"
    [ -s "$TMP/$rel" ] || die "downloaded an empty file: $url"
done

# ---- 3. install staged files (back up anything we overwrite) ---------------
log ""
log "installing sources:"
for rel in $FILES; do
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

# ---- 4. enable dvb2ip in .config (idempotent) ------------------------------
log ""
log "enabling $CONFIG_OPT in .config:"
CFG="$SDK_ROOT/.config"
if grep -q "^${CONFIG_OPT}=y" "$CFG"; then
    log "  already enabled ($CONFIG_OPT=y)"
    CFG_STATUS="already =y"
else
    cp -p "$CFG" "$CFG.bak.$TS"
    log "  backup : $CFG -> $CFG.bak.$TS"
    if grep -q "^# ${CONFIG_OPT} is not set" "$CFG"; then
        sed -i "s/^# ${CONFIG_OPT} is not set\$/${CONFIG_OPT}=y/" "$CFG"
        CFG_STATUS="flipped 'is not set' -> =y"
    else
        printf '%s=y\n' "$CONFIG_OPT" >> "$CFG"
        CFG_STATUS="appended =y"
    fi
    grep -q "^${CONFIG_OPT}=y" "$CFG" || die "failed to set ${CONFIG_OPT}=y in $CFG"
    log "  $CFG_STATUS"
fi

# ---- 5. rebuild scope ------------------------------------------------------
# A FULL app rebuild is mandatory the first time dvb2ip is enabled, because the
# macro inserts values into the GXMSG_* enum and renumbers every later message id
# (a partial build would mis-dispatch messages). Once it is already enabled and
# the last build's app_config.h already has SUPPORT=1, later iterations only need
# the changed objects rebuilt -- much faster.
NEED_FULL=0
[ "${CFG_STATUS:-}" = "already =y" ] || NEED_FULL=1
grep -q "define DVB2IP_SERVER_SUPPORT 1" "$SDK_ROOT/app/include/app_config.h" 2>/dev/null || NEED_FULL=1

log ""
if [ "$NEED_FULL" = "1" ]; then
    log "FULL app rebuild (dvb2ip newly enabled -> enum renumber makes a partial build unsafe):"
    rm -f "$SDK_ROOT/app/include/app_config.h" && log "  rm app/include/app_config.h (regenerated by post_config)"
    if [ -d "$SDK_ROOT/output/objects" ]; then
        n="$(find "$SDK_ROOT/output/objects" -maxdepth 1 -name '*.o' | wc -l | tr -d ' ')"
        find "$SDK_ROOT/output/objects" -maxdepth 1 -name '*.o' -delete
        log "  removed $n app objects from output/objects/"
    fi
else
    log "incremental rebuild (dvb2ip already enabled) -> forcing only changed objects:"
    for o in $OBJS; do
        if [ -f "$SDK_ROOT/$o" ]; then rm -f "$SDK_ROOT/$o" && log "  rm $o"; else log "  (absent) $o"; fi
    done
fi

# ---- 6. build (make, then make bin; stop hard if make fails) ---------------
log ""
log "=== build: make  (full app rebuild -- this takes a while) ==="
cd "$SDK_ROOT"
set +e; . "$ENV_SH"; set -e     # sourcing env scripts can return non-zero benignly

{ make 2>&1; echo $? > "$TMP/make.rc"; } | tee "$MAKE_LOG" | grep -iE 'dvb2ip|app ok|error|warning: implicit' || true
mk="$(cat "$TMP/make.rc" 2>/dev/null || echo 1)"
if [ "$mk" != "0" ]; then
    log ""
    log "---- last 30 lines of $MAKE_LOG ----"
    tail -n 30 "$MAKE_LOG" 2>/dev/null || true
    die "'make' failed (rc=$mk) -- NOT running 'make bin'. Full log: $MAKE_LOG"
fi

# confirm the macro actually regenerated to 1
if grep -q "define DVB2IP_SERVER_SUPPORT 1" "$SDK_ROOT/app/include/app_config.h" 2>/dev/null; then
    log "  app_config.h: DVB2IP_SERVER_SUPPORT = 1  (OK)"
else
    log "  WARNING: DVB2IP_SERVER_SUPPORT is not 1 in app_config.h -- lib present? Kconfig dep?"
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

# ---- 7. summary ------------------------------------------------------------
IMG="$SDK_ROOT/output/image/download_linux.bin"
log ""
log "======================= SUMMARY ======================="
log "REF               : $REF"
log "config            : ${CFG_STATUS:-already =y}"
log "sources installed :"
while IFS= read -r line; do log "    $line"; done < "$REPORT"
log ""
if [ -f "$IMG" ]; then
    isz="$(wc -c < "$IMG" | tr -d ' ')"
    log "image  : $IMG ($isz bytes)"
else
    log "image  : $IMG   (NOT FOUND - did 'make bin' succeed?)"
fi
log ""
log "next:"
log "  1) flash $IMG to the box"
log "  2) make sure the box is on the network (has an IP) and channels are scanned"
log "  3) serial console, watch for:"
log "       [DVB2IP] service_change: use=1 net=1 gui=1 prog=N ... -> START"
log "       [DVB2IP] server STARTED  ->  playlist: http://<ip>:8998/play_file ..."
log "     if it says '-> no-op', the printed use/net/gui/prog flags show which"
log "     gate is 0 (net=0 => no IP yet; prog=0 => no channels scanned)."
log "  4) from your laptop (same LAN):"
log "       curl -s http://<stb-ip>:8998/play_file        # lists current-TP prog ids"
log "       curl -s http://<stb-ip>:8999/stream=<id>.ts | mpv -   # or vlc"
log "======================================================="
