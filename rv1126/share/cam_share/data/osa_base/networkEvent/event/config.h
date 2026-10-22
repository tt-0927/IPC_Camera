
#ifndef _CORE_SOURCE_OSA_EVENT_CONFIG_INCLUDE_
#define _CORE_SOURCE_OSA_EVENT_CONFIG_INCLUDE_


/* Linux */
#ifdef __linux__
#define HAVE_EPOLL 1
#endif

/* OS X,FreeBSD */
#if (defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#define HAVE_KQUEUE 1
#endif

/* Solaris 10 */
#ifdef __sun
#include <sys/feature_tests.h>
#ifdef _DTRACE_VERSION
#define HAVE_EVPORT 1
#endif
#endif

/* WIN32 */


#endif //_CORE_SOURCE_OSA_EVENT_CONFIG_INCLUDE_

