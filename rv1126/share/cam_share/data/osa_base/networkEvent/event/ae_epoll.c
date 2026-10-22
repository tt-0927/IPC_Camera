

#include "config.h"

#ifdef HAVE_EPOLL

#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "ae.h"

typedef struct aeApiState {
    int epfd;
    struct epoll_event *events;
} aeApiState;

static int aeApiCreate(evEventBase *eventLoop) {
    aeApiState *state = malloc(sizeof(aeApiState));

    if (!state) return -1;
    state->events = malloc(sizeof(struct epoll_event)*eventLoop->setsize);
    if (!state->events) {
        free(state);
        return -1;
    }
    state->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
    if (state->epfd == -1) {
        free(state->events);
        free(state);
        return -1;
    }
    eventLoop->apidata = state;
    return 0;
}

static int aeApiResize(evEventBase *eventLoop, int setsize) {
    aeApiState *state = eventLoop->apidata;

    state->events = realloc(state->events, sizeof(struct epoll_event)*setsize);
    return 0;
}

static void aeApiFree(evEventBase *eventLoop) {
    aeApiState *state = eventLoop->apidata;

    close(state->epfd);
    free(state->events);
    free(state);
}

static int aeApiAddEvent(evEventBase *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = eventLoop->IOEvents[fd].events == EV_NONE ?
            EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= eventLoop->IOEvents[fd].events; /* Merge old events */
    if (mask & EV_READABLE) ee.events |= EPOLLIN;
    if (mask & EV_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd,op,fd,&ee) == -1) return -1;
    return 0;
}

static void aeApiDelEvent(evEventBase *eventLoop, int fd, int delmask) {
    aeApiState *state = eventLoop->apidata;
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    int mask = eventLoop->IOEvents[fd].events & (~delmask);

    ee.events = 0;
    if (mask & EV_READABLE) ee.events |= EPOLLIN;
    if (mask & EV_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (mask != EV_NONE) {
        epoll_ctl(state->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}

static int aeApiPoll(evEventBase *eventLoop, struct timeval *tvp) {
    aeApiState *state = eventLoop->apidata;
    int retval, numevents = 0;

    retval = epoll_wait(state->epfd,state->events,eventLoop->setsize,
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
    if (retval > 0) {
        int j;

        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = state->events+j;

            if (e->events & EPOLLIN) mask |= EV_READABLE;
            if (e->events & EPOLLOUT) mask |= EV_WRITABLE;
            if (e->events & EPOLLERR) mask |= EV_WRITABLE;
            if (e->events & EPOLLHUP) mask |= EV_WRITABLE;
            eventLoop->fired[j].fd = e->data.fd;
            eventLoop->fired[j].events = mask;
        }
    }
    return numevents;
}

static char *aeApiName(void) {
    return "epoll";
}

#endif
