# Installing CarPlay navigation on the HUD
### ⭐ PATCH BY: KID MIXER-MODER

Mazda Connect CMU (`MAZ_CMU-150`, FW 74.00.324; CX-5 KF / CX-8 2018 and
same-platform units). `LD_PRELOAD` shim + USB autorun, like the OEM Android-Auto mod.

## Files
| File | Role |
|---|---|
| `install.sh` | Main installer (idempotent, backs up before editing, WCP-aware). Shared by both methods. |
| `uninstall.sh` | Restores stock from backups. |
| `tweaks.sh` | USB-autorun payload — runs `install.sh`, shows progress, then reboots. |
| `dataRetrieval_config.txt` | Points the DataRetrieval loader at `tweaks.sh`. |
| `jci-autoupdate` | Flag file that arms the autorun. |
| `libpatch-blmjcicarplay.so` | The bridge (ARM 32-bit) — build with `make` (see README); not committed to the repo. |

> `cmu_dataretrieval.up` (the signed DataRetrieval engine) is **not shipped** here —
> it is a Mazda diagnostic tool. Supply your own for the USB method, or use SSH
> (Method A). Both end in the same on-disk state.

## Method A — SSH (recommended)
```sh
pscp -scp -r install jci@192.168.53.1:/tmp/
ssh jci@192.168.53.1
cd /tmp/install && sh install.sh
reboot
```

## Method B — USB stick (auto-install + auto-reboot)
1. Format a USB stick FAT32.
2. Copy every file here (including your `cmu_dataretrieval.up`) to the stick **root**.
3. Insert the stick (engine on / ACC). Progress shows on the centre display; the
   full log is written to `CP-HUD_log.txt` **on the stick**.
4. The unit reboots itself when done — then remove the USB.

## Verify (after reboot, over SSH)
```sh
P=$(ps | grep '[L]_jciCARPLAY' | awk '{print $1}')
tr '\0' '\n' < /proc/$P/maps | grep -c libpatch-blmjcicarplay   # expect > 0
```
Connect an iPhone and start turn-by-turn navigation; the maneuver + distance +
road name appear on the instrument-cluster HUD.

## Uninstall
```sh
cd /tmp/install && sh uninstall.sh && reboot
```

## Notes
- Independent of the Android-Auto mod (own `/data_persist/cp-hud-mod/` dir and
  `*.bak_precphud` backups).
- Re-running `install.sh` / `tweaks.sh` is safe (idempotent — won't duplicate lines).
