// SPDX-License-Identifier: AGPL-3.0-or-later
//
// CarPlay -> HUD bridge for Mazda Connect (CMU150).
// Copyright (C) 2026 KID MIXER-MODER.
//
// Part of an AGPL-3.0 work - see NOTICE.md for full attribution.
//
// Patch entry point for the CarPlay->HUD bridge. Runs at LD_PRELOAD time,
// before sm_svclauncher dlopen's /jci/carplay/blmjcicarplay.so.
//
// Self-gate: unlike blmjciaapa.so (which exports GetServiceInterfaces and can
// be probed with dlopen+dlsym), blmjcicarplay.so exports no usable anchor, so
// we gate on /proc/self/cmdline instead — the launcher is invoked as
//   {L_jciCARPLAY} /jci/sm/sm_svclauncher -l jciCARPLAY /jci/carplay/blmjcicarplay.so 0 -a
// If we are not in that PID we stay completely inert (the PLT shims in
// devmgr_shim.cpp still chain straight through to the real libdevmgr_interface
// impls, so any other process that happens to inherit LD_PRELOAD is unaffected).

#include "patch.h"

#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <unistd.h>

bool g_enabled = false;
std::FILE *g_logf = nullptr;

namespace {

pid_t gettid_compat(void)
{
#ifdef SYS_gettid
    return (pid_t)syscall(SYS_gettid);
#else
    return -1;
#endif
}

// Fatal-signal diagnostics (SEGV/BUS/ABRT/FPE/ILL). On ARM the unwinder can't
// pass the kernel sigreturn trampoline, so we print the raw register state +
// /proc/self/maps and re-raise for a core. Async-signal-safe primitives only.
void fatal_signal_handler(int sig, siginfo_t *info, void *ucontext)
{
    static const char hdr[] =
        "\n[libpatch-blmjcicarplay][C] *** fatal signal caught ***\n";
    (void)!write(STDERR_FILENO, hdr, sizeof(hdr) - 1);

    char buf[256];
    int  m = snprintf(buf, sizeof(buf),
                 "[libpatch-blmjcicarplay][C] signal=%d pid=%d tid=%d "
                 "si_code=%d si_addr=%p\n",
                 sig, (int)getpid(), (int)gettid_compat(),
                 info ? info->si_code : -1, info ? info->si_addr : nullptr);
    if (m > 0) (void)!write(STDERR_FILENO, buf, (size_t)m);

    if (ucontext) {
        ucontext_t *uc = static_cast<ucontext_t *>(ucontext);
        const mcontext_t &mc = uc->uc_mcontext;
        m = snprintf(buf, sizeof(buf),
                     "[libpatch-blmjcicarplay][C] regs: pc=%08lx lr=%08lx "
                     "sp=%08lx fp=%08lx ip=%08lx cpsr=%08lx fault=%08lx\n",
                     (unsigned long)mc.arm_pc, (unsigned long)mc.arm_lr,
                     (unsigned long)mc.arm_sp, (unsigned long)mc.arm_fp,
                     (unsigned long)mc.arm_ip, (unsigned long)mc.arm_cpsr,
                     (unsigned long)mc.fault_address);
        if (m > 0) (void)!write(STDERR_FILENO, buf, (size_t)m);
    }

    void *frames[64];
    int   n = backtrace(frames, 64);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);

    int mfd = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
    if (mfd >= 0) {
        char mbuf[4096];
        ssize_t rn;
        while ((rn = read(mfd, mbuf, sizeof(mbuf))) > 0) {
            ssize_t off = 0;
            while (off < rn) {
                ssize_t wn = write(STDERR_FILENO, mbuf + off, (size_t)(rn - off));
                if (wn <= 0) break;
                off += wn;
            }
        }
        close(mfd);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

void install_fatal_handler(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = fatal_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE,  &sa, nullptr);
    sigaction(SIGILL,  &sa, nullptr);
}

static int read_cmdline(char *out, size_t outsz)
{
    if (!out || outsz == 0) return 0;
    out[0] = '\0';
    int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    ssize_t n = read(fd, out, outsz - 1);
    close(fd);
    if (n <= 0) { out[0] = '\0'; return 0; }
    for (ssize_t i = 0; i < n; ++i) if (out[i] == '\0') out[i] = ' ';
    out[n] = '\0';
    return (int)n;
}

} // namespace

// Exposed for devmgr_shim.cpp's lazy gate.
bool in_carplay_launcher(void)
{
    char cmdline[512];
    read_cmdline(cmdline, sizeof(cmdline));
    // Launcher cmdline contains both the service tag and the OEM .so path.
    return (strstr(cmdline, "jciCARPLAY") != nullptr) ||
           (strstr(cmdline, "blmjcicarplay.so") != nullptr);
}

__attribute__((constructor))
static void on_load(void)
{
    if (!in_carplay_launcher()) {
        // Not our PID (could be the once-a-second killall watchdog child that
        // inherits LD_PRELOAD). Stay silent + inert.
        return;
    }
    g_enabled = true;
    // Open the bring-up log file (stderr is swallowed by syslog-ng). Append so
    // multiple boots accumulate; line-buffered so tail -f sees lines live.
    g_logf = std::fopen("/tmp/carplay_bridge.log", "a");
    if (g_logf) std::setvbuf(g_logf, nullptr, _IOLBF, 0);
    install_fatal_handler();
    LOGD("=== loaded into jciCARPLAY launcher pid=%d ppid=%d (CarPlay->HUD bridge, FW 74.00.324) ===",
         (int)getpid(), (int)getppid());
}
