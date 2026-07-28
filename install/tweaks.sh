#!/bin/sh
# ============================================================================
#  CarPlay navigation on the HUD  --  USB installer (Install / Uninstall menu)
#  Auto-detects Wireless-CarPlay (sm_WCP.conf) vs wired CarPlay (sm.conf).
#  For an ALREADY hacked/rooted Mazda Connect CMU (gen5, FW 74.00.324;
#  CX-5 KF / CX-8 2018 and same-platform units).
#
#  ==================  PATCH BY: KID MIXER-MODER  ==================
# ============================================================================
MOD_NAME="CarPlay HUD Navigation"
MOD_VER="2.0"
AUTHOR="KID MIXER-MODER"

hwclock --hctosys 2>/dev/null

# disable the hardware watchdog so a longer install can't trigger a reboot
echo 1 > /sys/class/gpio/Watchdog\ Disable/value 2>/dev/null
mount -o rw,remount / 2>/dev/null

MYDIR=$(dirname "$(readlink -f "$0")")
mount -o rw,remount "${MYDIR}" 2>/dev/null
mkdir -p "${MYDIR}/logs"

# --- Prompt: Install or Uninstall -------------------------------------------
/jci/tools/jci-dialog --question \
  --title="${MOD_NAME} V${MOD_VER}  |  ${AUTHOR}" \
  --text="What would you like to do?" \
  --ok-label="Install" \
  --cancel-label="Uninstall"
CHOICE=$?

if [ $CHOICE -eq 0 ]; then
  # ==========================================================================
  # INSTALL
  # ==========================================================================
  exec > "${MYDIR}/logs/install.log" 2>&1
  echo "=== ${MOD_NAME} V${MOD_VER} install start ($(date)) -- by ${AUTHOR} ==="

  killall -q jci-dialog
  /jci/tools/jci-dialog --info --title="${MOD_NAME} V${MOD_VER}  |  ${AUTHOR}" \
    --text="Installing ${MOD_NAME}...\nDo not remove the USB drive." --no-cancel &

  set -u
  TAG="[cp-hud install]"
  SELF_DIR=$(dirname "$(readlink -f "$0")")
  SO_SRC="$SELF_DIR/libpatch-blmjcicarplay.so"
  MOD_DIR="/data_persist/cp-hud-mod"
  SO_DST="$MOD_DIR/libpatch-blmjcicarplay.so"
  SO_TOKEN="libpatch-blmjcicarplay"
  PRELOAD_LINE='            <environ_var env_name="LD_PRELOAD" env_value="/data_persist/cp-hud-mod/libpatch-blmjcicarplay.so"/>'
  LDPATH_LINE='            <environ_var env_name="LD_LIBRARY_PATH" env_value="/jci/lib:/usr/lib"/>'
  MASTER="/etc/devmgr_config_master.xml"

  log()  { echo "$TAG $*"; }
  fail() { echo "$TAG ERROR: $*"; killall -q jci-dialog
           /jci/tools/jci-dialog --info --title="${MOD_NAME} V${MOD_VER}  |  ${AUTHOR}" \
             --text="Install FAILED: $*\nSee logs/install.log on the USB." --no-cancel &
           sync; mount -o remount,ro / 2>/dev/null; sleep 15; exit 1; }

  # --- preflight ---
  [ -f "$SO_SRC" ]       || fail "libpatch-blmjcicarplay.so not found on the USB"
  [ -f /jci/sm/sm.conf ] || fail "not a Mazda CMU (no /jci/sm/sm.conf)"
  log "installing from $SO_SRC ($(wc -c < "$SO_SRC") bytes)"

  # --- detect the ACTIVE config the way the firmware itself does -------------
  # /usr/bin/autostart runs get_board_type.sh: exit 2 = WCP hardware -> the SM
  # boots `sm -f sm_WCP.conf`; otherwise `sm -f sm.conf`. Choosing by mere file
  # presence is WRONG -- sm_WCP.conf ships on non-WCP units too, so it would
  # patch a config that is never launched. Run the same probe.
  if [ -x /jci/scripts/get_board_type.sh ]; then
    /jci/scripts/get_board_type.sh >/dev/null 2>&1; BT=$?
  else
    BT=""
  fi
  if [ "$BT" = "2" ] && [ -f /jci/sm/sm_WCP.conf ]; then
    CONFS="/jci/sm/sm_WCP.conf"
    log "Wireless-CarPlay hardware (get_board_type.sh=2) -> patching sm_WCP.conf"
  else
    CONFS="/jci/sm/sm.conf"
    log "no WCP hardware (get_board_type.sh=${BT:-n/a}) -> patching sm.conf"
  fi

  mount -o remount,rw / 2>/dev/null || fail "remount rw failed"

  # --- 1) place the .so on a persistent, boot-visible path ---
  mkdir -p "$MOD_DIR" || fail "mkdir $MOD_DIR failed"
  cp -f "$SO_SRC" "$SO_DST" || fail "copy .so failed"
  chmod 0644 "$SO_DST"
  log "installed $SO_DST"

  # clean any stale LD_PRELOAD in the OTHER config (double-inject crash-loops jciCARPLAY)
  for OTHER in /jci/sm/sm.conf /jci/sm/sm_WCP.conf; do
    case " $CONFS " in *" $OTHER "*) continue;; esac
    [ -f "$OTHER" ] || continue
    if grep -q "$SO_TOKEN" "$OTHER"; then
      [ -f "/data_persist/$(basename "$OTHER").bak_precphud" ] || cp -a "$OTHER" "/data_persist/$(basename "$OTHER").bak_precphud"
      grep -v "$SO_TOKEN" "$OTHER" | grep -v 'LD_LIBRARY_PATH.*jci/lib:/usr/lib' > /tmp/cphud.clean && cp /tmp/cphud.clean "$OTHER"
      rm -f /tmp/cphud.clean
      log "removed stale LD_PRELOAD from $OTHER"
    fi
  done

  # --- 2) patch the chosen config: LD_PRELOAD + LD_LIBRARY_PATH on jciCARPLAY ---
  for CONF in $CONFS; do
    [ -f "$CONF" ] || { log "skip (absent): $CONF"; continue; }
    BAK="/data_persist/$(basename "$CONF").bak_precphud"
    [ -f "$BAK" ] || { cp -a "$CONF" "$BAK"; log "backup -> $BAK"; }

    if grep -q "$SO_TOKEN" "$CONF"; then
      log "$CONF: already patched, skipping insert"
    else
      awk -v pl="$PRELOAD_LINE" -v lp="$LDPATH_LINE" '
        /name="jciCARPLAY"/ { print; print pl; print lp; next } 1
      ' "$CONF" > /tmp/cphud.new || fail "awk failed on $CONF"
      old=$(wc -l < "$CONF"); new=$(wc -l < /tmp/cphud.new)
      if [ "$new" = "$((old+2))" ] && grep -q "$SO_TOKEN" /tmp/cphud.new; then
        cp /tmp/cphud.new "$CONF"; log "$CONF: inserted environ_var ($old->$new lines)"
      else
        log "$CONF: SANITY FAILED ($old->$new), left untouched"
      fi
      rm -f /tmp/cphud.new
    fi
    # a shim fault won't reboot-loop the unit
    sed -i '/name="jciCARPLAY"/ s/reset_board="yes"/reset_board="no"/' "$CONF"
  done

  # --- 3) advertise native-nav over iAP2 ---
  if [ -f "$MASTER" ]; then
    MBAK="/data_persist/$(basename "$MASTER").bak_precphud"
    [ -f "$MBAK" ] || { cp -a "$MASTER" "$MBAK"; log "backup -> $MBAK"; }
    sed -i 's#<name>NaviSupported</name><value>FALSE</value>#<name>NaviSupported</name><value>TRUE</value>#' "$MASTER"
    log "set NaviSupported=TRUE"
  else
    log "WARN: $MASTER absent -- NaviSupported not set"
  fi

  sync
  mount -o remount,ro / 2>/dev/null
  echo "=== ${MOD_NAME} V${MOD_VER} install complete ==="

  killall -q jci-dialog
  /jci/tools/jci-dialog --info --title="${MOD_NAME} V${MOD_VER}  |  ${AUTHOR}" \
    --text="Installation complete!\nYou can remove the USB drive.\nRebooting in 5 seconds..." --no-cancel &

else
  # ==========================================================================
  # UNINSTALL
  # ==========================================================================
  exec > "${MYDIR}/logs/uninstall.log" 2>&1
  echo "=== ${MOD_NAME} V${MOD_VER} uninstall start ($(date)) -- by ${AUTHOR} ==="

  killall -q jci-dialog
  /jci/tools/jci-dialog --info --title="${MOD_NAME} V${MOD_VER}  |  ${AUTHOR}" \
    --text="Uninstalling ${MOD_NAME}...\nDo not remove the USB drive." --no-cancel &

  set -u
  TAG="[cp-hud uninstall]"
  MOD_DIR="/data_persist/cp-hud-mod"
  MASTER="/etc/devmgr_config_master.xml"
  log() { echo "$TAG $*"; }

  [ -f /jci/sm/sm.conf ] || log "not a CMU — attempting cleanup anyway"
  mount -o remount,rw / 2>/dev/null || log "remount rw failed"

  for CONF in /jci/sm/sm.conf /jci/sm/sm_WCP.conf; do
    [ -f "$CONF" ] || continue
    BAK="/data_persist/$(basename "$CONF").bak_precphud"
    if [ -f "$BAK" ]; then
      cp -a "$BAK" "$CONF" && log "restored $CONF from backup"
    elif grep -q 'libpatch-blmjcicarplay' "$CONF" 2>/dev/null; then
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
    log "reset NaviSupported=FALSE (no backup found)"
  fi

  rm -rf "$MOD_DIR" && log "removed $MOD_DIR"
  sync
  mount -o remount,ro / 2>/dev/null
  echo "=== ${MOD_NAME} V${MOD_VER} uninstall complete ==="

  killall -q jci-dialog
  /jci/tools/jci-dialog --info --title="${MOD_NAME} V${MOD_VER}  |  ${AUTHOR}" \
    --text="Uninstall complete!\nYou can remove the USB drive.\nRebooting in 5 seconds..." --no-cancel &
fi

sleep 5
killall -q jci-dialog
sync
reboot
exit 0
