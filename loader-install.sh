#!/bin/sh
# loader-install.sh - fetch the loader change and rebuild the bootloader.
#
# SEPARATE from install.sh: install.sh builds the APP; this builds the LOADER
# (platform/gxloader), which uses its own standalone ./build script and a
# different toolchain (arm-none-eabi). `make bin` does NOT rebuild the loader.
#
# WHAT IT DOES
#   1. downloads the changed platform/gxloader/common/meminfo.c from the repo,
#   2. runs the loader's own build: ./build canopus 6631SHNF release,
#   3. points you at the loader image to flash.
#
# IT DOES NOT FLASH. Flashing the BOOT partition is a separate, brick-capable
# step (usbdown-over-serial) you do by hand -- see the notes printed at the end.
#
# Usage:   curl -fsSL <raw-url>/loader-install.sh | sh
#   env overrides:  SDK_BASE=...  REF=...  CHIP=...  BOARD=...
#
# Plain POSIX sh. Needs curl + the loader toolchain already on PATH (the same
# environment you used for previous bootloader builds).

set -eu

RAW_BASE="https://raw.githubusercontent.com/caritechsolutions/pacman"
REF="${REF:-claude/gx6631-ts-userspace-rist-fe2wkc}"
REPO_PREFIX="6631SDK"
# SDK base = the dir that contains platform/ and solution/ (one up from SDK_ROOT).
SDK_BASE="${SDK_BASE:-/opt/stb/sdk-clean/6631SDK}"
CHIP="${CHIP:-canopus}"
BOARD="${BOARD:-6631SHNF}"

REL="platform/gxloader/common/meminfo.c"      # the changed loader source
LOADER_DIR="$SDK_BASE/platform/gxloader"

TS="$(date +%Y%m%d-%H%M%S)"
log() { printf '%s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

command -v curl >/dev/null 2>&1 || die "curl not found on PATH"
[ -d "$LOADER_DIR" ] || die "loader dir not found: $LOADER_DIR (set SDK_BASE=...)"
[ -x "$LOADER_DIR/build" ] || [ -f "$LOADER_DIR/build" ] || die "no ./build in $LOADER_DIR"
[ -f "$LOADER_DIR/conf/$CHIP/$BOARD/release.config" ] || \
    die "no conf/$CHIP/$BOARD/release.config -- wrong CHIP/BOARD? (set CHIP=/BOARD=)"

log "=== loader rebuild ==="
log "  REF   : $REF"
log "  board : $CHIP $BOARD (release)"
log "  loader: $LOADER_DIR"
log ""

# ---- 1. fetch the changed loader source (back up what we overwrite) --------
dest="$SDK_BASE/$REL"
url="$RAW_BASE/$REF/$REPO_PREFIX/$REL"
[ -f "$dest" ] && cp -p "$dest" "$dest.bak.$TS" && log "backup: $dest -> $dest.bak.$TS"
log "download: $url"
curl -fsSL "$url" -o "$dest" || die "download failed: $url"
[ -s "$dest" ] || die "downloaded an empty file: $url"
# sanity: make sure the TSW-clear line is actually present
grep -q "GXMEM_FLAG_DEMUX_TSW" "$dest" || die "meminfo.c missing the DEMUX_TSW change -- wrong REF?"
log "  installed $REL ($(wc -c < "$dest" | tr -d ' ') bytes)"

# ---- 2. build the loader with its own standalone build script --------------
log ""
log "=== ./build $CHIP $BOARD release  (loader toolchain) ==="
cd "$LOADER_DIR"
chmod +x ./build 2>/dev/null || true
./build "$CHIP" "$BOARD" release || die "loader build failed (toolchain on PATH? arm-none-eabi)"

# ---- 3. show the resulting loader image(s) ---------------------------------
log ""
log "======================= LOADER BUILT ======================="
log "outputs in $LOADER_DIR :"
ls -la "$LOADER_DIR"/loader*.bin "$LOADER_DIR"/loader.elf 2>/dev/null | while IFS= read -r l; do log "  $l"; done
log ""
log "FLASHING (manual, BOOT partition -- brick-capable, do carefully):"
log "  * this only rebuilt the loader image; it did NOT flash anything."
log "  * your GUI USB upgrade rewrites APP/ROOTFS only, NOT BOOT, so it will"
log "    NOT pick this up -- you must flash BOOT via usbdown-over-serial:"
log "      - box fully powered off, serial terminal CLOSED (frees the port),"
log "      - trigger usbdown in the ~400ms loader window at power-on,"
log "      - write the loader image to the BOOT partition (128k@0m)."
log "  * do NOT interrupt the write. Have your loader-recovery path ready."
log ""
log "VERIFY after reflash + boot:"
log "  cat /proc/cmdline        -> expect  protect_flag=85   (was 87)"
log "  serial on channel play   -> expect  source: tsw - hwsec 0   (was 1)"
log "  [DVB2IP] DIAG raw: [47 ..] sync47=1  -> clear TS, VLC decodes"
log "============================================================"
