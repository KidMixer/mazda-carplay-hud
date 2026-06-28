# Installing the CarPlay → HUD bridge

For a Mazda Connect CMU (`MAZ_CMU-150`, FW 74.00.324; CX-5 KF / CX-8 2018 and
same-platform units). Packaged the same way as the OEM Android-Auto mod
(`LD_PRELOAD` shim + optional USB autorun).

## Files in this folder
| File | Role |
|---|---|
| `install.sh` | Main installer (idempotent, backs up before editing, WCP-aware). |
| `uninstall.sh` | Restores stock from backups. |
| `run.sh` | USB-autorun wrapper — calls `install.sh`, shows on-screen progress. |
| `libpatch-blmjcicarplay.so` | The bridge — **build it first** (see [`../mazda`](../mazda)). |

> Build the `.so` with `make` in `../mazda`, then copy
> `mazda/build/release/libpatch-blmjcicarplay.so` into this folder before installing.

## What `install.sh` does
1. Copies `libpatch-blmjcicarplay.so` → `/data_persist/cp-hud-mod/`.
2. Adds two `<environ_var>` to the **jciCARPLAY** service — `LD_PRELOAD` (the shim)
   and `LD_LIBRARY_PATH` (`/jci/lib:/usr/lib`). It patches **`sm_WCP.conf`** on
   wireless-CarPlay units, otherwise **`sm.conf`** — never both (the OEM service
   manager merges the two, and a duplicate `LD_PRELOAD` crash-loops jciCARPLAY).
   Sets `reset_board="no"` on jciCARPLAY so a shim fault can't reboot-loop the unit.
3. Sets `NaviSupported=TRUE` in `/etc/devmgr_config_master.xml` (so the unit
   advertises native-nav capability over iAP2 and Apple starts streaming maneuvers).
4. Backs up every edited file to `/data_persist/*.bak_precphud` first.

Everything is on the read-only relfs rootfs (remounted rw only briefly) — it
**cannot brick** the unit; worst case a reboot returns it to stock.

## Method A — SSH (recommended)
```sh
# from your PC, copy this folder to the unit (default creds jci / jci, lands as root):
pscp -scp -r install jci@192.168.53.1:/tmp/
ssh jci@192.168.53.1
cd /tmp/install && sh install.sh
reboot
```

## Method B — USB stick
1. Format a USB stick FAT32.
2. Copy `run.sh`, `install.sh`, and the built `libpatch-blmjcicarplay.so` to the **root** of the stick.
3. Insert the stick (engine on / ACC). The autorun framework runs `run.sh`; a dialog shows progress.
4. When it says installed, **reboot**.

## Verify it loaded (after reboot, over SSH)
```sh
P=$(ps | grep '[L]_jciCARPLAY' | awk '{print $1}')
tr '\0' '\n' < /proc/$P/maps | grep -c libpatch-blmjcicarplay   # expect > 0
```
Then connect an iPhone and start turn-by-turn navigation (Apple/Google/any CarPlay
nav app). The maneuver + distance + road name appear on the instrument-cluster HUD.
A **debug build** additionally logs to `/tmp/carplay_bridge.log`
(`=== loaded ===`, `arm_navi ...`, `nav maneuver: "..." -> HUD`).

## Uninstall / revert to stock
```sh
cd /tmp/install && sh uninstall.sh && reboot
```

## Notes
- Independent of the Android-Auto mod (own `/data_persist/cp-hud-mod/` dir and
  `*.bak_precphud` backups), so it won't clash with an existing Android-Auto mod install.
- Re-running `install.sh` is safe (idempotent — won't duplicate lines).
