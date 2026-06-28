// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// libjcidbus connection-lifecycle bindings.
//
// This header reads like the vendor header that never shipped: just
// the prototypes of the OEM C functions we call. There is NO init step
// and NO global state — each prototype is backed by a self-resolving
// thunk in libjcidbus.cpp (see oem_thunk.h). Just #include and call;
// an unresolved symbol degrades to a benign failure return (NULL / 0)
// instead of crashing.
//
// Why we use libjcidbus at all (instead of dbus-c++): dbus-c++ leaves
// libdbus's "exit-on-disconnect" flag at its default (TRUE), so when
// the private service-bus socket is torn down — e.g. on an Android-
// Auto <-> CarPlay source switch — libdbus raises a fatal signal and
// kills the whole {L_jciCARPLAY} process (the self-directed SIGSEGV seen
// in three crash cores). libjcidbus's own connection setup calls
// dbus_connection_set_exit_on_disconnect(conn, FALSE) for us and
// exposes a clean JCIDBUS_worker_stop() teardown.
//
// All signatures were hand-derived from the Ghidra decompile
// (decompiled/libjcidbus.so.c) — no header ships for this library.
// Re-verify after any OEM firmware update.

#ifndef LIBPATCH_BLMJCICARPLAY_LIBJCIDBUS_H
#define LIBPATCH_BLMJCICARPLAY_LIBJCIDBUS_H

// JCIDBUS_conn_connect bus selector (decompiled libjcidbus:
// 0 = SERVICE bus / $JCI_SERVICE_BUS, 1 = HMI bus / $JCI_HMI_BUS).
constexpr int kJciServiceBus = 0;
constexpr int kJciHmiBus     = 1;

// --- connection lifecycle -------------------------------------
// conn_create returns the connection handle (NULL on failure).
// conn_connect returns NON-zero on success (acquires the well-known
// name and, internally, disables exit-on-disconnect). The remaining
// calls are fire-and-forget teardown.
void *JCIDBUS_conn_create(void *error_cb, int reserved);
int   JCIDBUS_conn_connect(void *conn, const char *name,
                           int jcibus, void *reserved);
int   JCIDBUS_worker_start(void *conn);
void  JCIDBUS_worker_stop(void *conn);
void  JCIDBUS_conn_disconnect(void *conn);
void  JCIDBUS_conn_free(void *conn);

#endif  // LIBPATCH_BLMJCICARPLAY_LIBJCIDBUS_H
