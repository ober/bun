/* FreeBSD compatibility stubs for glibc-specific symbols referenced by
 * precompiled WebKit/JSC static libraries built on Linux.
 */

#if defined(__FreeBSD__)

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/time.h>

/* --- __gthrw_pthread_cancel --- */
/* GCC's simple __gthread_active_p() checks whether a static pointer
 * `static void *const __gthread_active_ptr = &__gthrw_pthread_cancel;` is non-null.
 * On Linux, __gthrw_pthread_cancel is a weak alias for pthread_cancel in libpthread.so.
 * On FreeBSD this symbol doesn't exist → pointer is NULL → __gthread_active_p() returns
 * false → __gthread_once() returns -1 → std::call_once throws system_error(-1).
 *
 * Provide a wrapper here so WTF's __gthread_active_p() sees a non-null pointer. */
__attribute__((visibility("default")))
int __gthrw_pthread_cancel(pthread_t thread)
{
    return pthread_cancel(thread);
}

/* --- __pthread_key_create --- */
/* GCC's complex __gthread_active_p() variant (used in some WTF TUs) detects pthreads
 * availability via a R_X86_64_GLOB_DAT relocation against __pthread_key_create.
 * glibc exports this internal symbol; FreeBSD's libthr does not, so the GOT entry
 * stays NULL → __gthread_active_p() returns false → same crash as above.
 *
 * Provide a wrapper so the dynamic linker fills the GOT entry at runtime. */
#include <pthread.h>
__attribute__((visibility("default")))
int __pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
    return pthread_key_create(key, destructor);
}

/* --- errno --- */
/* glibc exposes errno as a function returning a pointer; FreeBSD uses __error() */
int* __errno_location(void)
{
    return __error();
}

/* --- assert --- */
/* glibc's __assert_fail; FreeBSD uses __assert or __assert_rtn */
void __assert_fail(const char* assertion, const char* file, unsigned int line, const char* function)
{
    /* Match glibc's format */
    fprintf(stderr, "%s:%u: %s: Assertion `%s' failed.\n", file, line, function ? function : "??", assertion);
    fflush(stderr);
    abort();
}

/* --- sysconf Linux→FreeBSD constant translation --- */
/* Precompiled Linux WebKit/JSC calls sysconf() with Linux-specific constant numbers.
 * The numeric values differ between Linux (glibc) and FreeBSD:
 *   _SC_PAGESIZE:          Linux=30, FreeBSD=47
 *   _SC_NPROCESSORS_CONF:  Linux=83, FreeBSD=57
 *   _SC_NPROCESSORS_ONLN:  Linux=84, FreeBSD=58
 *   _SC_PHYS_PAGES:        Linux=85, FreeBSD=121
 * Without translation, sysconf(30) on FreeBSD returns the wrong value,
 * causing WTF::pageSize() to assert isPowerOfTwo(s_pageSize). */
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <dlfcn.h>

__attribute__((visibility("default")))
long bun_freebsd_sysconf(int name)
    __asm__("sysconf");

__attribute__((visibility("default")))
long bun_freebsd_sysconf(int name)
{
    /* Translate Linux constant numbers to FreeBSD equivalents.
     * Then call the real sysconf via dlsym(RTLD_NEXT). */
    switch (name) {
    case 30: name = _SC_PAGESIZE;          break; /* Linux _SC_PAGESIZE      */
    case 83: name = _SC_NPROCESSORS_CONF;  break; /* Linux _SC_NPROCESSORS_CONF */
    case 84: name = _SC_NPROCESSORS_ONLN;  break; /* Linux _SC_NPROCESSORS_ONLN */
    case 85: name = _SC_PHYS_PAGES;        break; /* Linux _SC_PHYS_PAGES    */
    /* Pass-through for anything already using FreeBSD numbering. */
    }
    typedef long (*sysconf_fn)(int);
    static sysconf_fn real_sysconf = NULL;
    if (!real_sysconf)
        real_sysconf = (sysconf_fn)dlsym(RTLD_NEXT, "sysconf");
    return real_sysconf ? real_sysconf(name) : -1L;
}

/* --- isoc99 scanf wrappers --- */
/* glibc wraps sscanf/fscanf as __isoc99_sscanf / __isoc99_fscanf for C99 conformance.
 * On FreeBSD the standard functions are already C99-conformant. */
int __isoc99_sscanf(const char* str, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsscanf(str, fmt, ap);
    va_end(ap);
    return r;
}

int __isoc99_fscanf(FILE* stream, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vfscanf(stream, fmt, ap);
    va_end(ap);
    return r;
}

/* --- malloc_trim --- */
/* glibc malloc extension — no-op on FreeBSD (mimalloc handles its own trimming) */
int malloc_trim(size_t pad)
{
    (void)pad;
    return 0;
}

/* --- prctl --- */
/* Linux-only system call; stub that signals "not supported" */
#include <sys/syscall.h>
int prctl(int option, ...)
{
    (void)option;
    errno = ENOSYS;
    return -1;
}

/* --- syscall() interpose --- */
/* The precompiled Linux WebKit/JSC library calls syscall() with Linux-specific
 * syscall numbers. The Linux gettid syscall number differs by arch:
 *   x86_64:  186
 *   aarch64: 178
 * FreeBSD doesn't have a gettid syscall — use thr_self() instead.
 *
 * We provide our own syscall() that:
 *   - Translates known Linux-only numbers (gettid) to thr_self().
 *   - Passes everything else straight through to FreeBSD's __sys_syscall.
 */
#include <sys/thr.h>

#if defined(__x86_64__)
#define LINUX_SYS_gettid 186
#elif defined(__aarch64__)
#define LINUX_SYS_gettid 178
#else
#define LINUX_SYS_gettid (-1) /* unknown — pass through */
#endif

/* FreeBSD's internal syscall wrapper, available as a strong symbol in libc.
 * Using it lets us avoid arch-specific inline assembly for the pass-through
 * case and avoids recursive calls into our own syscall() override. */
extern long __sys_syscall(long nr, ...);

/* Rename our C function to the exported symbol "syscall" so the linker prefers
 * this strong static definition over the weak dynamic one in libsys.so.7. */
__attribute__((visibility("default")))
long bun_freebsd_syscall(long nr, long a1, long a2, long a3, long a4, long a5, long a6)
    __asm__("syscall");

__attribute__((visibility("default")))
long bun_freebsd_syscall(long nr, long a1, long a2, long a3, long a4, long a5, long a6)
{
    if (nr == LINUX_SYS_gettid) {
        long tid = -1;
        thr_self(&tid);
        return tid;
    }
    return __sys_syscall(nr, a1, a2, a3, a4, a5, a6);
}

/* --- sysinfo --- */
/* Linux-specific; return a minimal zeroed struct to avoid crashes */
struct sysinfo {
    long uptime;
    unsigned long loads[3];
    unsigned long totalram;
    unsigned long freeram;
    unsigned long sharedram;
    unsigned long bufferram;
    unsigned long totalswap;
    unsigned long freeswap;
    unsigned short procs;
    unsigned long totalhigh;
    unsigned long freehigh;
    unsigned int mem_unit;
    char _f[20 - 2 * sizeof(long) - sizeof(int)];
};

int sysinfo(struct sysinfo* info)
{
    if (!info) {
        errno = EFAULT;
        return -1;
    }
    memset(info, 0, sizeof(*info));
    /* Populate a few fields from FreeBSD equivalents */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    info->uptime = tv.tv_sec; /* rough approximation; real uptime needs sysctl */
    info->mem_unit = 1;
    return 0;
}

/* --- statx --- */
/* Linux-specific extended stat; stub returning ENOSYS */
#ifndef STATX_BASIC_STATS
#define STATX_BASIC_STATS 0x000007ffU
#endif
struct statx_timestamp {
    int64_t tv_sec;
    uint32_t tv_nsec;
    int32_t __reserved;
};
struct statx {
    uint32_t stx_mask;
    uint32_t stx_blksize;
    uint64_t stx_attributes;
    uint32_t stx_nlink;
    uint32_t stx_uid;
    uint32_t stx_gid;
    uint16_t stx_mode;
    uint16_t __spare0[1];
    uint64_t stx_ino;
    uint64_t stx_size;
    uint64_t stx_blocks;
    uint64_t stx_attributes_mask;
    struct statx_timestamp stx_atime;
    struct statx_timestamp stx_btime;
    struct statx_timestamp stx_ctime;
    struct statx_timestamp stx_mtime;
    uint32_t stx_rdev_major;
    uint32_t stx_rdev_minor;
    uint32_t stx_dev_major;
    uint32_t stx_dev_minor;
    uint64_t __spare2[14];
};

int statx(int dfd, const char* pathname, int flags, unsigned int mask, struct statx* buf)
{
    (void)dfd;
    (void)pathname;
    (void)flags;
    (void)mask;
    (void)buf;
    errno = ENOSYS;
    return -1;
}

/* --- stderr / stdout as actual global symbols --- */
/* On Linux, glibc provides stderr and stdout as exported global FILE* variables.
 * On FreeBSD they are macros (__stderrp / __stdoutp).
 * Precompiled Linux objects reference them as symbols, so we provide them here. */
extern FILE *__stderrp; /* declared in FreeBSD libc */
extern FILE *__stdoutp;
FILE *bun_stderr_compat __asm__("stderr");
FILE *bun_stdout_compat __asm__("stdout");

__attribute__((constructor(101))) static void init_bun_stdio_compat(void)
{
    bun_stderr_compat = __stderrp;
    bun_stdout_compat = __stdoutp;
}

/* --- pthread_getattr_np --- */
/* Linux-specific; FreeBSD has pthread_attr_get_np with different argument order.
 * Provide a compatibility wrapper. */
#include <pthread.h>
#include <pthread_np.h>

int pthread_getattr_np(pthread_t thread, pthread_attr_t* attr)
{
    /* FreeBSD equivalent is pthread_attr_get_np(thread, attr) */
    return pthread_attr_get_np(thread, attr);
}

/* --- pthread_once compatibility --- */
/*
 * FreeBSD's pthread_once_t is:
 *   struct pthread_once { int state; pthread_mutex_t mutex; };  // 16 bytes
 *
 * Linux's pthread_once_t is:
 *   int;                                                         //  4 bytes
 *
 * The precompiled Linux WebKit/JSC library has std::once_flag objects whose
 * _M_once member was allocated as 4 bytes (Linux layout).  When FreeBSD's
 * libthr pthread_once() tries to access 16 bytes at that address it reads into
 * adjacent static variables, gets an uninitialised pthread_mutex_t, and returns
 * EINVAL.  std::call_once() then throws std::system_error(EINVAL), which
 * propagates uncaught and hits std::terminate().
 *
 * Fix: provide our own pthread_once() that only touches the first 4 bytes
 * (the 'state' field in both layouts).  We use a simple CAS spin-lock which
 * is safe regardless of whether the caller allocated 4 or 16 bytes for the
 * control block.
 *
 * State values match FreeBSD's <pthread.h> so that calls from correctly-sized
 * FreeBSD once_flag objects also work:
 *   PTHREAD_NEEDS_INIT  = 0
 *   PTHREAD_IN_PROGRESS = 1
 *   PTHREAD_DONE_INIT   = 2
 */
#include <sched.h>   /* sched_yield */

__attribute__((visibility("default")))
int bun_pthread_once_compat(pthread_once_t* once_control, void (*init_routine)(void))
    __asm__("pthread_once");

__attribute__((visibility("default")))
int bun_pthread_once_compat(pthread_once_t* once_control, void (*init_routine)(void))
{
    /* Access only the first 4 bytes: compatible with both 4-byte (Linux) and
     * 16-byte (FreeBSD) layouts. */
    volatile int* state = (volatile int*)once_control;

    /* Fast path: already done */
    if (__atomic_load_n(state, __ATOMIC_ACQUIRE) == 2 /* PTHREAD_DONE_INIT */)
        return 0;

    int expected = 0; /* PTHREAD_NEEDS_INIT */
    if (__atomic_compare_exchange_n(state, &expected, 1 /* PTHREAD_IN_PROGRESS */,
                                    0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        /* We won the CAS – run the initialiser. */
        init_routine();
        __atomic_store_n(state, 2, __ATOMIC_RELEASE); /* PTHREAD_DONE_INIT */
        return 0;
    }

    /* Another thread owns it – spin-wait until done. */
    while (__atomic_load_n(state, __ATOMIC_ACQUIRE) != 2)
        sched_yield();

    return 0;
}

/* --- sched_setscheduler stub ---
 * WTF's RealTimeThreads.cpp calls sched_setscheduler(SCHED_FIFO/SCHED_RR) on the JIT
 * thread to set real-time scheduling priority. On FreeBSD this requires CAP_SYS_NICE or
 * root; without it, the call returns EPERM. WTF then calls std::__throw_system_error(EPERM)
 * which propagates as an uncaught C++ exception through the JSCInitialize call_once lambda
 * and hits std::terminate().
 *
 * Real-time scheduling is a performance optimization for the JIT. It is safe to skip it.
 * We override sched_setscheduler to always pretend success.
 *
 * We also override sched_getscheduler to return SCHED_OTHER (normal scheduling).
 */
#include <sched.h>

__attribute__((visibility("default")))
int bun_sched_setscheduler_stub(pid_t pid, int policy, const struct sched_param *param)
    __asm__("sched_setscheduler");

__attribute__((visibility("default")))
int bun_sched_setscheduler_stub(pid_t pid, int policy, const struct sched_param *param)
{
    (void)pid;
    (void)policy;
    (void)param;
    return 0; /* pretend success — RT scheduling is optional */
}

__attribute__((visibility("default")))
int bun_sched_getscheduler_stub(pid_t pid)
    __asm__("sched_getscheduler");

__attribute__((visibility("default")))
int bun_sched_getscheduler_stub(pid_t pid)
{
    (void)pid;
    return SCHED_OTHER;
}

/* --- __interceptor_sigaction --- */
/* The precompiled Linux WTF was built with ubsan/sanitizer support.
 * It calls __interceptor_sigaction instead of plain sigaction.
 * On FreeBSD, the ubsan interceptor's initializer (InterceptFunction) calls
 * dlsym(RTLD_NEXT, "sigaction") to find the real sigaction, but this may fail
 * to find it (the sanitizer is statically linked and RTLD_NEXT may not search
 * past the binary). When real_sigaction stays NULL every sigaction call returns
 * -1, causing WTF::Thread::initializePlatformThreading()'s ASSERT to fire.
 *
 * Provide a strong definition of __interceptor_sigaction that calls
 * __sys_sigaction (FreeBSD's private direct syscall entry) directly. */
#include <signal.h>
extern int __sys_sigaction(int, const struct sigaction *, struct sigaction *);

/* Linux sa_flags bit values (differ from FreeBSD). */
#define L_SA_SIGINFO    0x00000004
#define L_SA_ONSTACK    0x08000000
#define L_SA_RESTART    0x10000000
#define L_SA_NODEFER    0x40000000
#define L_SA_RESETHAND  0x80000000
#define L_SA_NOCLDSTOP  0x00000001
/* FreeBSD sa_flags bit values. */
#define F_SA_ONSTACK    0x0001
#define F_SA_RESTART    0x0002
#define F_SA_RESETHAND  0x0004
#define F_SA_NOCLDSTOP  0x0008
#define F_SA_NODEFER    0x0010
#define F_SA_NOCLDWAIT  0x0020
#define F_SA_SIGINFO    0x0040

/*
 * Translate a Linux-layout struct sigaction to FreeBSD and call __sys_sigaction.
 *
 * Linux struct:
 *   offset   0: sa_handler/sa_sigaction (8 bytes)
 *   offset   8: sa_mask (sigset_t, 128 bytes)
 *   offset 136: sa_flags (int)
 *   offset 140: sa_restorer (pointer)
 * FreeBSD struct:
 *   offset   0: sa_handler/sa_sigaction (8 bytes)
 *   offset   8: sa_flags (int)
 *   offset  12: padding
 *   offset  16: sa_mask (sigset_t, 16 bytes)
 *
 * Detection: if int at offset 8 has bits above 0xFF set, the caller is using
 * Linux layout (because their sa_mask bytes leak into our sa_flags slot).
 * Native FreeBSD callers always have valid sa_flags <= 0xFF.
 */
static int translate_sigaction(int signo, const struct sigaction *act, struct sigaction *oldact)
{
    if (!act)
        return __sys_sigaction(signo, act, oldact);

    /* Quick check: is this already a FreeBSD-layout struct? */
    {
        int probe;
        __builtin_memcpy(&probe, (const unsigned char*)act + 8, sizeof(probe));
        if (!(probe & ~0xFF))
            return __sys_sigaction(signo, act, oldact);
    }

    const unsigned char *in = (const unsigned char*)act;

    /* Linux sa_flags at offset 136. */
    int linux_flags;
    __builtin_memcpy(&linux_flags, in + 136, sizeof(linux_flags));

    int freebsd_flags = 0;
    if (linux_flags & L_SA_SIGINFO)    freebsd_flags |= F_SA_SIGINFO;
    if (linux_flags & L_SA_ONSTACK)    freebsd_flags |= F_SA_ONSTACK;
    if (linux_flags & L_SA_RESTART)    freebsd_flags |= F_SA_RESTART;
    if (linux_flags & L_SA_NODEFER)    freebsd_flags |= F_SA_NODEFER;
    if (linux_flags & L_SA_RESETHAND)  freebsd_flags |= F_SA_RESETHAND;
    if (linux_flags & L_SA_NOCLDSTOP)  freebsd_flags |= F_SA_NOCLDSTOP;

    struct sigaction native_sa;
    __builtin_memset(&native_sa, 0, sizeof(native_sa));
    __builtin_memcpy(&native_sa, in, sizeof(void*));
    native_sa.sa_flags = freebsd_flags;
    /* First 16 bytes of Linux sa_mask → FreeBSD sa_mask. */
    __builtin_memcpy(&native_sa.sa_mask, in + 8, sizeof(native_sa.sa_mask));

    return __sys_sigaction(signo, &native_sa, oldact);
}

/* Sanitizer-instrumented code calls __interceptor_sigaction instead of sigaction. */
__attribute__((visibility("default")))
int bun_interceptor_sigaction(int signo, const struct sigaction *act, struct sigaction *oldact)
    __asm__("__interceptor_sigaction");

int bun_interceptor_sigaction(int signo, const struct sigaction *act, struct sigaction *oldact)
{
    return translate_sigaction(signo, act, oldact);
}

/* Non-instrumented code calls sigaction directly. */
__attribute__((visibility("default")))
int bun_sigaction_interpose(int signo, const struct sigaction *act, struct sigaction *oldact)
    __asm__("sigaction");

__attribute__((visibility("default")))
int bun_sigaction_interpose(int signo, const struct sigaction *act, struct sigaction *oldact)
{
    return translate_sigaction(signo, act, oldact);
}

/* --- mmap Linux→FreeBSD flag translation --- */
/* Precompiled Linux WebKit/JSC calls mmap() with Linux-specific flag values.
 * The critical difference: Linux MAP_ANONYMOUS=0x0020, FreeBSD MAP_RENAME=0x0020.
 * On FreeBSD, the anonymous-mapping flag is MAP_ANON=0x1000.
 *
 * Concrete example: libpas calls mmap(0, 128MB, PROT_READ|PROT_WRITE, 0x4022, -1, 0)
 * where 0x4022 = MAP_NORESERVE(0x4000)|MAP_ANONYMOUS(0x20)|MAP_PRIVATE(0x2).
 * On FreeBSD 0x0020 means MAP_RENAME (rename private pages to file), not anonymous.
 * With a fd of -1 and no MAP_ANON, FreeBSD mmap returns EINVAL → NULL result →
 * pas_compact_heap_reservation_try_allocate assertion "page_result.result" fires.
 *
 * Linux flag values that differ from FreeBSD:
 *   MAP_ANONYMOUS  = 0x0020 (Linux)  → MAP_ANON = 0x1000 (FreeBSD)
 *   MAP_NORESERVE  = 0x4000 (Linux)  → strip (no-op on FreeBSD anonymous maps)
 *   MAP_STACK      = 0x20000 (Linux) → MAP_STACK = 0x0400 (FreeBSD)
 *   MAP_GROWSDOWN  = 0x0100 (Linux)  → strip (no FreeBSD equivalent)
 *
 * Detection heuristic: if bit 0x0020 is set but bit 0x1000 is not set, the
 * caller is using Linux-style flags.  Native FreeBSD code using MAP_ANON (0x1000)
 * is never affected.  MAP_RENAME (0x20) is obsolete and never used in practice.
 */
#include <sys/mman.h>

#define LINUX_MAP_ANONYMOUS 0x00000020
#define LINUX_MAP_NORESERVE 0x00004000
#define LINUX_MAP_STACK     0x00020000
#define LINUX_MAP_GROWSDOWN 0x00000100
#define FBSD_MAP_ANON       0x00001000
#define FBSD_MAP_STACK      0x00000400

extern void *__sys_mmap(void *, size_t, int, int, int, off_t);
extern int   __sys_madvise(void *, size_t, int);

__attribute__((visibility("default")))
void *bun_freebsd_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
    __asm__("mmap");

__attribute__((visibility("default")))
void *bun_freebsd_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    if ((flags & LINUX_MAP_ANONYMOUS) && !(flags & FBSD_MAP_ANON)) {
        /* Linux-style anonymous mapping flags: translate to FreeBSD equivalents */
        flags &= ~LINUX_MAP_ANONYMOUS;   /* remove MAP_RENAME (wrong on FreeBSD) */
        flags |= FBSD_MAP_ANON;          /* add MAP_ANON */
        flags &= ~LINUX_MAP_NORESERVE;   /* Linux MAP_NORESERVE → not needed on FreeBSD */
        flags &= ~LINUX_MAP_GROWSDOWN;   /* no FreeBSD equivalent, strip */
        if (flags & LINUX_MAP_STACK) {
            flags &= ~LINUX_MAP_STACK;
            flags |= FBSD_MAP_STACK;
        }
    }
    return __sys_mmap(addr, len, prot, flags, fd, offset);
}

/* --- madvise Linux→FreeBSD advice translation --- */
/* Linux and FreeBSD use different numeric values for some madvise() constants.
 * The most important difference affecting libpas:
 *   MADV_FREE = 8 (Linux)  → MADV_FREE = 5 (FreeBSD)
 * Without translation, Linux MADV_FREE (8) maps to FreeBSD MADV_NOCORE (8),
 * which hides memory from core dumps rather than releasing pages. */

#define LINUX_MADV_FREE     8
#define LINUX_MADV_REMOVE        9
#define LINUX_MADV_DONTFORK      11
#define LINUX_MADV_DONTDUMP      16
#define LINUX_MADV_DOFORK        17
#define LINUX_MADV_WIPEONFORK    18
#define LINUX_MADV_KEEPONFORK    19

/* FreeBSD MADV_FREE is 5, same as POSIX */
#ifndef MADV_FREE
#define MADV_FREE 5
#endif

__attribute__((visibility("default")))
int bun_freebsd_madvise(void *addr, size_t len, int advice)
    __asm__("madvise");

__attribute__((visibility("default")))
int bun_freebsd_madvise(void *addr, size_t len, int advice)
{
    switch (advice) {
    case LINUX_MADV_FREE:        advice = MADV_FREE;   break; /* 8 → 5 */
    case LINUX_MADV_REMOVE:      return 0;  /* no FreeBSD equivalent */
    case LINUX_MADV_DONTFORK:    return 0;  /* no FreeBSD equivalent */
    case LINUX_MADV_DONTDUMP:    return 0;  /* no FreeBSD equivalent */
    case LINUX_MADV_DOFORK:      return 0;  /* no-op */
    case LINUX_MADV_WIPEONFORK:  return 0;  /* no FreeBSD equivalent */
    case LINUX_MADV_KEEPONFORK:  return 0;  /* no FreeBSD equivalent */
    /* MADV_NORMAL(0), MADV_RANDOM(1), MADV_SEQUENTIAL(2), MADV_WILLNEED(3),
     * MADV_DONTNEED(4) are identical between Linux and FreeBSD. */
    }
    return __sys_madvise(addr, len, advice);
}

/* --- clock_gettime Linux→FreeBSD clock ID translation --- */
/* Some clock IDs differ between Linux and FreeBSD:
 *   Linux CLOCK_MONOTONIC_COARSE = 6  → FreeBSD CLOCK_MONOTONIC_FAST = 12
 *   Linux CLOCK_REALTIME_COARSE  = 7  → FreeBSD CLOCK_UPTIME_PRECISE = 7
 *     (different semantics; we map to CLOCK_REALTIME_FAST = 10)
 *   Linux CLOCK_BOOTTIME         = 7  → FreeBSD CLOCK_UPTIME = 5
 *
 * WebKit/libpas calls clock_gettime(CLOCK_MONOTONIC_COARSE) which on Linux is
 * 6.  FreeBSD has no clock ID 6 → EINVAL → assertions fire in libpas.
 *
 * Note: Linux CLOCK_MONOTONIC = 1 collides with FreeBSD CLOCK_VIRTUAL = 1.
 * However, WebKit was compiled against glibc headers and Linux numbering, so
 * any reference to "CLOCK_MONOTONIC" in the precompiled .a uses 1.  We
 * translate that to FreeBSD CLOCK_MONOTONIC = 4. */
#include <time.h>
extern int __sys_clock_gettime(clockid_t, struct timespec *);

__attribute__((visibility("default")))
int bun_freebsd_clock_gettime(clockid_t clk_id, struct timespec *tp)
    __asm__("clock_gettime");

__attribute__((visibility("default")))
int bun_freebsd_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    switch ((int)clk_id) {
    case 1:  /* Linux CLOCK_MONOTONIC = FreeBSD CLOCK_VIRTUAL */
        clk_id = CLOCK_MONOTONIC; /* = 4 on FreeBSD */
        break;
    case 6:  /* Linux CLOCK_MONOTONIC_COARSE */
        clk_id = CLOCK_MONOTONIC_FAST; /* = 12 on FreeBSD */
        break;
    /* Leave 7 alone: FreeBSD's CLOCK_UPTIME_PRECISE works as a monotonic
     * clock and is more precise than CLOCK_BOOTTIME. */
    default:
        break;
    }
    return __sys_clock_gettime(clk_id, tp);
}

/* --- getauxval --- */
/* Linux-only auxiliary vector accessor.  Some glibc-compiled code references
 * it (notably libgcc/libstdc++).  Return 0 — caller should treat as "not
 * present" and fall back to other mechanisms. */
__attribute__((visibility("default")))
unsigned long getauxval(unsigned long type)
{
    (void)type;
    return 0;
}

/* --- __timezone --- */
/* glibc exports timezone as __timezone (a `long`).
 * On FreeBSD x86_64, libc has `extern long timezone` from tzset().
 * On FreeBSD aarch64, <time.h> declares `char *timezone(int, int)` (an XSI
 * legacy function), which collides with `extern long timezone`.
 *
 * Use localtime_r() + tm_gmtoff to compute the offset portably. */
long bun_timezone_compat __asm__("__timezone");

__attribute__((constructor(102))) static void init_bun_timezone_compat(void)
{
    tzset();
    time_t t = 0;
    struct tm lt;
    localtime_r(&t, &lt);
    /* glibc convention: __timezone is seconds WEST of UTC.
     * tm_gmtoff is seconds EAST of UTC, so negate. */
    bun_timezone_compat = -(long)lt.tm_gmtoff;
}

#endif /* defined(__FreeBSD__) */
