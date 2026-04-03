/*
 * FreeBSD kqueue compatibility layer for bun-usockets.
 *
 * Maps macOS-specific kevent64 API to standard FreeBSD kevent API.
 * EVFILT_MACHPORT (Mach ports) is replaced with EVFILT_USER.
 */

#pragma once
#if defined(__FreeBSD__)

#include <sys/event.h>
#include <sys/time.h>

/* Map struct kevent64_s → struct kevent via preprocessor.
 * FreeBSD's struct kevent has ext[4] so field access like .filter/.flags/.udata work. */
#define kevent64_s kevent

/* EV_SET64 ignores the ext0 and ext1 arguments */
#define EV_SET64(kev, a, b, c, d, e, f, g, h) EV_SET(kev, a, b, c, d, e, (void *)(uintptr_t)(f))

/* Flag constants */
#define KEVENT_FLAG_IMMEDIATE  0x1
#define KEVENT_FLAG_ERROR_EVENTS 0x2

/* kevent64 wrapper using standard kevent.
 * KEVENT_FLAG_IMMEDIATE → zero timeout (return immediately).
 * KEVENT_FLAG_ERROR_EVENTS → standard kevent returns EV_ERROR in flags. */
static inline int kevent64(int kq,
    const struct kevent *changelist, int nchanges,
    struct kevent *eventlist, int nevents,
    unsigned int flags,
    const struct timespec *timeout)
{
    static const struct timespec zero_timeout = {0, 0};
    if (flags & KEVENT_FLAG_IMMEDIATE) {
        timeout = &zero_timeout;
    }
    return kevent(kq, changelist, nchanges, eventlist, nevents, timeout);
}

/* EVFILT_MACHPORT replacement: use EVFILT_USER for async wakeup on FreeBSD */
#ifndef EVFILT_MACHPORT
#define EVFILT_MACHPORT EVFILT_USER
#endif

/* NOTE_FFCOPY, NOTE_TRIGGER for EVFILT_USER */
/* These are defined in <sys/event.h> on FreeBSD */
/* mach_port_t → use the callback pointer as ident (cast to uint64_t) */

#endif /* __FreeBSD__ */
