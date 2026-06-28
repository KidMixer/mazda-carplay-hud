// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// Shared logging helper used across the patch's translation units.

#ifndef LIBPATCH_BLMJCICARPLAY_LOG_H
#define LIBPATCH_BLMJCICARPLAY_LOG_H

#include <stdint.h>
#include <cstdio>

// Coarse log helpers. The launcher PID's stderr is captured by sm
// into the device log. Each message carries two tags — the subsystem
// it came from and a level letter — so they're easy to grep:
//
//   [libpatch-blmjcicarplay][TOUCH][V] verbose — per-frame / per-event touch chatter
//   [libpatch-blmjcicarplay][CORE][D]  debug   — lifecycle, one-shot resolution results
//   [libpatch-blmjcicarplay][HUD][W]   warn    — recoverable oddities (e.g. SYN_DROPPED)
//   [libpatch-blmjcicarplay][TOUCH][E] error   — recoverable failures (open(), poll())
//   [libpatch-blmjcicarplay][CORE][C]  critical— shim cannot function (dlsym(RTLD_NEXT) miss)
//
// The subsystem tag comes from LOG_TAG. Define it before including
// this header to label every line a translation unit emits, e.g.
//
//   #define LOG_TAG "HUD"
//   #include "../log.h"
//
// Files that don't set one fall back to "CORE". Call sites are
// unchanged — LOGD("..."), LOGE("..."), etc. — the subsystem is
// injected automatically.
//
// Levels are compile-time-gated by LOG_LEVEL. Anything below
// the threshold expands to an `if (false)` guard so the format string
// is still type-checked but the call is never executed (and is
// trivially dead-code-eliminated even at -O0). The Makefile passes
// -DDEBUG for the debug config and -DNDEBUG for release; we key off
// those: debug => VERBOSE, release => ERROR.
//
// We deliberately don't depend on a fancy logger — the OEM
// COMMON_Log_Write is reachable via the same anchor-and-offset trick,
// but pulling it in would add 2 more offset-bound symbols for no
// real benefit during bring-up.

#define LOG_LEVEL_VERBOSE  0
#define LOG_LEVEL_DEBUG    1
#define LOG_LEVEL_WARN     2
#define LOG_LEVEL_ERROR    3
#define LOG_LEVEL_CRITICAL 4
#define LOG_LEVEL_SILENT   5

#ifndef LOG_LEVEL
#  ifdef NDEBUG
#    define LOG_LEVEL LOG_LEVEL_ERROR
#  else
#    define LOG_LEVEL LOG_LEVEL_VERBOSE
#  endif
#endif

// Subsystem tag for this translation unit. Define before including
// log.h to override; defaults to "CORE" for the loader / lifecycle TUs.
#ifndef LOG_TAG
#  define LOG_TAG "CORE"
#endif

#define LOG_EMIT(tag, fmt, ...)                                            \
    do {                                                                   \
        std::fprintf(stderr,                                               \
                     "[libpatch-blmjcicarplay][" LOG_TAG "][" tag "] " fmt "\n", \
                     ##__VA_ARGS__);                                       \
        std::fflush(stderr);                                               \
    } while (0)

// Keep the format string type-checked even when the level is
// compiled out. `if (false)` is folded by the compiler at all
// optimisation levels, so neither the fprintf nor the argument
// expressions are evaluated at runtime.
#define LOG_DROP(fmt, ...)                                                 \
    do {                                                                   \
        if (false) {                                                       \
            std::fprintf(stderr, "[libpatch-blmjcicarplay][" LOG_TAG "] " fmt "\n", \
                         ##__VA_ARGS__);                                   \
        }                                                                  \
    } while (0)

#if LOG_LEVEL <= LOG_LEVEL_VERBOSE
#  define LOGV(fmt, ...) LOG_EMIT("V", fmt, ##__VA_ARGS__)
#else
#  define LOGV(fmt, ...) LOG_DROP(fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#  define LOGD(fmt, ...) LOG_EMIT("D", fmt, ##__VA_ARGS__)
#else
#  define LOGD(fmt, ...) LOG_DROP(fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#  define LOGW(fmt, ...) LOG_EMIT("W", fmt, ##__VA_ARGS__)
#else
#  define LOGW(fmt, ...) LOG_DROP(fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#  define LOGE(fmt, ...) LOG_EMIT("E", fmt, ##__VA_ARGS__)
#else
#  define LOGE(fmt, ...) LOG_DROP(fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL <= LOG_LEVEL_CRITICAL
#  define LOGC(fmt, ...) LOG_EMIT("C", fmt, ##__VA_ARGS__)
#else
#  define LOGC(fmt, ...) LOG_DROP(fmt, ##__VA_ARGS__)
#endif

#endif // LIBPATCH_BLMJCICARPLAY_LOG_H
