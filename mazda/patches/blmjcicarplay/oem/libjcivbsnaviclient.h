// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// OEM D-Bus HUD bindings (libjcivbsnaviclient, layered on libjcidbus).
//
// This header reads like the vendor header that never shipped: just
// the structs and prototypes of the OEM C functions we call. There is
// NO init step and NO global state — each prototype below is backed by
// a self-resolving thunk in libjcivbsnaviclient.cpp that lazily
// dlsym()s the real symbol on first use, caches it, and forwards. Just
// #include and call; an unresolved symbol degrades to a benign failure
// return (0 / -1) instead of crashing.
//
// The connection lifecycle (JCIDBUS_conn_*, worker, the bus selector
// constants) lives in the sibling libjcidbus.h — libjcivbsnaviclient
// is layered on libjcidbus, so we include it here and callers get the
// whole HUD surface from this one header.
//
// All signatures were hand-derived from the Ghidra decompile
// (decompiled/libjcivbsnaviclient.so.c) — no header ships for this
// library. Re-verify after any OEM firmware update.

#ifndef LIBPATCH_BLMJCICARPLAY_LIBJCIVBSNAVICLIENT_H
#define LIBPATCH_BLMJCICARPLAY_LIBJCIVBSNAVICLIENT_H

#include <stdint.h>

#include "libjcidbus.h"   // connection lifecycle + kJci*Bus selectors

// 12-byte com.jci.vbs.navi HUD frame, signature (uqyqyy). Field
// offsets verified against VBS_NAVI_HUD_Display_s_t_pack:
// u32@0, u16@4, u8@6, u16@8, u8@10, u8@11 (sizeof == 12 with the
// natural pad byte at offset 7).
struct VbsNaviHudDisplay {
    uint32_t nextManeuverInfo;
    uint16_t distanceValue;
    uint8_t  distanceUnit;
    uint16_t displaySpeedLimit;
    uint8_t  displaySpeedUnit;
    uint8_t  text_ID3;
};

// 8-byte com.jci.vbs.navi.tmc street-name struct, signature (sy):
// char*@0, u8@4. The library transcodes UTF-8 -> UCS-2 and pages
// it internally; we just hand it a NUL-terminated C string.
struct VbsNaviHudMsg2 {
    const char *guidancePointName;
    uint8_t     syncBit;
};

// --- HUD setters ----------------------------------------------
// Setters: (conn, &struct, unused, completion_cb, user) -> int
// (0 = queued OK). Pass NULL completion_cb/user when no reply needed.
// GetHUDStatus(conn, cb, user); the reply callback is invoked as
// cb(conn, status_byte, user) — a zero status byte means the HUD is
// not installed / not powered.
int VBS_NAVI_SetHUDDisplayMsgReq(void *conn, VbsNaviHudDisplay *disp,
                                 void *unused, void *cb, void *user);
int VBS_NAVI_TMC_SetHUD_Display_Msg2(void *conn, VbsNaviHudMsg2 *msg2,
                                     void *unused, void *cb, void *user);
int VBS_NAVI_GetHUDStatus(void *conn, void *cb, void *user);

#endif  // LIBPATCH_BLMJCICARPLAY_LIBJCIVBSNAVICLIENT_H
