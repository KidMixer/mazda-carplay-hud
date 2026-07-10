#!/bin/sh
# ============================================================================
#  Uninstaller for CarPlay navigation on the HUD. Restores stock config from the
#  backups install.sh made (/data_persist/*.bak_precphud) and removes the .so.
#  ==================  PATCH BY: KID MIXER-MODER  ==================
#  Usage:  sh uninstall.sh   (over SSH, or via tweaks.sh from USB)
# ============================================================================
set -u
TAG="[cp-hud uninstall]"
MOD_DIR="/data_persist/cp-hud-mod"
MASTER="/etc/devmgr_config_master.xml"
log() { echo "$TAG $*"; }

[ -f /jci/sm/sm.conf ] || { log "not a CMU — aborting"; exit 1; }
mount -o remount,rw / 2>/dev/null || { log "remount rw failed"; exit 1; }

for CONF in /jci/sm/sm.conf /jci/sm/sm_WCP.conf; do
  [ -f "$CONF" ] || continue
  BAK="/data_persist/$(basename "$CONF").bak_precphud"
  if [ -f "$BAK" ]; then
    cp -a "$BAK" "$CONF" && log "restored $CONF from backup"
  elif grep -q 'libpatch-blmjcicarplay' "$CONF" 2>/dev/null; then
    # no backup (partial install): strip our lines + restore reset_board
    grep -v 'libpatch-blmjcicarplay' "$CONF" | grep -v 'LD_LIBRARY_PATH.*jci/lib:/usr/lib' > /tmp/cphud.u
    cp /tmp/cphud.u "$CONF"; rm -f /tmp/cphud.u
    sed -i '/name="jciCARPLAY"/ s/reset_board="no"/reset_board="yes"/' "$CONF"
    log "stripped cp-hud lines from $CONF (no backup found)"
  fi
done

MBAK="/data_persist/$(basename "$MASTER").bak_precphud"
if [ -f "$MBAK" ]; then
  cp -a "$MBAK" "$MASTER" && log "restored $MASTER from backup"
else
  sed -i 's#<name>NaviSupported</name><value>TRUE</value>#<name>NaviSupported</name><value>FALSE</value>#' "$MASTER" 2>/dev/null
  log "reset NaviSupported=FALSE in $MASTER (no backup found)"
fi

rm -rf "$MOD_DIR" && log "removed $MOD_DIR"
sync
mount -o remount,ro / 2>/dev/null

log "=== VERIFY (expect 0 / FALSE) ==="
log "sm.conf preload-lines=$(grep -c 'libpatch-blmjcicarplay' /jci/sm/sm.conf 2>/dev/null)   NaviSupported=$(grep -oE '<name>NaviSupported</name><value>[A-Z]+' "$MASTER" 2>/dev/null | grep -oE '[A-Z]*$')"
log "DONE. Reboot to return fully to stock."
