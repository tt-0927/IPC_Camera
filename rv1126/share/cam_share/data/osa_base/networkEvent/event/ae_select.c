

#ifndef WIN32

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "ae.h"


typedef struct aeApiState {
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;
} aeApiState;

static int aeApiCreate(evEventBase *eventLoop)
{
    aeApiState *state = (aeApiState*)malloc(sizeof(aeApiState));
    if (!state) return -1;

    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    eventLoop->apidata = state;
    return 0;
}

static int aeApiResize(evEventBase *eventLoop, int setsize) {
    /* Just ensure we have enough room in the fd_set type. */
    if (setsize >= FD_SETSIZE) return -1;
    return 0;
}

static void aeApiFree(evEventBase *eventLoop) {
    free(eventLoop->apidata);
}

static int aeApiAddEvent(evEventBase *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;

    if (mask & EV_READABLE) FD_SET(fd,&state->rfds);
    if (mask & EV_WRITABLE) FD_SET(fd,&state->wfds);
    return 0;
}

static void aeApiDelEvent(evEventBase *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;

    if (mask & EV_READABLE) FD_CLR(fd,&state->rfds);
    if (mask & EV_WRITABLE) FD_CLR(fd,&state->wfds);
}

static int aeApiPoll(evEventBase *eventLoop, struct timeval *tvp) {
    aeApiState *state = eventLoop->apidata;
    int retval, j, numevents = 0;

    memcpy(&state->_rfds,&state->rfds,sizeof(fd_set));
    memcpy(&state->_wfds,&state->wfds,sizeof(fd_set));

    retval = select(eventLoop->maxfd+1,
                &state->_rfds,&state->_wfds,NULL,tvp);
    if (retval > 0) {
        for (j = 0; j <= eventLoop->maxfd; j++) {
            int mask = 0;
            evIOEvent *fe = &eventLoop->IOEvents[j];

            if (fe->events == EV_NONE) continue;
            if (fe->events & EV_READABLE && FD_ISSET(j,&state->_rfds))
                mask |= EV_READABLE;
            if (fe->events & EV_WRITABLE && FD_ISSET(j,&state->_wfds))
                mask |= EV_WRITABLE;
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].events = mask;
            numevents++;
        }
    }
    return numevents;
}

static char *aeApiName(void) {
    return "select";
}

#endif
