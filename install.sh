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
#   - app_ts_record.c       : dmx2 clear-TS capture; instance runtime-selectable
#                             via /tmp/ristdmx (default 2, the unprotected one).
#   - app_rist_capture.c    : zap-driven dmx2 clear-TS -> UDP output (sources the
#                             app_ts_record capture; dest via /tmp/ristcap "ip:port").
#   - app_play_control.c    : re-adds the capture-on-zap hook (app_rist_play_change)
#                             in app_normal_play. Screen decode left untouched.
#   - app_rist_api.c/.h     : recovery API client (fetch + cache by service_id).
#   - app_rist_stats.c/.h   : per-view statistics ring, POSTed on the API call.
#   - app_network_service.c : starts the API client when the network gets an IP.
#   - full_screen.c/.h      : "Waiting..." tip while a RIST channel comes up.
# NOTE: headers count. A .c deployed without its changed header builds against
# the stale declaration -- exactly how FULL_STATE_CONNECTING failed to compile.
FILES="app/dvb2ip_server/app_dvb2ip_platform.c
app/dvb2ip_server/app_ts_record.c
app/module/app_rist_capture.c
app/include/module/app_rist_api.h
app/module/app_rist_api.c
app/include/module/app_rist_stats.h
app/module/app_rist_stats.c
app/module/network/app_network_service.c
app/module/app_play_control.c
app/include/full_screen.h
app/full_screen.c"

# Objects for those sources (flat basename.o), used for the fast INCREMENTAL path
# once dvb2ip is already enabled.
OBJS="output/objects/app_dvb2ip_platform.o
output/objects/app_ts_record.o
output/objects/app_rist_capture.o
output/objects/app_rist_api.o
output/objects/app_rist_stats.o
output/objects/app_network_service.o
output/objects/app_play_control.o
output/objects/full_screen.o"

# Kconfig symbols to enable. config_parse.sh does `source .config` and reads
# these as plain shell variables, so appending them here is sufficient -- the
# Kconfig `depends on` clauses are not re-evaluated by the build.
#   BR2_MOD_DVB2IP_SERVER : the dmx2 clear-TS capture the whole chain feeds from.
#   BR2_MOD_APP_ETH       : wired ethernet. The box HAS a working eth0 netdev, but
#     ETH_SUPPORT=0 made app_if_dev_check_type() return IF_TYPE_UNKOWN for it, so
#     the app skipped the interface entirely. Note Config.in.network's `depends on`
#     does not list canopus/6631SHNF, so this is NOT selectable via menuconfig --
#     which is exactly why it is force-appended here. Safe if the port is dead:
#     enumeration gates eth0 on app_check_netlink() (an ETHTOOL_GLINK carrier
#     check), so no link => eth0 skipped => WiFi continues as today.
CONFIG_OPTS="BR2_MOD_DVB2IP_SERVER
BR2_MOD_APP_ETH"

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

# ---- 2. PREFLIGHT: every file this branch changes must be in FILES ----------
# Failure mode this closes: a changed file left out of FILES. The box then keeps
# the OLD source while everything else updates -- which cost us a silent stale
# app_rist_capture.c once, and a 'FULL_STATE_CONNECTING undeclared' build break
# once. deploy.manifest is generated from git (gen-deploy-manifest.sh), so it
# cannot drift by hand the way FILES can.
log ""
log "=== preflight: FILES vs deploy.manifest ==="
MANIFEST="$TMP/deploy.manifest"
if curl -fsSL "$RAW_BASE/$REF/deploy.manifest" -o "$MANIFEST" 2>/dev/null && [ -s "$MANIFEST" ]; then
    pf_missing=""
    while IFS= read -r m; do
        [ -n "$m" ] || continue
        case " $(echo $FILES) " in
            *" $m "*) : ;;
            *) pf_missing="$pf_missing $m" ;;
        esac
    done < "$MANIFEST"

    if [ -n "$pf_missing" ]; then
        log ""
        log "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        log "!! PREFLIGHT FAILED - these files changed on '$REF' but are NOT"
        log "!! in install.sh's FILES list, so the box would build against the"
        log "!! OLD copy of each:"
        for m in $pf_missing; do log "!!     $m"; done
        log "!!"
        log "!! Add them to FILES (and their .o to OBJS if they are .c files)."
        log "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        die "preflight failed: FILES is missing $(echo $pf_missing | wc -w) changed file(s)"
    fi
    log "  OK: all $(grep -c . "$MANIFEST") changed file(s) are in FILES"
else
    log "  WARNING: could not fetch deploy.manifest -- FILES not cross-checked."
    log "           (regenerate + commit it with ./gen-deploy-manifest.sh)"
fi

# ---- 2b. download changed sources to temp FIRST (never half-install) --------
log ""
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
HDR_CHANGED=0
HDR_LIST=""
for rel in $FILES; do
    dest="$SDK_ROOT/$rel"
    mkdir -p "$(dirname "$dest")"
    if [ -f "$dest" ]; then
        cp -p "$dest" "$dest.bak.$TS"
        log "  backup : $dest -> $dest.bak.$TS"
        # A changed HEADER can silently invalidate objects we are NOT rebuilding:
        # an inserted enum value, a struct field, a changed macro. Those objects
        # keep the old constants and misbehave at RUNTIME with no build error.
        # Detect it here and force a full rebuild below.
        case "$rel" in
            *.h) cmp -s "$TMP/$rel" "$dest" || { HDR_CHANGED=1; HDR_LIST="$HDR_LIST $rel"; } ;;
        esac
    else
        # A brand-new header introduces declarations nothing was built against.
        case "$rel" in
            *.h) HDR_CHANGED=1; HDR_LIST="$HDR_LIST $rel(new)" ;;
        esac
    fi
    cp "$TMP/$rel" "$dest"
    sz="$(wc -c < "$dest" | tr -d ' ')"
    log "  write  : $dest ($sz bytes)"
    printf '%s  (%s bytes)\n' "$rel" "$sz" >> "$REPORT"
done

# ---- 4. enable dvb2ip in .config (idempotent) ------------------------------
log ""
log "enabling Kconfig symbols in .config:"
CFG="$SDK_ROOT/.config"
CFG_STATUS="already =y"
CFG_BACKED_UP=0
for opt in $CONFIG_OPTS; do
    if grep -q "^${opt}=y" "$CFG"; then
        log "  $opt : already enabled"
        continue
    fi
    if [ "$CFG_BACKED_UP" = "0" ]; then
        cp -p "$CFG" "$CFG.bak.$TS"
        log "  backup : $CFG -> $CFG.bak.$TS"
        CFG_BACKED_UP=1
    fi
    if grep -q "^# ${opt} is not set" "$CFG"; then
        sed -i "s/^# ${opt} is not set\$/${opt}=y/" "$CFG"
        log "  $opt : flipped 'is not set' -> =y"
    else
        printf '%s=y\n' "$opt" >> "$CFG"
        log "  $opt : appended =y"
    fi
    grep -q "^${opt}=y" "$CFG" || die "failed to set ${opt}=y in $CFG"
    CFG_STATUS="changed"
done

# ---- 5. rebuild scope ------------------------------------------------------
# A FULL app rebuild is mandatory the first time dvb2ip is enabled, because the
# macro inserts values into the GXMSG_* enum and renumbers every later message id
# (a partial build would mis-dispatch messages). Once it is already enabled and
# the last build's app_config.h already has SUPPORT=1, later iterations only need
# the changed objects rebuilt -- much faster.
#
# A changed HEADER also forces a full rebuild. An incremental build only
# recompiles the objects in OBJS, so a header that shifts an enum value, changes
# a struct layout or redefines a macro leaves every OTHER object holding stale
# constants -- and that fails at RUNTIME, silently, with no build error to catch
# it. (FULL_STATE_CONNECTING inserted ahead of FULL_STATE_DUMMY would have done
# exactly that to the 18 other objects referencing FULL_STATE_*.) Header changes
# are rare, so paying a full rebuild eliminates the class outright rather than
# relying on append-only discipline being remembered every time.
NEED_FULL=0
[ "${CFG_STATUS:-}" = "already =y" ] || NEED_FULL=1   # any symbol newly set
grep -q "define DVB2IP_SERVER_SUPPORT 1" "$SDK_ROOT/app/include/app_config.h" 2>/dev/null || NEED_FULL=1
if [ "$HDR_CHANGED" = "1" ]; then
    NEED_FULL=1
    log ""
    log "header change detected ->$HDR_LIST"
    log "  forcing a FULL rebuild: non-rebuilt objects could hold stale enum/struct/macro values"
fi

log ""
if [ "$NEED_FULL" = "1" ]; then
    log "FULL app rebuild (dvb2ip newly enabled, or a header changed -> partial build unsafe):"
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

if grep -q "define ETH_SUPPORT 1" "$SDK_ROOT/app/include/app_config.h" 2>/dev/null; then
    log "  app_config.h: ETH_SUPPORT = 1  (OK -- wired eth0 will be enumerated when it has carrier)"
else
    log "  WARNING: ETH_SUPPORT is not 1 in app_config.h -- wired stays disabled, WiFi only."
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
