#!/bin/sh
# ============================================================================
#  CarPlay navigation on the HUD  --  USB autorun payload (JCI DataRetrieval)
#
#  ==================  PATCH BY: KID MIXER-MODER  ==================
#
#  Stick root needs: tweaks.sh, install.sh, libpatch-blmjcicarplay.so,
#  dataRetrieval_config.txt, jci-autoupdate, and a signed cmu_dataretrieval.up
#  (not shipped -- supply your own, or install over SSH; see INSTALL.md).
# ============================================================================
set -u
MOD_NAME="CarPlay HUD Navigation"
MOD_VER="1.0"
AUTHOR="KID MIXER-MODER"
DIALOG=/jci/tools/jci-dialog

hwclock --hctosys 2>/dev/null

MYDIR=$(dirname "$(readlink -f "$0")")
mount -o rw,remount "$MYDIR" 2>/dev/null
LOG="$MYDIR/CP-HUD_log.txt"

log() { echo "$*"; echo "$*" >> "$LOG"; /bin/fsync "$LOG" 2>/dev/null; }
dlg() {
  sleep 2
  killall -q jci-dialog
  [ -x "$DIALOG" ] && "$DIALOG" --info --title="$MOD_NAME v$MOD_VER" \
      --text="$*" --no-cancel >/dev/null 2>&1 &
}

log ""
log "===== CarPlay HUD Navigation  USB install  ($(date)) ====="
log "By  : $AUTHOR"
log "USB : $MYDIR"
dlg "Installing CarPlay navigation on the HUD...\nDo not remove the USB stick."

# --- preflight --------------------------------------------------------------
if [ ! -f /jci/sm/sm.conf ]; then
  log "FATAL: not a Mazda CMU (no /jci/sm/sm.conf) -- aborting, nothing changed."
  dlg "ERROR: this is not a Mazda CMU.\nNothing was changed."
  sleep 15
  exit 1
fi
if [ ! -f "$MYDIR/install.sh" ] || [ ! -f "$MYDIR/libpatch-blmjcicarplay.so" ]; then
  log "FATAL: install.sh or libpatch-blmjcicarplay.so missing next to tweaks.sh."
  dlg "ERROR: installer files missing on the USB.\nNothing was changed."
  sleep 15
  exit 1
fi

# --- run the shared installer, log onto the USB -----------------------------
log "---> running install.sh"
sh "$MYDIR/install.sh" >> "$LOG" 2>&1
RC=$?
/bin/fsync "$LOG" 2>/dev/null

if [ "$RC" != "0" ]; then
  log "install.sh FAILED (rc=$RC) -- NOT rebooting. See CP-HUD_log.txt on the USB."
  dlg "Install FAILED (rc=$RC).\nSee CP-HUD_log.txt on the USB."
  sleep 20
  exit "$RC"
fi

# --- success: notify + reboot -----------------------------------------------
log "===== install OK -> rebooting ====="
sync
dlg "CarPlay navigation on the HUD installed.\nThe unit will restart in a few seconds..."
sleep 10
killall -q jci-dialog
[ -x "$DIALOG" ] && "$DIALOG" --info --title="$MOD_NAME v$MOD_VER | $AUTHOR" \
    --text="Done. You may remove the USB now.\nRestarting..." --no-cancel >/dev/null 2>&1 &
sleep 3
sync
reboot
exit 0
