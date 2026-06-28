// SPDX-License-Identifier: AGPL-3.0-or-later
//
// Adapted from headunit (https://github.com/Trevelopment/headunit),
// licensed under GNU AGPL v3.
// See NOTICE.md at the repo root for the full attribution.
//
// Producer-side interface for the HUD sender.
// HUD output module — push interface used by nav.cpp.
//
// Three small non-blocking functions the AAP nav callback in
// `nav.cpp` calls once per phone-side event. Each updates a
// seqlock-protected snapshot inside `hud_send.cpp` and wakes the
// dedicated sender thread that owns the OEM D-Bus connections.
//
// Lifecycle (start/stop) is declared in patch.h alongside the
// touch reader's start/stop — same lifecycle bracket fires both
// from the aap_create_session / aap_destroy_session PLT shims in
// lifecycle.cpp.
//
// Implementation reference: `mazda/hud/{hud.h,hud.cpp}` in the
// upstream headunit project (see the SPDX header at the top of
// hud_send.cpp for the URL). We mirror the proxy classes and the
// turn-icon lookup table from that implementation but adapted to
// live inside an LD_PRELOAD library rather than the headunit's
// own daemon.

#ifndef LIBPATCH_BLMJCICARPLAY_HUD_SEND_H
#define LIBPATCH_BLMJCICARPLAY_HUD_SEND_H

#include <stdint.h>

// 0x500 NAVMessagesStatus. `status` per hu.proto: 1=START, 2=STOP.
// Anything else is treated as STOP (defensive).
void hud_on_status(uint32_t status);

// 0x501 NAVTurnMessage. `road_name` may be NULL or empty.
//   turn_side  - 1=LEFT, 2=RIGHT, 3=UNSPECIFIED (proto TURN_SIDE)
//   turn_event - sparse 0..19  (proto TURN_EVENT)
//   turn_angle - degrees, signed
//   turn_number - maneuver / exit number
//
// road_name lifetime: NOT held past return. The pointer is
// snapshot-copied into a fixed-size buffer inline.
void hud_on_next_turn(const char *road_name,
                      uint32_t    turn_side,
                      uint32_t    turn_event,
                      int32_t     turn_angle,
                      int32_t     turn_number);

// 0x502 NAVDistanceMessage.
//   distance_meters  - raw meters (proto `distance`)
//   time_until_seconds - ETA seconds (proto `time_until`)
//   display_distance - the "raw unit * 1000" int32 from the SDK
//                      header (truncated from proto uint64). The
//                      HUD wire format wants "raw unit * 10", so
//                      we divide by 100 on the way in.
//   display_distance_unit - proto DISPLAY_DISTANCE_UNIT, 1..6.
//                           Mapped to the HUD's own unit enum
//                           inside hud_send.cpp.
void hud_on_distance(int32_t  distance_meters,
                     int32_t  time_until_seconds,
                     int32_t  display_distance,
                     uint32_t display_distance_unit);

#endif // LIBPATCH_BLMJCICARPLAY_HUD_SEND_H
