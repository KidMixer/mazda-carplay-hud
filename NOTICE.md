# NOTICE — attribution & copyleft summary

This is an unofficial, community-developed modification. It is **not affiliated with,
endorsed by, or supported by Mazda Motor Corporation or Apple Inc.** "Mazda", "Mazda
Connect", "CarPlay" and related marks belong to their respective owners.

## Copyright (original work)
**Copyright (C) 2026 KID MIXER-MODER** — all source in this repository **except** the
headunit-derived files listed under "Derived code" below. This includes:
- the passive libc `msgrcv` tap of the iAP2 message queue (`devmgr_shim.cpp`),
- the Apple iAP2 `maneuverType` decode + route-window maneuver selection (`nav.cpp`),
- the `jciCARPLAY` process gate and devmgr PLT interposition (`main.cpp`, `devmgr_shim.cpp`),
- the OEM `libjcidbus` / VBS_NAVI HUD transport (`oem/`, `patch.h`, `log.h`) — its signatures
  were hand-derived by reverse-engineering the OEM libraries with Ghidra,
- the cross-compile `Makefile`, the install scripts (`install/`), and the documentation (`docs/`).

## Derived code (AGPL-3.0 — the reason this work is AGPL-3.0)
- **headunit (Trevelopment / Bryan Adams et al.)** — <https://github.com/Trevelopment/headunit>.
  The HUD guidance sender (`mazda/patches/blmjcicarplay/hud/hud_send.cpp` and `hud_send.h`) is
  adapted from it; the vendored CMU D-Bus proxies (`mazda/dbus/`) are taken from it; and the
  cross-compile build pattern follows it. Those files carry `SPDX-License-Identifier:
  AGPL-3.0-or-later` headers referencing the original. This is the only third-party code reused
  here.

## Build dependency (third-party, not part of this work)
- **m3-toolchain** — <https://github.com/lmagder/m3-toolchain> (lmagder) — an independent ARM
  cross-compiler with its own license, referenced via `.gitmodules` (not vendored here).

## License
The whole work is distributed under the **GNU Affero General Public License v3.0** (see
`LICENSE`). AGPL-3.0 is copyleft: derivatives must keep the license and make their source
available to users (including users interacting over a network). It is **required** here
because the work incorporates AGPL code from headunit (above) — not chosen freely.
