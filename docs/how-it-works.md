# How it works — data path & maneuverType catalogue

## The problem
Mazda Connect (CMU150, FW 74.00.324) renders Android Auto and native navigation on the
instrument-cluster HUD, but **never CarPlay turn-by-turn**. An exhaustive search of the
firmware shows there is no Apple-iAP2 `maneuverType` → HUD-icon mapping anywhere on the
unit: the OEM `jciCARPLAY.so` "TurnByTurn" subsystem is status/arbitration only (a boolean
"who owns TBT"), carrying no geometry. The head unit was simply never built to draw CarPlay
maneuvers — it relies on phone-screen mirroring.

So the maneuver decode must live in **our** bridge.

## Data path
```
iPhone (CarPlay, actively navigating)
   │  Apple iAP2 over USB/wireless
   ▼
usr/bin/ipoddev_full_auth        (OEM iAP2 producer)
   │  viAP2NavRouteManeuverUpdate -> SysV message queue (msgsnd)
   ▼
jciCARPLAY  (OEM process)  ◄── our shim is LD_PRELOAD'ed in here
   │  the OEM receive thread calls libc msgrcv() to pull each event
   │
   ├── devmgr_shim.cpp: we interpose msgrcv() (PLT/preload). Every message the OEM
   │   receives also passes through us — we COPY-READ it and always chain the real call
   │   (fully passive, zero crash risk, never alters the data).
   │      maneuver burst = type 0x8059 (840 B)
   │      guidance       = type 0x8058 (1628 B)
   │
   ├── nav.cpp: decode maneuverType / distance / road-name, pick the first REAL maneuver
   │
   ▼
hud/hud_send.cpp -> com.jci.vbs.navi  (OEM HUD nav D-Bus service, via libjcidbus)
   │   VBS_NAVI_SetHUDDisplayMsgReq (+ SetHUD_Display_Msg2)
   ▼
Instrument-cluster HUD  → arrow + distance + road name
```

`NaviSupported=TRUE` in `/etc/devmgr_config_master.xml` is what makes Apple advertise +
stream the maneuver data over iAP2 in the first place — the installer sets it.

## Activation / gating (no DRM)
- **`main.cpp` constructor** reads `/proc/self/cmdline`; it only arms (`g_enabled=true`)
  if the process is the `jciCARPLAY` launcher (`in_carplay_launcher`). In any other process
  that happens to inherit `LD_PRELOAD`, the shim stays completely inert and chains straight
  through to the real OEM functions.
- The PLT shims for `eDevMgriPodStartCallStateUpdates` / `…CommunicationUpdates` /
  `…SendLocationInformation` call `arm_navi()` when a CarPlay session sets up; that brings up
  the HUD sender and flips `g_armed` so the passive `msgrcv` tap begins decoding maneuvers.

## iAP2 nav message model (reverse-engineered)
- **MANEUVER `0x8059` (840 B)** — a *window* of the next ~5 maneuvers (idx 0..N):
  - `index`             = `u32 @ 0x20 >> 16`
  - `maneuverDesc`      = text @ `0x24` (256 B) — the live instruction text
  - `maneuverType`      = `u32 @ 0x124`
  - `distBetweenManeuver` = `u32 @ 0x128`
  - `afterManeuverRoadName` = text @ `0x12c` (256 B)
- **GUIDANCE `0x8058` (1628 B)** — `distToNextManeuver` (metres, counts down) @ `0x34c`.
- **Selection:** idx 0 is the current heading/segment (depart / continue / head-compass)
  and is never displayed. `distToNextManeuver` counts down to the first *real* maneuver in
  the window; when it is passed the window slides (idx 0 becomes the new heading). So the
  bridge latches the first real maneuver's icon + the road it leads onto, paired with the
  live distance.

## maneuverType catalogue (confirmed from real-drive captures)
Apple encodes the turn **side** inside `maneuverType` (e.g. 11 vs 12 are distinct values);
there is no separate left/right field.

| value | meaning | HUD treatment |
|------:|---------|---------------|
| 8  | continue / on-road (`desc` = road name) | CONTEXT (skip) |
| 11 | "Rẽ trái" — turn left | TURN-LEFT |
| 12 | "Rẽ phải" — turn right | TURN-RIGHT |
| 19 | "về phía X" — depart / proceed toward | CONTEXT |
| 21 | continue onto road | CONTEXT |
| 24 / 25 / 27 | head North / East / South-West (compass) | CONTEXT |
| 34 | name-change onto road (`desc` = road name) | CONTEXT |
| 45 | "về phía ĐCT…" — ramp / merge onto expressway | SLIGHT |
| 56 | fork (multi-road choice) | FORK |

"CONTEXT" entries are leading route context that must be skipped so the distance/road-name
track the next *actionable* maneuver. The set above is not exhaustive — new types are
catalogued by reading the `desc` field of an existing full-drive capture against its
`maneuverType` (the `desc` is the live Maps instruction text, so the mapping is direct).

## Why a passive `msgrcv` tap (and not the OEM vtable)
The OEM receive thread inside `jciCARPLAY` pulls every devmgr event through the libc
`msgrcv` PLT entry. Interposing `msgrcv` lets the bridge *see* every message — maneuvers
included — while always forwarding the real call unchanged. It never modifies the OEM
dispatch, vtable, or data, so it cannot crash or destabilise the OEM process: it is a pure
read-side tap.
