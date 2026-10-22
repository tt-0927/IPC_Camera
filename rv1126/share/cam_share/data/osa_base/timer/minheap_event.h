

#ifndef MINHEAPEVENTFIRECAT_H
#define MINHEAPEVENTFIRECAT_H
 
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
 
//come from https://github.com/libevent/libevent/blob/release-2.1.8-stable/mm-internal.h
#define mm_malloc(sz) malloc(sz)
#define mm_calloc(n, sz) calloc((n), (sz))
#define mm_strdup(s) strdup(s)
#define mm_realloc(p, sz) realloc((p), (sz))
#define mm_free(p) free(p)
 
//come from https://github.com/libevent/libevent/blob/release-2.1.8-stable/include/event2/util.h
#define evutil_timercmp(tvp, uvp, cmp)                          \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?                           \
    ((tvp)->tv_usec cmp (uvp)->tv_usec) :                     \
    ((tvp)->tv_sec cmp (uvp)->tv_sec))
 
#define evutil_timersub(tvp, uvp, vvp)                      \
    do {                                                    \
    (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;     \
    (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;  \
    if ((vvp)->tv_usec < 0) {                         \
    (vvp)->tv_sec--;                             \
    (vvp)->tv_usec += 1000000;                       \
    }                                                   \
    } while (0)
 
#define evutil_timeradd(tvp, uvp, vvp)                          \
    do {                                                        \
    (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;         \
    (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;       \
    if ((vvp)->tv_usec >= 1000000) {                      \
    (vvp)->tv_sec++;                                 \
    (vvp)->tv_usec -= 1000000;                           \
    }                                                       \
    } while (0)
 
//come from https://github.com/libevent/libevent/blob/release-2.1.8-stable/include/event2/event_struct.h
struct event
{
    /* for managing timeouts */
    union {
        //TAILQ_ENTRY(event) ev_next_with_common_timeout;
        int min_heap_idx;
    } ev_timeout_pos;
 
    unsigned int timer_id;
    struct timeval ev_interval;
    struct timeval ev_timeout;
    int ev_exe_num;
 
    int (*ev_callback)(void *arg);	//超时回调函数
    void* ev_arg;					//回调函数的参数
 
    int ev_res; /* result passed to event callback */
    int ev_flags;
};
 
static inline void gettime(struct timeval *tm);
void gettime(struct timeval *tm)
{
    gettimeofday(tm, NULL);
}
 
#endif // MINHEAPEVENTFIRECAT_H

