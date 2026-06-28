// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// Self-resolving thunks for libjcidbus connection lifecycle.
// See libjcidbus.h for the rationale and the hand-derived ABI, and
// oem_thunk.h for the OEM_THUNK / OEM_THUNK_VOID macros.

#define LOG_TAG "LIBJCIDBUS"
#include "../log.h"

#include "libjcidbus.h"
#include "oem_thunk.h"

#include <dlfcn.h>

namespace {

// Resolve one libjcidbus symbol by name. libjcidbus is a direct
// DT_NEEDED of blmjciaapa (always resident), but we still dlopen it
// once (RTLD_NOW|RTLD_GLOBAL, refcount bump) so resolution is robust
// regardless of how the process happens to be linked, and fall back
// to the global scope if the dlopen didn't take.
void *oem_sym(const char *name)
{
	static void *handle = dlopen("libjcidbus.so", RTLD_NOW | RTLD_GLOBAL);
	void *sym = dlsym(handle ? handle : RTLD_DEFAULT, name);
	if (sym == nullptr) {
		LOGC("oem: dlsym(%s) failed: %s", name, dlerror());
	}
	return sym;
}

} // namespace

OEM_THUNK(void *, JCIDBUS_conn_create,
          (void *error_cb, int reserved), (error_cb, reserved), nullptr)
OEM_THUNK(int, JCIDBUS_conn_connect,
          (void *conn, const char *name, int jcibus, void *reserved),
          (conn, name, jcibus, reserved), 0)
OEM_THUNK(int, JCIDBUS_worker_start, (void *conn), (conn), -1)
OEM_THUNK_VOID(JCIDBUS_worker_stop, (void *conn), (conn))
OEM_THUNK_VOID(JCIDBUS_conn_disconnect, (void *conn), (conn))
OEM_THUNK_VOID(JCIDBUS_conn_free, (void *conn), (conn))
