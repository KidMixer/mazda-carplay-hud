#!/bin/sh
# ============================================================================
#  CarPlay navigation on the HUD  --  installer for Mazda CMU (MAZ_CMU-150 / FW 74.00.324)
#  CX-5 KF / CX-8 2018 and same-platform Mazda Connect units.
#
#  ==================  PATCH BY: KID MIXER-MODER  ==================
# ============================================================================
set -u
TAG="[cp-hud install]"
SELF_DIR=$(dirname "$0")
SO_SRC="$SELF_DIR/libpatch-blmjcicarplay.so"
MOD_DIR="/data_persist/cp-hud-mod"
SO_DST="$MOD_DIR/libpatch-blmjcicarplay.so"
SO_TOKEN="libpatch-blmjcicarplay"
PRELOAD_LINE='            <environ_var env_name="LD_PRELOAD" env_value="/data_persist/cp-hud-mod/libpatch-blmjcicarplay.so"/>'
LDPATH_LINE='            <environ_var env_name="LD_LIBRARY_PATH" env_value="/jci/lib:/usr/lib"/>'
MASTER="/etc/devmgr_config_master.xml"

log() { echo "$TAG $*"; }
die() { echo "$TAG ERROR: $*"; mount -o remount,ro / 2>/dev/null; exit 1; }

# --- preflight ---------------------------------------------------------------
[ -f "$SO_SRC" ] || die "libpatch-blmjcicarplay.so not found next to install.sh ($SO_SRC). Build it first (see ../mazda)."
[ -f /jci/sm/sm.conf ] || die "not a CMU (no /jci/sm/sm.conf) — aborting"
log "installing from $SO_SRC ($(wc -c < "$SO_SRC") bytes)"

# --- pick the sm config the SM actually launches jciCARPLAY from -------------
# /usr/bin/autostart runs get_board_type.sh: exit 2 = WCP hardware -> the SM
# boots `sm -f sm_WCP.conf`; otherwise `sm -f sm.conf`. sm_WCP.conf ALSO ships
# on non-WCP units, so choosing by file presence would wrongly patch a config
# that is never launched. Run the same hardware probe and patch the active one.
if [ -x /jci/scripts/get_board_type.sh ]; then
  /jci/scripts/get_board_type.sh >/dev/null 2>&1; BT=$?
else
  BT=""
fi
if [ "$BT" = "2" ] && [ -f /jci/sm/sm_WCP.conf ]; then
  CONFS="/jci/sm/sm_WCP.conf"
  log "WCP hardware (get_board_type.sh=2) -> patching sm_WCP.conf"
else
  CONFS="/jci/sm/sm.conf"
  log "no WCP hardware (get_board_type.sh=${BT:-n/a}) -> patching sm.conf"
fi

# --- remount rootfs rw -------------------------------------------------------
mount -o remount,rw / 2>/dev/null || die "remount rw failed"

# --- 1) place the .so on a boot-visible persistent path ----------------------
mkdir -p "$MOD_DIR" || die "mkdir $MOD_DIR failed"
cp -f "$SO_SRC" "$SO_DST" || die "copy .so failed"
chmod 0644 "$SO_DST"
log "installed $SO_DST"

# clean any stale LD_PRELOAD this mod may have left in the OTHER config
for OTHER in /jci/sm/sm.conf /jci/sm/sm_WCP.conf; do
  case " $CONFS " in *" $OTHER "*) continue;; esac
  [ -f "$OTHER" ] || continue
  if grep -q "$SO_TOKEN" "$OTHER"; then
    [ -f "/data_persist/$(basename "$OTHER").bak_precphud" ] || cp -a "$OTHER" "/data_persist/$(basename "$OTHER").bak_precphud"
    grep -v "$SO_TOKEN" "$OTHER" | grep -v 'LD_LIBRARY_PATH.*jci/lib:/usr/lib' > /tmp/cphud.clean && cp /tmp/cphud.clean "$OTHER"
    rm -f /tmp/cphud.clean
    log "removed stale LD_PRELOAD from $OTHER (avoids double-inject)"
  fi
done

# --- 2) patch the chosen sm.conf: LD_PRELOAD + LD_LIBRARY_PATH on jciCARPLAY --
for CONF in $CONFS; do
  [ -f "$CONF" ] || { log "skip (absent): $CONF"; continue; }
  BAK="/data_persist/$(basename "$CONF").bak_precphud"
  [ -f "$BAK" ] || { cp -a "$CONF" "$BAK"; log "backup -> $BAK"; }

  if grep -q "$SO_TOKEN" "$CONF"; then
    log "$CONF: LD_PRELOAD already present, skipping insert"
  else
    awk -v pl="$PRELOAD_LINE" -v lp="$LDPATH_LINE" '
      /name="jciCARPLAY"/ { print; print pl; print lp; next } 1
    ' "$CONF" > /tmp/cphud.new || die "awk failed on $CONF"
    old=$(wc -l < "$CONF"); new=$(wc -l < /tmp/cphud.new)
    if [ "$new" = "$((old+2))" ] && grep -q "$SO_TOKEN" /tmp/cphud.new; then
      cp /tmp/cphud.new "$CONF"; log "$CONF: inserted environ_var ($old->$new lines)"
    else
      log "$CONF: SANITY FAILED ($old->$new), left untouched"
    fi
    rm -f /tmp/cphud.new
  fi
  # reset_board=no on jciCARPLAY only (a shim fault won't reboot-loop the unit)
  sed -i '/name="jciCARPLAY"/ s/reset_board="yes"/reset_board="no"/' "$CONF"
done

# --- 3) enable nav advertisement in the devmgr master template ---------------
if [ -f "$MASTER" ]; then
  MBAK="/data_persist/$(basename "$MASTER").bak_precphud"
  [ -f "$MBAK" ] || { cp -a "$MASTER" "$MBAK"; log "backup -> $MBAK"; }
  sed -i 's#<name>NaviSupported</name><value>FALSE</value>#<name>NaviSupported</name><value>TRUE</value>#' "$MASTER"
  log "set NaviSupported=TRUE in $MASTER"
else
  log "WARN: $MASTER absent — NaviSupported not set"
fi

# --- finish ------------------------------------------------------------------
sync
mount -o remount,ro / 2>/dev/null
echo ""
log "=== VERIFY ==="
for CONF in $CONFS; do
  log "$(basename "$CONF") preload-lines=$(grep -c "$SO_TOKEN" "$CONF" 2>/dev/null)  $(grep 'name=.jciCARPLAY. path' "$CONF" 2>/dev/null | grep -oE 'reset_board=.[a-z]*')"
done
log "NaviSupported=$(grep -oE '<name>NaviSupported</name><value>[A-Z]+' "$MASTER" 2>/dev/null | grep -oE '[A-Z]*$')   so-installed=$([ -f "$SO_DST" ] && echo yes || echo NO)"
echo ""
log "DONE. **Reboot the unit** to load the bridge."
log "After a CarPlay nav session, the debug build logs to /tmp/carplay_bridge.log."
log "To remove: run uninstall.sh"
