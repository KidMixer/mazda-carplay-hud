# CarPlay → HUD bridge for Mazda Connect (CMU150)

`LD_PRELOAD` shims for the Mazda CMU infotainment system that put **Apple CarPlay
turn-by-turn navigation on the instrument-cluster Head-Up Display (HUD)** — the way the
community Android-Auto HUD mods bring Android Auto guidance to the same display.

By **KID MIXER-MODER**. The HUD guidance sender + CMU D-Bus proxies are adapted from
[Trevelopment/headunit](https://github.com/Trevelopment/headunit) (AGPL-3.0, which is why this
is too); everything else — the iAP2 decode, the OEM transport (reverse-engineered with Ghidra),
the build files — is original to this project.

Stock Mazda Connect renders Android Auto and native navigation on the HUD but **never
CarPlay** (CarPlay is only screen-mirrored). This bridge decodes the Apple iAP2
turn-by-turn stream inside the OEM `jciCARPLAY` process and forwards each maneuver —
arrow, distance, road name — to the OEM navigation HUD D-Bus API.

- **HUD navigation only** — maneuver + distance + road name (no posted speed-limit sign).
- **No licensing / DRM / network** code — just the bridge.
- Supported: Mazda Connect **CMU150**, FW **74.00.324** (CX-5 KF, CX-8 2018 and
  same-platform units); wired and wireless CarPlay (WCP).

## Repository layout
```
mazda/
  Makefile                       cross-compile rules (m3-toolchain)
  m3-toolchain/                  ARM cross-compiler (git submodule)
  dbus/                          vendored CMU D-Bus proxy headers (from headunit)
  patches/blmjcicarplay/
    main.cpp                     LD_PRELOAD entry + jciCARPLAY process gate + crash handler
    devmgr_shim.cpp              devmgr PLT shims + passive msgrcv iAP2 tap
    nav.cpp                      iAP2 maneuver decode + route-window selection
    hud/hud_send.cpp, hud_send.h HUD D-Bus sender (com.jci.vbs.navi)   [derived from headunit]
    oem/                         OEM transport glue (libjcidbus, VBS_NAVI client)
    patch.h, log.h               shared helpers
install/                         install.sh / uninstall.sh / run.sh / INSTALL.md
docs/how-it-works.md             data path + maneuverType catalogue
```

## Building from source
```sh
git clone --recursive <this-repo>
# or, if cloned without --recursive:
git submodule update --init --recursive

cd mazda
make            # -> build/release/libpatch-blmjcicarplay.so
make debug      # unstripped; logs to /tmp/carplay_bridge.log
make clean
```
The default `make` produces the navigation-only HUD shim. Build options live in the
`Makefile` (`FEATURE_FLAGS`): `CARPLAY_NO_SPLIM` (on) and `CARPLAY_VN_NORMALIZE`
(strip Vietnamese diacritics for the HUD font — set to 0 for other locales).

## Install
Copy the built `libpatch-blmjcicarplay.so` next to the install scripts and run them on
the unit — see [`install/INSTALL.md`](install/INSTALL.md). The installer `LD_PRELOAD`s the
shim into `jciCARPLAY` (sm_WCP.conf on wireless units, sm.conf otherwise), sets
`NaviSupported=TRUE`, and backs up every edited file. Reversible; it cannot brick the unit
(read-only relfs — worst case a reboot returns it to stock).

## Toolchain notes
The cross-compiler is the [`m3-toolchain`](https://github.com/lmagder/m3-toolchain)
submodule — an independent, third-party ARM cross-compiler (triple
`arm-cortexa9_neon-linux-gnueabi`). Build flags match the device's runtime: `-march=armv7-a -mtune=cortex-a9 -mfpu=neon`
and `-D_GLIBCXX_USE_CXX11_ABI=0` (the CMU ships the GCC-4.6-era libstdc++ COW std::string
ABI). The shim links no `dbus-c++` (it talks to the HUD via the OEM `libjcidbus` transport);
`-Wl,--as-needed` drops the unused dependency.

## License & attribution
Licensed under the **GNU Affero General Public License v3.0** — see [`LICENSE`](LICENSE)
and [`NOTICE.md`](NOTICE.md). AGPL-3.0 is inherited from the upstream projects (copyleft).

- **All code in this repo** — the CarPlay iAP2 decode, the `jciCARPLAY` interposition, the OEM
  `libjcidbus`/VBS_NAVI transport (reverse-engineered from the OEM libraries with Ghidra), the
  **`Makefile`**, the install scripts and docs — © 2026 **KID MIXER-MODER**, except the derived
  code below.
- **Derived code (AGPL-3.0)** — the HUD guidance sender (`hud/hud_send.*`) is adapted from, and
  the CMU D-Bus proxies (`dbus/`) are vendored from,
  [Trevelopment/headunit](https://github.com/Trevelopment/headunit). This is the only reused
  code, and the reason the whole work is AGPL-3.0.
- **`m3-toolchain`** — the ARM cross-compiler is an independent third-party submodule
  ([lmagder/m3-toolchain](https://github.com/lmagder/m3-toolchain), its own license), not part of this work.

Per-file `SPDX-License-Identifier` headers mark which is which. See [`AUTHORS`](AUTHORS).

## Disclaimer
Unofficial, community-developed modification — **not affiliated with, endorsed by, or
supported by Mazda Motor Corporation or Apple Inc.** "Mazda", "Mazda Connect", "CarPlay"
and related marks belong to their respective owners. For research and personal use on your
own vehicle, at your own risk. No warranty.

## How it works
Short version: the shim is `LD_PRELOAD`ed into `jciCARPLAY`; it passively taps the libc
`msgrcv` the OEM uses to receive iAP2 events, decodes the maneuver (`0x8059`) / guidance
(`0x8058`) bursts, and pushes the result to `com.jci.vbs.navi`. Full data path and the
`maneuverType` catalogue: [`docs/how-it-works.md`](docs/how-it-works.md).
