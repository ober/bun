/* FreeBSD: wrapper for WTF::initializeMainThread().
 *
 * ZigGlobalObject.cpp calls Bun_initializeMainThread_freebsd() instead of
 * WTF::initializeMainThread() directly on FreeBSD.
 *
 * Compiled with -fno-exceptions (like the rest of Bun), so no try/catch here.
 */

#include <stdio.h>
#include <execinfo.h>
#include <exception>

/* Forward-declare to avoid pulling in bmalloc / WTF headers. */
namespace WTF { void initializeMainThread(); }

extern "C" void Bun_initializeMainThread_freebsd()
{
    fprintf(stderr, "[FreeBSD-JSC] calling WTF::initializeMainThread\n"); fflush(stderr);
    WTF::initializeMainThread();
    fprintf(stderr, "[FreeBSD-JSC] WTF::initializeMainThread returned\n"); fflush(stderr);
}
