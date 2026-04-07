/* FreeBSD: wrapper for WTF::initializeMainThread().
 *
 * ZigGlobalObject.cpp calls Bun_initializeMainThread_freebsd() instead of
 * WTF::initializeMainThread() directly on FreeBSD.  This TU exists so that
 * the precompiled WebKit's inlined verifyCanGC calls from JSCInitialize
 * stay contained — they don't bleed into ZigGlobalObject.cpp's call graph.
 *
 * Compiled with -fno-exceptions (like the rest of Bun).  The compat shims
 * in freebsd-glibc-compat.c (pthread_once CAS, sched_setscheduler noop,
 * sigaction translation) prevent the C++ exceptions that would otherwise
 * fire during WTF initialization.
 */

/* Forward-declare to avoid pulling in bmalloc / WTF headers. */
namespace WTF { void initializeMainThread(); }

extern "C" void Bun_initializeMainThread_freebsd()
{
    WTF::initializeMainThread();
}
