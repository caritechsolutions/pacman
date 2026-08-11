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
#      NOT safe here,
#   4. cross-builds the ARM stb_part7_receiver and injects it into whichever
#      rootfs source `make bin` actually consumes (sections 6a-6b), verifying
#      by reading the result back rather than by assuming the copy landed,
#   5. checks the partition budget against the flash map and refuses to hand
#      you an image that cannot flash (section 6d).
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

# STB-side Part 7 receiver. Source lives in the SHMS repo (single source of
# truth, shared with the headend sender it must stay in step with) and is
# fetched + cross-compiled by this script -- see section 6a.
STB_RX_NAME="stb_part7_receiver"
STB_RX_SRC_URL="https://raw.githubusercontent.com/caritechsolutions/Satellite-Hybrid-Management-System/main/stb_part7_receiver.c"
ROOTFS_DIR="${ROOTFS_DIR:-/opt/stb/rootfs-work}"
RIST_TREES="${RIST_TREES:-/opt/stb/rist}"

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

# ---- 6a. cross-build the STB Part 7 receiver -------------------------------
# Runs INSIDE this script on purpose: the whole workflow is one curl|sh on the
# toolchain VM, so anything needing ARM compilation has to happen here. Placed
# after env.sh (the cross gcc is on PATH from it) and before `make bin`, which
# packs the image tree.
#
# The source is fetched from SHMS main rather than kept as a second copy here --
# one source of truth. The ?cachebust is not decoration: raw.githubusercontent
# caches for ~5 minutes and has previously served a stale file for a whole
# debugging cycle.
log ""
log "=== cross-build: $STB_RX_NAME (ARM) ==="

src="$TMP/${STB_RX_NAME}.c"
url="${STB_RX_SRC_URL}?$(date +%s)"
log "download: $url"
curl -fsSL "$url" -o "$src" || die "FAILED to fetch ${STB_RX_NAME}.c from SHMS -- refusing to build a stale copy"
[ -s "$src" ] || die "fetched an empty ${STB_RX_NAME}.c -- refusing to build"
grep -q "BLOCK_CONTENT_PACKETS" "$src" || die "fetched ${STB_RX_NAME}.c looks wrong (no BLOCK_CONTENT_PACKETS) -- refusing to build"
log "  fetched $(wc -c < "$src" | tr -d ' ') bytes from SHMS main"

# Pick the librist ARM tree. Several exist at different commits, so rather than
# hardcoding or guessing "newest", prefer the tree whose built librist.so MATCHES
# the one already staged in the rootfs -- that is the library the box will
# actually run, so linking against anything else is the bug we are avoiding.
# Fall back to newest-by-mtime with a loud warning.
deployed_so="$(ls -1 "$ROOTFS_DIR"/lib/librist.so.[0-9]* 2>/dev/null | head -1)"
LIBRIST_TREE=""
if [ -n "$deployed_so" ] && command -v md5sum >/dev/null 2>&1; then
    want="$(md5sum "$deployed_so" | cut -d' ' -f1)"
    log "  rootfs librist: $deployed_so (md5 ${want})"
    for t in "$RIST_TREES"/*/librist/build-arm; do
        [ -d "$t" ] || continue
        cand="$(ls -1 "$t"/librist.so.[0-9]* 2>/dev/null | head -1)"
        [ -n "$cand" ] || continue
        if [ "$(md5sum "$cand" | cut -d' ' -f1)" = "$want" ]; then
            LIBRIST_TREE="$t"
            log "  librist tree: $t  (MATCHES the rootfs library)"
            break
        fi
    done
fi
if [ -z "$LIBRIST_TREE" ]; then
    # Sort by the LIBRARY's mtime, not the directory's: the build-arm dirs are all
    # created at checkout time, so `ls -dt` on them ranks by checkout order rather
    # than by which librist was built most recently.
    newest_so="$(ls -1t "$RIST_TREES"/*/librist/build-arm/librist.so.[0-9]* 2>/dev/null | head -1)"
    [ -n "$newest_so" ] && LIBRIST_TREE="$(dirname "$newest_so")"
    if [ -n "$LIBRIST_TREE" ]; then
        log "  WARNING: no librist build matches the rootfs copy; using NEWEST tree"
        log "           $LIBRIST_TREE"
        log "           if the box misbehaves, suspect a librist mismatch first"
    fi
fi
[ -n "$LIBRIST_TREE" ] || die "no ARM librist build found under $RIST_TREES/*/librist/build-arm -- run ristSTB scripts/cross-build.sh once"

TREE_ROOT="$(dirname "$(dirname "$LIBRIST_TREE")")"      # .../<checkout>
CROSS_CC="${CROSS_CC:-arm-nationalchip-linux-uclibcgnueabihf-gcc}"
command -v "$CROSS_CC" >/dev/null 2>&1 || die "$CROSS_CC not on PATH (is $ENV_SH the right toolchain env?)"

if "$CROSS_CC" -mcpu=cortex-a7 -mfpu=vfpv3-d16 -mfloat-abi=hard -std=gnu99 -O2 -Wall \
       -I "$TREE_ROOT/librist/include" \
       -I "$LIBRIST_TREE/include" \
       -I "$LIBRIST_TREE/include/librist" \
       "$src" -L "$LIBRIST_TREE" -lrist -lpthread \
       -o "$TMP/$STB_RX_NAME" 2>"$TMP/${STB_RX_NAME}.err"; then
    log "  compiled OK"
else
    log "---- compiler output ----"
    tail -n 25 "$TMP/${STB_RX_NAME}.err" 2>/dev/null || true
    die "cross-compile of ${STB_RX_NAME}.c FAILED"
fi

file "$TMP/$STB_RX_NAME" 2>/dev/null | grep -q 'ARM' \
    || die "built ${STB_RX_NAME} is not an ARM ELF -- wrong compiler?"

# Strip. ROOTFS is the tightest partition on the board, and debug_info in a
# binary that is never debugged on-target is pure partition tax.
CROSS_STRIP="${CROSS_STRIP:-$(printf '%s' "$CROSS_CC" | sed 's/gcc$/strip/')}"
sz_before="$(wc -c < "$TMP/$STB_RX_NAME" | tr -d ' ')"
if command -v "$CROSS_STRIP" >/dev/null 2>&1 && "$CROSS_STRIP" "$TMP/$STB_RX_NAME" 2>/dev/null; then
    sz_after="$(wc -c < "$TMP/$STB_RX_NAME" | tr -d ' ')"
    log "  stripped: $sz_before -> $sz_after bytes"
else
    log "  WARNING: $CROSS_STRIP unavailable -- shipping UNSTRIPPED ($sz_before bytes)"
fi

# ---- 6b. put it where `make bin` will actually find it ---------------------
# The bug this replaces: we used to copy into output/image/bin_linux/root/usr/bin
# and print "installed ->". That directory is a SCRATCH EXTRACTION -- mkimg.sh
# starts the rootfs stage with `rm -rf ${flashimg_build_path}/root` and then
# re-populates it from a source archive. So the copy was deleted seconds later
# and the log line was a lie that cost a flash cycle.
#
# mkimg.sh picks the rootfs source by this precedence (first hit wins):
#   1. $SDK_ROOT/output/image/rootfs            (directory, copied under fakeroot)
#   2. $SDK_ROOT/output/image/rootfs.tar.gz
#   3. $SDK_ROOT/../buildroot/output/rootfs.tar.gz
#   4. $SDK_ROOT/projects/<family>/<proj>/flash/rootfs.tar.gz
#   5. $SDK_ROOT/projects/<family>/<proj>/flash/rootfs.bin   (copied verbatim)
# We resolve the SAME order rather than hardcoding one, and inject there.
log ""
log "=== install $STB_RX_NAME into the rootfs source ==="

PROJ_NAME="$(sed -n 's/^BR2_PROJ_NAME="\(.*\)"$/\1/p'   "$SDK_ROOT/.config" | head -1)"
FAM_NAME="$(sed  -n 's/^BR2_FAMILY_NAME="\(.*\)"$/\1/p' "$SDK_ROOT/.config" | head -1)"
[ -n "$PROJ_NAME" ] || die "could not read BR2_PROJ_NAME from $SDK_ROOT/.config"
[ -n "$FAM_NAME"  ] || die "could not read BR2_FAMILY_NAME from $SDK_ROOT/.config"
PROJ_PATH="$SDK_ROOT/projects/$FAM_NAME/$PROJ_NAME"
log "  project: $FAM_NAME/$PROJ_NAME"

RFS_KIND=""; RFS_PATH=""
if   [ -e "$SDK_ROOT/output/image/rootfs" ];              then RFS_KIND=dir;    RFS_PATH="$SDK_ROOT/output/image/rootfs"
elif [ -e "$SDK_ROOT/output/image/rootfs.tar.gz" ];       then RFS_KIND=targz;  RFS_PATH="$SDK_ROOT/output/image/rootfs.tar.gz"
elif [ -e "$SDK_ROOT/../buildroot/output/rootfs.tar.gz" ];then RFS_KIND=targz;  RFS_PATH="$SDK_ROOT/../buildroot/output/rootfs.tar.gz"
elif [ -e "$PROJ_PATH/flash/rootfs.tar.gz" ];             then RFS_KIND=targz;  RFS_PATH="$PROJ_PATH/flash/rootfs.tar.gz"
elif [ -e "$PROJ_PATH/flash/rootfs.bin" ];                then RFS_KIND=binonly;RFS_PATH="$PROJ_PATH/flash/rootfs.bin"
else die "no rootfs source found -- 'make bin' would fail anyway. Looked in output/image/ and $PROJ_PATH/flash/"
fi
log "  rootfs source: $RFS_PATH  [$RFS_KIND]"

case "$RFS_KIND" in
binonly)
    die "the active rootfs source is a PREBUILT squashfs ($RFS_PATH).
     Nothing can be injected into it here, so ${STB_RX_NAME} would never reach
     the box. Provide $PROJ_PATH/flash/rootfs.tar.gz (mkimg.sh prefers it) or
     an output/image/rootfs/ tree, then re-run."
    ;;

dir)
    # Priority 1: a plain staging tree. Copy in and read the result back.
    mkdir -p "$RFS_PATH/usr/bin" || die "cannot create $RFS_PATH/usr/bin"
    cp "$TMP/$STB_RX_NAME" "$RFS_PATH/usr/bin/$STB_RX_NAME" || die "copy into $RFS_PATH failed"
    chmod 0755 "$RFS_PATH/usr/bin/$STB_RX_NAME"
    [ -x "$RFS_PATH/usr/bin/$STB_RX_NAME" ] || die "VERIFY FAILED: $RFS_PATH/usr/bin/$STB_RX_NAME is not executable"
    log "  VERIFIED in tree: usr/bin/$STB_RX_NAME ($(wc -c < "$RFS_PATH/usr/bin/$STB_RX_NAME" | tr -d ' ') bytes)"
    ;;

targz)
    # SURGICAL injection: decompress, delete our member if present, append the
    # new one, recompress. Every OTHER member is carried across untouched, so
    # ownership, permissions and -- critically -- symlinks survive by not being
    # touched at all. lib/librist.so.4 -> librist.so.4.5.0 stays a symlink
    # because we never re-create it.
    #
    # This is deliberately NOT "re-tar $ROOTFS_DIR". Re-tarring a staging tree
    # trusts that the tree is still an exact superset of the shipped tarball; if
    # anyone has touched the tarball since, re-tarring silently reverts them.
    # Injecting one member cannot revert anything.
    tar --version 2>/dev/null | grep -qi 'gnu tar' \
        || die "GNU tar required (need --delete); found: $(tar --version 2>&1 | head -1)"

    # Pristine copy, made once, is the way back. Timestamped backups accumulate
    # per run and the oldest is no longer the original after run 2.
    if [ ! -f "$RFS_PATH.orig" ]; then
        cp -p "$RFS_PATH" "$RFS_PATH.orig" || die "could not create $RFS_PATH.orig"
        log "  pristine backup created: $RFS_PATH.orig"
    else
        log "  pristine backup exists : $RFS_PATH.orig"
    fi
    # .prev is overwritten each run rather than timestamped: these are ~5 MB
    # apiece and one per run would quietly fill the toolchain VM. .orig is the
    # one that matters and it is never rewritten.
    cp -p "$RFS_PATH" "$RFS_PATH.prev" || die "could not back up $RFS_PATH"
    log "  previous-run backup    : $RFS_PATH.prev"
    log "  TO REVERT:  cp -p $RFS_PATH.orig $RFS_PATH"

    log "  reading current archive..."
    tar tzf "$RFS_PATH" > "$TMP/rfs.before" 2>/dev/null || die "cannot list $RFS_PATH (corrupt archive?)"
    n_before="$(grep -c . "$TMP/rfs.before" || true)"
    [ "${n_before:-0}" -gt 100 ] || die "$RFS_PATH lists only ${n_before:-0} entries -- that is not a rootfs, refusing to touch it"

    # Member naming: some archives are './usr/bin/x', some 'usr/bin/x'. Match
    # whatever this archive already uses or mkimg's extract lands it elsewhere.
    if head -20 "$TMP/rfs.before" | grep -q '^\./'; then MPFX="./"; else MPFX=""; fi
    MEMBER="${MPFX}usr/bin/$STB_RX_NAME"
    log "  entries: $n_before   member path: $MEMBER"

    gzip -dc "$RFS_PATH" > "$TMP/rfs.tar" || die "gunzip of $RFS_PATH failed"

    # Remove any previous copy under EITHER prefix, so re-runs are idempotent
    # and an old differently-prefixed entry cannot shadow the new one.
    for m in "usr/bin/$STB_RX_NAME" "./usr/bin/$STB_RX_NAME"; do
        if grep -qx "$m" "$TMP/rfs.before"; then
            tar --delete -f "$TMP/rfs.tar" "$m" 2>/dev/null \
                || die "tar --delete of existing $m failed"
            log "  removed previous member: $m"
        fi
    done

    mkdir -p "$TMP/stage/usr/bin"
    cp "$TMP/$STB_RX_NAME" "$TMP/stage/usr/bin/$STB_RX_NAME"
    chmod 0755 "$TMP/stage/usr/bin/$STB_RX_NAME"
    ( cd "$TMP/stage" && tar -rf "$TMP/rfs.tar" \
        --owner=0 --group=0 --numeric-owner --mode=0755 "$MEMBER" ) \
        || die "tar -r (append) of $MEMBER failed"

    gzip -9 -c "$TMP/rfs.tar" > "$TMP/rfs.tar.gz" || die "gzip of the new archive failed"

    # VERIFY BEFORE PUBLISHING. Read the rebuilt archive back and prove both
    # that our member is in it and that nothing else fell out. Only then does
    # the real file get replaced -- a failed verify leaves the original in place.
    tar tzf "$TMP/rfs.tar.gz" > "$TMP/rfs.after" 2>/dev/null \
        || die "the rebuilt archive is unreadable -- original left untouched"
    grep -qx "$MEMBER" "$TMP/rfs.after" \
        || die "VERIFY FAILED: $MEMBER not in the rebuilt archive -- original left untouched"
    n_after="$(grep -c . "$TMP/rfs.after" || true)"
    lost="$(sort "$TMP/rfs.before" > "$TMP/b.s"; sort "$TMP/rfs.after" > "$TMP/a.s"; comm -23 "$TMP/b.s" "$TMP/a.s" | grep -c . || true)"
    if [ "${lost:-0}" -ne 0 ]; then
        log "  entries present before but missing after:"
        comm -23 "$TMP/b.s" "$TMP/a.s" | head -20 | while IFS= read -r l; do log "      $l"; done
        die "VERIFY FAILED: $lost entr(y|ies) lost rebuilding the archive -- original left untouched"
    fi
    # `cat >` rather than `mv`: it writes THROUGH the existing inode, so the
    # archive keeps its original owner and mode. `mv` would drop $TMP's.
    cat "$TMP/rfs.tar.gz" > "$RFS_PATH" || die "could not write $RFS_PATH"

    # Final read-back of the file that will actually be consumed.
    tar tzf "$RFS_PATH" 2>/dev/null | grep -qx "$MEMBER" \
        || die "VERIFY FAILED after write: $MEMBER absent from $RFS_PATH"
    log "  VERIFIED in archive: $MEMBER   (entries $n_before -> $n_after)"
    ;;
esac

# Keep the librist staging tree in step. This does NOT feed the image (see the
# precedence list above); it exists so the tree stays a truthful mirror.
if [ -d "$ROOTFS_DIR/usr/bin" ]; then
    cp "$TMP/$STB_RX_NAME" "$ROOTFS_DIR/usr/bin/$STB_RX_NAME" && chmod 0755 "$ROOTFS_DIR/usr/bin/$STB_RX_NAME"
    log "  mirrored (not an image source): $ROOTFS_DIR/usr/bin/$STB_RX_NAME"
fi

# Informational drift check. We no longer re-tar $ROOTFS_DIR, so drift is not a
# hazard any more -- but if the two HAVE diverged that is worth knowing, because
# it means the staging tree is not the thing the box runs.
if [ "$RFS_KIND" = targz ] && [ -d "$ROOTFS_DIR" ]; then
    ( cd "$ROOTFS_DIR" && find . -type f -o -type l ) 2>/dev/null | sed 's|^\./||' | sort > "$TMP/work.s" || true
    sed 's|^\./||' "$TMP/rfs.after" | sed -e '/\/$/d' -e '/^$/d' | sort > "$TMP/arch.s" || true
    only_arch="$(comm -13 "$TMP/work.s" "$TMP/arch.s" | grep -c . || true)"
    only_work="$(comm -23 "$TMP/work.s" "$TMP/arch.s" | grep -c . || true)"
    if [ "${only_arch:-0}" -eq 0 ] && [ "${only_work:-0}" -eq 0 ]; then
        log "  drift check: $ROOTFS_DIR and the archive hold the same file set"
    else
        log "  drift check: staging tree and archive DIFFER"
        log "               in archive but not in $ROOTFS_DIR : $only_arch"
        log "               in $ROOTFS_DIR but not in archive : $only_work"
        log "               (harmless here -- we inject one member, we do not re-tar)"
        comm -13 "$TMP/work.s" "$TMP/arch.s" | head -8 | while IFS= read -r l; do log "                 archive-only: $l"; done
        comm -23 "$TMP/work.s" "$TMP/arch.s" | head -8 | while IFS= read -r l; do log "                 tree-only   : $l"; done
    fi
fi

log ""
log "=== build: make  (continuing) ==="
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

# ---- 6c. prove the binary survived into the packed rootfs ------------------
# bin_linux/root is the scratch tree mkimg.sh extracts the rootfs source into
# and then squashes. Checking it AFTER 'make bin' is a genuine end-to-end proof
# that the injection above reached the image -- unlike writing to it beforehand,
# which the extraction wipes.
log ""
log "=== verify: $STB_RX_NAME reached the packed rootfs ==="
PACKED="$SDK_ROOT/output/image/bin_linux/root/usr/bin/$STB_RX_NAME"
if [ -f "$PACKED" ]; then
    log "  OK: $PACKED ($(wc -c < "$PACKED" | tr -d ' ') bytes)"
else
    log "  the extracted tree does not contain usr/bin/$STB_RX_NAME."
    log "  rootfs source used for the injection was: $RFS_PATH [$RFS_KIND]"
    log "  mkimg.sh log lines about the rootfs source:"
    grep -iE 'rootfs|generate from|copy from' "$MAKEBIN_LOG" 2>/dev/null | head -10 | while IFS= read -r l; do log "      $l"; done
    die "VERIFY FAILED: $STB_RX_NAME is NOT in the image. Do not flash this build.
     Most likely mkimg.sh chose a HIGHER-precedence rootfs source than the one
     we injected into -- the lines above say which."
fi

# ---- 6d. partition budget --------------------------------------------------
# ROOTFS on this board is declared 'auto' size at an 'auto' address, so the
# "99.2% used" figure the box prints is just the 64k block rounding, not a
# ceiling -- the partition grows with the file. The REAL ceiling is the next
# partition that pins an absolute address (DATA at 0xDC0000 here): everything
# before it has to fit underneath. So that is what we check.
log ""
log "=== partition budget ==="
BIN_DIR="$SDK_ROOT/output/image/bin_linux"
FLASH_CONF="$BIN_DIR/flash.conf"
[ -f "$FLASH_CONF" ] || FLASH_CONF="$PROJ_PATH/flash/flash.conf"
if [ -f "$FLASH_CONF" ]; then
    BLK="$(sed -n 's/^block_size[[:space:]]*\(0x[0-9A-Fa-f]*\).*/\1/p' "$FLASH_CONF" | head -1)"
    BLK="$(( ${BLK:-0x10000} ))"
    [ "$BLK" -gt 0 ] || BLK=65536
    grep -vE '^[[:space:]]*(#|$)' "$FLASH_CONF" | awk 'NF>=9 && $1 ~ /^[A-Z]+$/' > "$TMP/parts" || true

    cursor=0; overflow=0; headroom=""; ceiling_name=""
    log "  block 0x$(printf '%X' "$BLK")   name      addr       size       file"
    while read -r pname pfile pcrc pfs pmode pupd pver paddr psize prest; do
        [ -n "${pname:-}" ] || continue
        if [ "$paddr" != "auto" ]; then
            a="$(( paddr ))"
            if [ "$cursor" -gt "$a" ]; then
                log "  !! $pname is pinned at 0x$(printf '%X' "$a") but the partitions"
                log "     before it already run to 0x$(printf '%X' "$cursor") -- OVERFLOW by"
                log "     $(( cursor - a )) bytes"
                overflow=1
            else
                headroom="$(( a - cursor ))"; ceiling_name="$pname"
            fi
            cursor="$a"
        fi
        s=0
        if [ "$psize" = "auto" ]; then
            if [ "$pfile" != "NULL" ] && [ -f "$BIN_DIR/$pfile" ]; then
                fsz="$(wc -c < "$BIN_DIR/$pfile" | tr -d ' ')"
                s="$(( (fsz + BLK - 1) / BLK * BLK ))"
            fi
        else
            case "$psize" in
                *[kK]) s="$(( ${psize%[kK]} * 1024 ))" ;;
                *[mM]) s="$(( ${psize%[mM]} * 1024 * 1024 ))" ;;
                0x*|[0-9]*) s="$(( psize ))" ;;
            esac
        fi
        log "        $(printf '%-8s 0x%08X 0x%08X  %s' "$pname" "$cursor" "$s" "$pfile")"
        cursor="$(( cursor + s ))"
    done < "$TMP/parts"

    if [ "$overflow" -ne 0 ]; then
        die "PARTITION OVERFLOW -- this image will not flash correctly. Do not flash it.
     Options: shrink the rootfs (strip more binaries / drop unused files), or move
     $STB_RX_NAME to the APP partition instead of ROOTFS."
    elif [ -n "$headroom" ]; then
        log "  headroom before $ceiling_name: $headroom bytes ($(( headroom / 1024 )) KB)"
        if [ "$headroom" -lt "$BLK" ]; then
            log "  NOTE: under one erase block spare -- the next addition may not fit."
        fi
    fi
else
    log "  WARNING: no flash.conf found -- partition budget NOT checked"
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
