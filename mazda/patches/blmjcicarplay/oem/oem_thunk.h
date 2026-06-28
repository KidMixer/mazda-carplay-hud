// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// Shared self-resolving-thunk macros for the hand-written OEM bindings
// under oem/. Each binding .cpp provides its own file-local resolver:
//
//   void *oem_sym(const char *name);   // dlsym from the right library
//
// then declares each OEM prototype's body with these macros. A thunk
// resolves the real OEM symbol of the same name on first call, caches
// it in a function-local static (C++11 "magic static" — thread-safe
// init), and forwards. An unresolved symbol degrades to the supplied
// benign failure value instead of crashing.
//
// `params` and `args` are parenthesised lists, e.g.
//   OEM_THUNK(int, Foo, (void *a, int b), (a, b), -1)
//
// The thunks are ordinary (mangled, hidden-visibility) C++ functions,
// so even though they reuse the OEM names they do NOT interpose the
// real C symbols from this LD_PRELOAD .so — `dlsym(handle, "<name>")`
// inside oem_sym still fetches the genuine library function.

#ifndef LIBPATCH_BLMJCICARPLAY_OEM_THUNK_H
#define LIBPATCH_BLMJCICARPLAY_OEM_THUNK_H

#define OEM_THUNK(ret, name, params, args, failval) \
	ret name params \
	{ \
		using Fn = ret (*) params; \
		static Fn fn = reinterpret_cast<Fn>(oem_sym(#name)); \
		return fn ? fn args : (failval); \
	}

#define OEM_THUNK_VOID(name, params, args) \
	void name params \
	{ \
		using Fn = void (*) params; \
		static Fn fn = reinterpret_cast<Fn>(oem_sym(#name)); \
		if (fn) fn args; \
	}

#endif  // LIBPATCH_BLMJCICARPLAY_OEM_THUNK_H
