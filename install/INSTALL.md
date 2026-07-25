# Installing CarPlay navigation on the HUD
### ⭐ PATCH BY: KID MIXER-MODER

Mazda Connect CMU (`MAZ_CMU-150`, FW 74.00.324; CX-5 KF / CX-8 2018 and
same-platform units). `LD_PRELOAD` shim + USB autorun, like the OEM Android-Auto mod.
**Auto-detects** Wireless-CarPlay (`sm_WCP.conf`) vs wired CarPlay (`sm.conf`) — no manual choice.

## Files
| File | Role |
|---|---|
| `tweaks.sh` | **USB installer** — on-screen **Install / Uninstall** menu; auto-detects WCP/wired, then reboots. |
| `install.sh` | Installer core (idempotent, backs up, WCP-aware) — used by the SSH method. |
| `uninstall.sh` | Restores stock from backups — SSH method. |
| `dataRetrieval_config.txt` | Points the DataRetrieval loader at `tweaks.sh`. |
| `jci-autoupdate` | Flag file that arms the autorun. |
| `libpatch-blmjcicarplay.so` | The bridge (ARM 32-bit) — build with `make` (see README); not committed to the repo. |

> `cmu_dataretrieval.up` (the signed DataRetrieval engine) is **not shipped** here —
> it is a Mazda diagnostic tool. Supply your own for the USB method, or use SSH
> (Method A, which needs no `.up`). Both end in the same on-disk state.

## Method A — SSH (recommended; no `cmu_dataretrieval.up` needed)
```sh
pscp -scp -r install jci@192.168.53.1:/tmp/
ssh jci@192.168.53.1
cd /tmp/install && sh install.sh      # to remove: sh uninstall.sh
reboot
```

## Method B — USB stick (on-screen menu, auto-reboot)
1. Format a USB stick FAT32.
2. Copy every file here (including your own `cmu_dataretrieval.up`) to the stick **root**.
3. Insert the stick (engine on / ACC). A dialog appears on the centre display:
   - **Install** → installs the bridge (auto-detects WCP/wired) and reboots.
   - **Uninstall** → restores stock and reboots.
4. Progress shows on screen; the full log is written to `logs/` **on the stick**
   (`install.log` / `uninstall.log`). The unit reboots itself when done.

## Verify (after reboot, over SSH)
```sh
P=$(ps | grep '[L]_jciCARPLAY' | awk '{print $1}')
tr '\0' '\n' < /proc/$P/maps | grep -c libpatch-blmjcicarplay   # expect > 0
```
Connect an iPhone and start turn-by-turn navigation; the maneuver + distance +
road name appear on the instrument-cluster HUD.

## Notes
- Independent of the Android-Auto mod (own `/data_persist/cp-hud-mod/` dir and
  `*.bak_precphud` backups).
- Re-running is safe (idempotent — won't duplicate lines).
