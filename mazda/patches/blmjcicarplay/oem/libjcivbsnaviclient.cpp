// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// Self-resolving thunks for the OEM HUD setters (libjcivbsnaviclient).
// See libjcivbsnaviclient.h for the rationale and the hand-derived
// ABI, and oem_thunk.h for the OEM_THUNK macro.

#define LOG_TAG "LIBJCIVBSNAVICLIENT"
#include "../log.h"

#include "libjcivbsnaviclient.h"
#include "oem_thunk.h"

#include <dlfcn.h>

namespace {

// Resolve one symbol by name. The OEM client lib is normally already
// resident (transitive DT_NEEDED via the lds-dbus client libs); we
// still dlopen it (RTLD_NOW|RTLD_GLOBAL, refcount bump) once so we
// don't depend on that incidental chain across firmware versions, and
// fall back to the global scope if the dlopen didn't take.
void *oem_sym(const char *name)
{
	static void *handle = dlopen("libjcivbsnaviclient.so",
	                             RTLD_NOW | RTLD_GLOBAL);
	void *sym = dlsym(handle ? handle : RTLD_DEFAULT, name);
	if (sym == nullptr) {
		LOGC("oem: dlsym(%s) failed: %s", name, dlerror());
	}
	return sym;
}

} // namespace

OEM_THUNK(int, VBS_NAVI_SetHUDDisplayMsgReq,
          (void *conn, VbsNaviHudDisplay *disp, void *unused, void *cb, void *user),
          (conn, disp, unused, cb, user), -1)
OEM_THUNK(int, VBS_NAVI_TMC_SetHUD_Display_Msg2,
          (void *conn, VbsNaviHudMsg2 *msg2, void *unused, void *cb, void *user),
          (conn, msg2, unused, cb, user), -1)
OEM_THUNK(int, VBS_NAVI_GetHUDStatus,
          (void *conn, void *cb, void *user), (conn, cb, user), -1)
