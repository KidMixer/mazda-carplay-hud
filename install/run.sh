#!/bin/sh
# ============================================================================
#  USB autorun entry point (MZD-AIO / dp-tweak style).
#  Place this run.sh + install.sh + libpatch-blmjcicarplay.so at the ROOT of a
#  FAT32 USB stick. On these CMUs the boot autorun framework scans
#  /tmp/mnt/sd*/run.sh and executes it, so simply inserting the stick and
#  waiting (or rebooting with it inserted) runs the installer. Shows progress
#  on the center display via jci-dialog.
#
#  NOTE: requires the AIO/dp-tweak autorun framework already present on the unit
#  (it is, on any unit that has the Android-Auto mod). If not present, run
#  install.sh manually over SSH instead.
# ============================================================================
HERE=$(dirname "$0")
DIALOG=/jci/tools/jci-dialog
LOG=/tmp/cp-hud-install.log

show() { [ -x "$DIALOG" ] && "$DIALOG" --info --title="CarPlay HUD" --text="$*" --no-cancel >/dev/null 2>&1 & }

{
  echo "=== CarPlay->HUD USB install $(date) ==="
  show "Installing CarPlay->HUD bridge... do not remove USB."
  sh "$HERE/install.sh"
  RC=$?
  if [ "$RC" = "0" ]; then
    show "CarPlay->HUD installed. Reboot the car to activate."
  else
    show "Install FAILED (rc=$RC). See $LOG."
  fi
} >> "$LOG" 2>&1
